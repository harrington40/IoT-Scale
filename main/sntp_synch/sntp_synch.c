// File: main/sntp_sync/sntp_sync.c

#include "sntp_synch.h"
#include "esp_sntp.h"
#include "freertos/event_groups.h"

#include <sys/time.h>      // for gettimeofday()
#include <time.h>          // for struct tm, localtime_r(), strftime()
#include "log_utils.h"     // for LOG_ROW()

static const char *TAG = "sntp_sync";
static EventGroupHandle_t s_time_event;
#define TIME_SYNC_BIT BIT0

static void time_sync_notification_cb(struct timeval *tv)
{
    // 1) Log the SNTP-sync event
    LOG_ROW(TAG, "SNTP time synchronised");

    // 2) Fetch the new system time
    struct timeval now;
    gettimeofday(&now, NULL);

    // 3) Convert to broken-down time
    struct tm timeinfo;
    localtime_r(&now.tv_sec, &timeinfo);

    // 4) Format as "YYYY-MM-DD HH:MM:SS"
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // 5) Log the actual calendar time that was set
    LOG_ROW(TAG, "→ System time set to %s", buf);

    // 6) Signal waiting tasks
    xEventGroupSetBits(s_time_event, TIME_SYNC_BIT);
}

void sntp_sync_start(void)
{
    s_time_event = xEventGroupCreate();
    LOG_ROW(TAG, "Starting SNTP client, polling pool.ntp.org");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

bool sntp_time_available(TickType_t timeout)
{
    EventBits_t bits = xEventGroupWaitBits(
        s_time_event,
        TIME_SYNC_BIT,
        pdFALSE,   // do not clear on exit
        pdTRUE,    // wait for all bits (only one here)
        timeout
    );
    return (bits & TIME_SYNC_BIT) != 0;
}
