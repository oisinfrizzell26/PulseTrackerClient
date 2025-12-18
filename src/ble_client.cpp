/*******************************************************************************
 * ESP32 BLE Client - Workout Data Receiver
 *
 * Connects to MAX32655 BLE peripheral and receives workout data.
 * Uses ESP-IDF native NimBLE BLE APIs.
 * Integrated with MQTT to forward workout data to broker.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

// NimBLE includes
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "ble_client.h"
#include "app_mqtt.h"
#include "hr_session.h"

static const char *TAG = "BLE_CLIENT";

// UUIDs - MUST match MAX32655 ble_uuid.h
static const ble_uuid128_t service_uuid =
    BLE_UUID128_INIT(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const ble_uuid128_t tx_char_uuid =
    BLE_UUID128_INIT(0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const ble_uuid128_t rx_char_uuid =
    BLE_UUID128_INIT(0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

#define TARGET_DEVICE_NAME "MAX32655"

// Connection state
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t tx_char_handle = 0;
static uint16_t tx_char_def_handle = 0;
static uint16_t tx_cccd_handle = 0;
static uint16_t rx_char_handle = 0;
static uint16_t rx_char_def_handle = 0;
static uint16_t service_start_handle = 0;
static uint16_t service_end_handle = 0;
static bool connected = false;
static bool service_discovered = false;
static bool mtu_exchanged = false;

// Callback for workout data
static ble_workout_callback_t workout_callback = NULL;

// Forward declarations
static void ble_app_scan(void);
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static void exchange_mtu(void);
static void request_conn_params_update(void);

/*******************************************************************************
 * Workout Data Processing
 ******************************************************************************/

static void format_time(uint32_t ms, char *buf, size_t buf_len)
{
    uint32_t minutes = ms / 60000;
    uint32_t seconds = (ms % 60000) / 1000;
    uint32_t millis = ms % 1000;
    snprintf(buf, buf_len, "%02lu:%02lu.%03lu",
             (unsigned long)minutes, (unsigned long)seconds, (unsigned long)millis);
}

static bool json_get_string(const char *json, const char *key, char *out, size_t out_len)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":\"", key);

    const char *start = strstr(json, search);
    if (start == NULL)
        return false;

    start += strlen(search);
    const char *end = strchr(start, '"');
    if (end == NULL)
        return false;

    size_t len = end - start;
    if (len >= out_len)
        len = out_len - 1;

    strncpy(out, start, len);
    out[len] = '\0';
    return true;
}

static bool json_get_int(const char *json, const char *key, int *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char *start = strstr(json, search);
    if (start == NULL)
        return false;

    start += strlen(search);
    *out = atoi(start);
    return true;
}

static bool json_get_ulong(const char *json, const char *key, unsigned long *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char *start = strstr(json, search);
    if (start == NULL)
        return false;

    start += strlen(search);
    *out = strtoul(start, NULL, 10);
    return true;
}

