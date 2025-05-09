idf_component_register(
    EMBED_FILES

        "certs/fullchain.crt"
        "certs/server.key"
   
   

        
    SRCS  
        "main.c"
        "certs/server_cert.c"
        "hx711/hx711.c"
        "calibration/calibration.c"
        "kalman_filter/kalman_filter.c"
        "moving_avg/moving_average.c"
        "weight_manager/weight_manager.c"
        "wifi_power_management/wifi_power_management.c"
        "wifi/wifi_comm.c"
        "bluetooth/bluetooth.c"
        "body_composition/body_composition.c"
        "spiffs_manager/spiffs_manager.c"
        "sntp_synch/sntp_synch.c"
        "bluetooth_comm_gatt/bluetooth_comm_gatt.c"
        
    INCLUDE_DIRS
        
        "."        # for main.h, etc.
        "hx711" 
        "calibration"
        "kalman_filter"
        "moving_avg" 
        "weight_manager" # for hx711/hx711.h
        "wifi_power_management"
        "wifi"
        "bluetooth"
        "body_composition"
        "spiffs_manager"
        "sntp_synch"
        "bluetooth_comm_gatt"
        "log_utils"
        "certs"
   
    
        
        
        

        
    REQUIRES
        driver     # for gpio_*
        esp_rom    # for ets_delay_us()
        esp_wifi
        bt
        spiffs
        json
        nvs_flash
        esp_coex
        esp-tls
        mbedtls
        esp_http_server
        esp_https_server

            
)

