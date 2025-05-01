// calibration.h

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    float slope;
    float intercept;
} calibration_t;

/**
 * @brief  Compute tare (zero‐offset) by averaging raw samples.
 */
int calibration_tare(int32_t *zero_raw, const int32_t raw[], size_t length);

#if defined(MULTI_POINT_CALIBRATION)
int calibration_compute(
    calibration_t *cal,
    const int32_t  raw[],
    const float    weight[],
    size_t         length
);
#else
int calibration_compute(
    calibration_t *cal,
    int32_t        zero_raw,
    int32_t        raw_at_weight,
    float          weight
);
#endif

/**
 * @brief  Convert raw reading to weight.
 */
float calibration_convert(const calibration_t *cal, int32_t raw);

#endif // CALIBRATION_H
