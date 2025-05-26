#include "led.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#define TAG "LED"
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_OUTPUT_IO  2  // GPIO pin
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT   // Set duty resolution to 8 bits
#define LEDC_FREQUENCY          5000               // Frequency in Hertz. Set frequency at 5 kHz

static uint8_t led_state = 0;

void led_init(void) {
    // Prepare and set configuration of timers
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and set configuration of LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // LED off initially
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void led_on(void) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 255);  // Max duty for full brightness
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    led_state = 1;
}

void led_off(void) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);  // 0% duty = off
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    led_state = 0;
}

uint8_t get_led_state(void) {
    return led_state;
}