static void process_workout_event(const char *json_data, uint16_t len)
{
    if (strstr(json_data, "\"cmd\":\"hr_req\"")) {
        ESP_LOGI(TAG, "HR request received from MAX - starting 5s capture");
        hr_session_start(0);
        return;
    }

    char event_type[16] = {0};
    char mode[16] = {0};
    char time_str[16] = {0};
    int lap_num = 0;
    int total_laps = 0;
    unsigned long lap_ms = 0;
    unsigned long split_ms = 0;
    unsigned long total_ms = 0;

    ESP_LOGI(TAG, "Raw workout data (%d bytes): %s", len, json_data);

    // Forward raw JSON to MQTT
    if (workout_callback) {
        workout_callback(json_data, len);
    }

    // Also publish to MQTT directly
    mqtt_publish_workout_data(json_data);

    if (!json_get_string(json_data, "event", event_type, sizeof(event_type))) {
        ESP_LOGW(TAG, "Could not parse event type");
        return;
    }

    printf("\n========================================\n");

    if (strcmp(event_type, "start") == 0) {
        json_get_string(json_data, "mode", mode, sizeof(mode));
        json_get_int(json_data, "laps", &total_laps);

        printf(">>> WORKOUT STARTED!\n");
        printf("    Mode: %s (%d laps)\n", mode, total_laps);
    }
    else if (strcmp(event_type, "lap") == 0) {
        json_get_int(json_data, "lap", &lap_num);
        json_get_ulong(json_data, "lap_ms", &lap_ms);
        json_get_ulong(json_data, "split_ms", &split_ms);

        format_time(lap_ms, time_str, sizeof(time_str));
        printf(">>> LAP %d COMPLETE\n", lap_num);
        printf("    Lap Time:   %s\n", time_str);

        format_time(split_ms, time_str, sizeof(time_str));
        printf("    Split Time: %s\n", time_str);
    }
    else if (strcmp(event_type, "done") == 0) {
        json_get_int(json_data, "laps", &total_laps);
        json_get_ulong(json_data, "total_ms", &total_ms);

        format_time(total_ms, time_str, sizeof(time_str));
        printf(">>> WORKOUT COMPLETE!\n");
        printf("    Total Laps: %d\n", total_laps);
        printf("    Total Time: %s\n", time_str);
    }
    else if (strcmp(event_type, "stop") == 0) {
        json_get_int(json_data, "laps", &lap_num);
        json_get_ulong(json_data, "total_ms", &total_ms);

        format_time(total_ms, time_str, sizeof(time_str));
        printf(">>> WORKOUT STOPPED\n");
        printf("    Laps Completed: %d\n", lap_num);
        printf("    Time: %s\n", time_str);
    }
    else if (strcmp(event_type, "status") == 0) {
        char state[16] = {0};
        json_get_string(json_data, "state", state, sizeof(state));
        json_get_int(json_data, "lap", &lap_num);
        json_get_ulong(json_data, "elapsed_ms", &total_ms);

        format_time(total_ms, time_str, sizeof(time_str));
        printf(">>> STATUS UPDATE\n");
        printf("    State: %s\n", state);
        printf("    Current Lap: %d\n", lap_num);
        printf("    Elapsed: %s\n", time_str);
    }
    else {
        printf(">>> Unknown Event: %s\n", event_type);
    }

    printf("========================================\n\n");
}

/*******************************************************************************
 * BLE Functions
 ******************************************************************************/

static const ble_uuid16_t cccd_uuid = BLE_UUID16_INIT(0x2902);

static int ble_on_mtu_exchange(uint16_t conn_handle, const struct ble_gatt_error *error,
                               uint16_t mtu, void *arg)
{
    if (error->status == 0) {
        ESP_LOGI(TAG, "MTU exchange complete: MTU=%d", mtu);
        mtu_exchanged = true;
    } else {
        ESP_LOGE(TAG, "MTU exchange failed: %d", error->status);
    }
    return 0;
}

static void exchange_mtu(void)
{
    ESP_LOGI(TAG, "Requesting MTU exchange (preferred: 256)...");
    int rc = ble_gattc_exchange_mtu(conn_handle, ble_on_mtu_exchange, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "MTU exchange request failed: %d", rc);
    }
}

static void request_conn_params_update(void)
{
    struct ble_gap_upd_params params = {
        .itvl_min = 12,
        .itvl_max = 24,
        .latency = 0,
        .supervision_timeout = 400,
        .min_ce_len = 0,
        .max_ce_len = 0,
    };

    ESP_LOGI(TAG, "Requesting connection parameter update...");
    int rc = ble_gap_update_params(conn_handle, &params);
    if (rc != 0) {
        ESP_LOGE(TAG, "Connection param update request failed: %d", rc);
    }
}

static int ble_on_notify(uint16_t conn_handle, const struct ble_gatt_error *error,
                         struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        printf("\n========================================\n");
        printf("   NOTIFICATIONS ENABLED!\n");
        printf("   Listening for workout data...\n");
        printf("========================================\n\n");
    } else {
        ESP_LOGE(TAG, "Failed to enable notifications: %d", error->status);
    }
    return 0;
}

static void subscribe_to_notifications(void)
{
    uint8_t value[2] = {0x01, 0x00};

    uint16_t cccd;
    if (tx_cccd_handle != 0) {
        cccd = tx_cccd_handle;
        ESP_LOGI(TAG, "Using discovered CCCD handle: %d", cccd);
    } else {
        cccd = tx_char_handle + 1;
        ESP_LOGW(TAG, "CCCD not discovered! Assuming handle: %d", cccd);
    }

    ESP_LOGI(TAG, "=== SUBSCRIBING TO NOTIFICATIONS ===");
    ESP_LOGI(TAG, "TX Char Value Handle: %d", tx_char_handle);
    ESP_LOGI(TAG, "CCCD Handle: %d", cccd);

    int rc = ble_gattc_write_flat(conn_handle, cccd, value, sizeof(value),
                                  ble_on_notify, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to subscribe: %d", rc);

        if (tx_cccd_handle == 0) {
            ESP_LOGI(TAG, "Trying alternative CCCD handle...");
            cccd = tx_char_handle + 2;
            rc = ble_gattc_write_flat(conn_handle, cccd, value, sizeof(value),
                                      ble_on_notify, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "Alternative subscribe also failed: %d", rc);
            }
        }
    }
}

