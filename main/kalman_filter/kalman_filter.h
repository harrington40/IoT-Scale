#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    // Core Kalman variables
    float x_est;    // State estimate
    float P_est;    // Estimate covariance
    float Q;        // Process noise
    float R;        // Measurement noise
    float K;        // Kalman gain
    
    // Adaptive parameters
    float Q_min;
    float Q_max;
    float R_min;
    float R_max;
    float threshold;
    
    // RTOS protection
    SemaphoreHandle_t mutex;
} KalmanFilter;

// Basic initialization
void kalman_init(KalmanFilter *kf, float process_noise, float measurement_noise);

// Adaptive initialization
void kalman_init_adaptive(KalmanFilter *kf, float process_noise, float measurement_noise);

// RTOS initialization
void kalman_init_rtos(KalmanFilter *kf, float process_noise, float measurement_noise, bool is_adaptive);

// Standard update
float kalman_update(KalmanFilter *kf, float measurement);

// Adaptive update
float kalman_update_adaptive(KalmanFilter *kf, float measurement);

// Thread-safe versions
float kalman_update_rtos(KalmanFilter *kf, float measurement);
float kalman_update_adaptive_rtos(KalmanFilter *kf, float measurement);

// Cleanup
void kalman_deinit(KalmanFilter *kf);

#ifdef __cplusplus
}
#endif

#endif // KALMAN_FILTER_H