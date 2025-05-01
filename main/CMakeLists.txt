idf_component_register(
    SRCS  
        "main.c"
        "hx711/hx711.c"
        "calibration/calibration.c"
        "kalman_filter/kalman_filter.c"
        "moving_avg/moving_average.c"
    INCLUDE_DIRS
        "."        # for main.h, etc.
        "hx711" 
        "calibration"
        "kalman_filter"
        "moving_avg"  # for hx711/hx711.h
    REQUIRES
        driver     # for gpio_*
        esp_rom    # for ets_delay_us()

)