static int ble_on_desc_discovery(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 uint16_t chr_val_handle,
                                 const struct ble_gatt_dsc *dsc, void *arg)
{
    if (error->status == 0 && dsc != NULL) {
        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(&dsc->uuid.u, uuid_str);

        ESP_LOGI(TAG, "Descriptor found: handle=%d, uuid=%s", dsc->handle, uuid_str);

        if (ble_uuid_cmp(&dsc->uuid.u, &cccd_uuid.u) == 0) {
            tx_cccd_handle = dsc->handle;
            ESP_LOGI(TAG, ">>> Found CCCD at handle %d <<<", tx_cccd_handle);
        }
    }
    else if (error->status == BLE_HS_EDONE) {
        ESP_LOGI(TAG, "Descriptor discovery complete");

        printf("\n========================================\n");
        printf("   CONNECTED TO MAX32655 TRACKER\n");
        printf("   TX Handle: %d, CCCD: %d\n", tx_char_handle,
               tx_cccd_handle ? tx_cccd_handle : (tx_char_handle + 1));
        printf("   Enabling notifications...\n");
        printf("========================================\n\n");

        service_discovered = true;
        subscribe_to_notifications();
    }
    else {
        ESP_LOGE(TAG, "Descriptor discovery error: %d", error->status);
        subscribe_to_notifications();
    }
    return 0;
}

static void discover_descriptors(void)
{
    uint16_t start = tx_char_handle + 1;
    uint16_t end = (rx_char_def_handle > 0) ? (rx_char_def_handle - 1) : service_end_handle;

    if (end < start) {
        end = start;
    }

    ESP_LOGI(TAG, "Discovering descriptors (handles %d-%d)...", start, end);

    int rc = ble_gattc_disc_all_dscs(conn_handle, start, end,
                                     ble_on_desc_discovery, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Descriptor discovery failed: %d", rc);
        subscribe_to_notifications();
    }
}

static int ble_on_char_discovery(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_chr *chr, void *arg)
{
    if (error->status == 0 && chr != NULL) {
        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(&chr->uuid.u, uuid_str);
        ESP_LOGI(TAG, "Characteristic: def=%d, val=%d, props=0x%02x, uuid=%s",
                 chr->def_handle, chr->val_handle, chr->properties, uuid_str);

        if (ble_uuid_cmp(&chr->uuid.u, &tx_char_uuid.u) == 0) {
            tx_char_def_handle = chr->def_handle;
            tx_char_handle = chr->val_handle;
            ESP_LOGI(TAG, ">>> TX characteristic found");
        }
        else if (ble_uuid_cmp(&chr->uuid.u, &rx_char_uuid.u) == 0) {
            rx_char_def_handle = chr->def_handle;
            rx_char_handle = chr->val_handle;
            ESP_LOGI(TAG, ">>> RX characteristic found");
        }
    }
    else if (error->status == BLE_HS_EDONE) {
        ESP_LOGI(TAG, "Characteristic discovery complete");

        if (tx_char_handle != 0) {
            discover_descriptors();
        } else {
            ESP_LOGE(TAG, "TX characteristic not found!");
        }
    }
    return 0;
}

static int ble_on_service_discovery(uint16_t conn_handle,
                                    const struct ble_gatt_error *error,
                                    const struct ble_gatt_svc *service, void *arg)
{
    if (error->status == 0 && service != NULL) {
        service_start_handle = service->start_handle;
        service_end_handle = service->end_handle;

        ESP_LOGI(TAG, "Found service (handles %d-%d)", service_start_handle, service_end_handle);

        int rc = ble_gattc_disc_all_chrs(conn_handle,
                                         service_start_handle,
                                         service_end_handle,
                                         ble_on_char_discovery, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Char discovery failed: %d", rc);
        }
    }
    else if (error->status == BLE_HS_EDONE) {
        if (service_start_handle == 0) {
            ESP_LOGE(TAG, "Service not found!");
        }
    }
    return 0;
}

