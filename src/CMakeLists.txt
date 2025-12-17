# ESP-IDF Component CMakeLists.txt for PulseTracker

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.cpp)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        mqtt
        bt
        driver
        esp_adc
    PRIV_REQUIRES
        bt
)
