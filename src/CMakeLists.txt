# ESP-IDF Component CMakeLists.txt for PulseTracker

FILE(GLOB_RECURSE app_sources_cpp ${CMAKE_SOURCE_DIR}/src/*.cpp)
FILE(GLOB_RECURSE app_sources_c ${CMAKE_SOURCE_DIR}/src/*.c)

idf_component_register(
    SRCS
        ${app_sources_cpp}
        ${app_sources_c}
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