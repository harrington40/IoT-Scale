// File: main/sntp_synch/sntp_synch.c

#include "sntp_synch.h"
#include "esp_sntp.h"
#include "freertos/event_groups.h"
#include "log_utils.h"
#include <sys/time.h>
#include <time.h>

static const char *TAG = "sntp_sync";
static EventGroupHandle_t s_time_event = NULL;
#define TIME_SYNC_BIT BIT0

/**
 * @brief SNTP time sync callback; logs the new system time and sets event bit
 */
static void time_sync_notification_cb(struct timeval *tv)
{
    LOG_ROW(TAG, "SNTP time synchronised");

    struct timeval now;
    gettimeofday(&now, NULL);

    struct tm timeinfo;
    localtime_r(&now.tv_sec, &timeinfo);

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    LOG_ROW(TAG, "→ System time set to %s", buf);

    xEventGroupSetBits(s_time_event, TIME_SYNC_BIT);
}

void sntp_sync_start(void)
{
    if (s_time_event != NULL) {
        // Already started, do nothing
        return;
    }
    s_time_event = xEventGroupCreate();
    LOG_ROW(TAG, "Starting SNTP client, polling pool.ntp.org");

    // Initialize SNTP operating mode and callback
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

bool sntp_time_available(TickType_t timeout)
{
    if (!s_time_event) {
        return false;
    }
    EventBits_t bits = xEventGroupWaitBits(
        s_time_event,
        TIME_SYNC_BIT,
        pdFALSE,
        pdTRUE,
        timeout
    );
    return (bits & TIME_SYNC_BIT) != 0;
}