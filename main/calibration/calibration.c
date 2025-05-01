#include "calibration.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief  Compute the zero‐offset (tare) by averaging `length` samples.
 * @param  zero_raw   Out: the average of the raw readings.
 * @param  raw        In:  array of raw ADC readings.
 * @param  length     Number of samples in `raw[]`.
 * @return 0 on success, <0 on error.
 */
int calibration_tare(int32_t *zero_raw, const int32_t raw[], size_t length)
{
    if (!zero_raw || !raw || length == 0) {
        return -1;
    }
    int64_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += raw[i];
    }
    *zero_raw = (int32_t)(sum / (int64_t)length);
    return 0;
}

#if defined(MULTI_POINT_CALIBRATION)

/**
 * @brief  Multipoint least‐squares fit:
 *         weight = slope * raw + intercept
 */
int calibration_compute(
    calibration_t *cal,
    const int32_t  raw[],
    const float    weight[],
    size_t         length
) {
    if (!cal || !raw || !weight || length < 2) {
        return -1;
    }

    double sum_x   = 0, sum_y   = 0;
    double sum_xy  = 0, sum_x2  = 0;

    for (size_t i = 0; i < length; i++) {
        double x = (double)raw[i];
        double y = (double)weight[i];
        sum_x  += x;
        sum_y  += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    double N     = (double)length;
    double denom = N * sum_x2 - sum_x * sum_x;
    if (denom == 0) {
        return -2;  // All raw[] equal → no fit possible
    }

    double slope     = (N * sum_xy - sum_x * sum_y) / denom;
    double intercept = (sum_y - slope * sum_x) / N;

    cal->slope     = (float)slope;
    cal->intercept = (float)intercept;
    return 0;
}

#else  // SINGLE‐POINT CALIBRATION

/**
 * @brief  Single‐point calibration (plus tare).
 *
 * Compute slope = weight / (raw_at_weight – zero_raw),  
 * intercept = –slope × zero_raw.
 */
int calibration_compute(
    calibration_t *cal,
    int32_t        zero_raw,
    int32_t        raw_at_weight,
    float          weight
) {
    if (!cal || weight == 0.0f) {
        return -1;
    }
    float slope     = weight / (float)(raw_at_weight - zero_raw);
    float intercept = -slope * (float)zero_raw;

    cal->slope     = slope;
    cal->intercept = intercept;
    return 0;
}

#endif

/**
 * @brief  Convert a raw ADC value into weight using the computed model.
 * @param  cal   The slope/intercept model.
 * @param  raw   The raw ADC reading.
 * @return       The weight in the same units used for calibration.
 */
float calibration_convert(const calibration_t *cal, int32_t raw)
{
    return cal->slope * (float)raw + cal->intercept;
}