static void discover_services(void)
{
    ESP_LOGI(TAG, "Discovering services...");

    int rc = ble_gattc_disc_svc_by_uuid(conn_handle, &service_uuid.u,
                                        ble_on_service_discovery, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Service discovery failed: %d", rc);
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
    {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                         event->disc.length_data);
        if (rc != 0)
            return 0;

        if (fields.name != NULL && fields.name_len > 0) {
            if (fields.name_len == strlen(TARGET_DEVICE_NAME) &&
                memcmp(fields.name, TARGET_DEVICE_NAME, fields.name_len) == 0) {
                ESP_LOGI(TAG, "Found MAX32655! Connecting...");
                ble_gap_disc_cancel();

                rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr,
                                     30000, NULL, ble_gap_event, NULL);
                if (rc != 0) {
                    ESP_LOGE(TAG, "Connect failed: %d", rc);
                    ble_app_scan();
                }
            }
        }
        return 0;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
    {
        if (!connected) {
            ESP_LOGI(TAG, "Scan complete, retrying...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ble_app_scan();
        }
        return 0;
    }

    case BLE_GAP_EVENT_CONNECT:
    {
        if (event->connect.status == 0) {
            conn_handle = event->connect.conn_handle;
            connected = true;
            mtu_exchanged = false;

            printf("\n========================================\n");
            printf("   CONNECTED TO MAX32655!\n");
            printf("   Connection Handle: %d\n", conn_handle);
            printf("========================================\n\n");

            request_conn_params_update();
            exchange_mtu();
            vTaskDelay(pdMS_TO_TICKS(100));
            discover_services();
        } else {
            ESP_LOGE(TAG, "Connection failed: %d", event->connect.status);
            connected = false;
            ble_app_scan();
        }
        return 0;
    }

    case BLE_GAP_EVENT_DISCONNECT:
    {
        printf("\n!!! BLE DISCONNECTED (reason: %d) - Reconnecting...\n\n",
               event->disconnect.reason);

        hr_session_cancel();
        connected = false;
        service_discovered = false;
        mtu_exchanged = false;
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        tx_char_handle = 0;
        tx_char_def_handle = 0;
        tx_cccd_handle = 0;
        rx_char_handle = 0;
        rx_char_def_handle = 0;
        service_start_handle = 0;
        service_end_handle = 0;

        vTaskDelay(pdMS_TO_TICKS(500));
        ble_app_scan();
        return 0;
    }

    case BLE_GAP_EVENT_NOTIFY_RX:
    {
        uint16_t attr_handle = event->notify_rx.attr_handle;
        uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);

        ESP_LOGI(TAG, "Notification received: handle=%d, len=%d", attr_handle, len);

        if (len > 0 && len < 512) {
            char buffer[512];
            int rc = os_mbuf_copydata(event->notify_rx.om, 0, len, buffer);
            if (rc == 0) {
                buffer[len] = '\0';
                process_workout_event(buffer, len);
            }
        }
        return 0;
    }

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Connection params updated");
        return 0;

    default:
        return 0;
    }
}

static void ble_app_scan(void)
{
    struct ble_gap_disc_params disc_params = {
        .itvl = 0x0050,
        .window = 0x0030,
        .filter_policy = 0,
        .limited = 0,
        .passive = 0,
        .filter_duplicates = 1,
    };

    ESP_LOGI(TAG, "Scanning for MAX32655...");

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 30000, &disc_params,
                          ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Scan failed: %d", rc);
    }
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Address config failed: %d", rc);
        return;
    }

    ble_svc_gap_device_name_set("ESP32-WorkoutRx");
    ble_app_scan();
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE reset: %d", reason);
}

/*******************************************************************************
 * Public API
 ******************************************************************************/

void ble_client_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE client...");

    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE init failed: %s", esp_err_to_name(ret));
        return;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_att_set_preferred_mtu(256);

    ble_svc_gap_init();
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE client initialized");
}

bool ble_client_is_connected(void)
{
    return connected;
}

void ble_client_set_workout_callback(ble_workout_callback_t callback)
{
    workout_callback = callback;
}

bool ble_client_send_message(const char *msg)
{
    if (!connected || rx_char_handle == 0 || msg == NULL) {
        ESP_LOGW(TAG, "BLE TX not ready");
        return false;
    }

    int rc = ble_gattc_write_flat(conn_handle, rx_char_handle,
                                  msg, strlen(msg),
                                  NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Write failed: %d", rc);
        return false;
    }

    ESP_LOGI(TAG, "TX → MAX: %s", msg);
    return true;
}
