idf_component_register(
    

    EMBED_TXTFILES 
       "certs/fullchain.pem"
       "certs/privkey.pem"   
        
    SRCS  
        "main.c"
        
        "hx711/hx711.c"
        "calibration/calibration.c"
        "kalman_filter/kalman_filter.c"
        "bluetooth_nimble/blue_tooth.c"
        
        "moving_avg/moving_average.c"
        "weight_manager/weight_manager.c"
        "wifi_power_management/wifi_power_management.c"
        "wifi/wifi_comm.c"
        "bluetooth/bluetooth.c"
        "body_composition/body_composition.c"
        "spiffs_manager/spiffs_manager.c"
        "sntp_synch/sntp_synch.c"
        
        "wifi_https_server/wifi_https_server.c"       
        
    INCLUDE_DIRS
        
        "."        # for main.h, etc.
        "hx711" 
        "calibration"
        "kalman_filter"
        "moving_avg" 
        "weight_manager" # for hx711/hx711.h
        "wifi_power_management"
        "wifi"
        "bluetooth_nimble"
        "body_composition"
        "spiffs_manager"
        "sntp_synch"
        
        "log_utils"
        "certs"
        "wifi_https_server"
   
    
        
        
        

        
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

