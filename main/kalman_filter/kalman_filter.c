#include "kalman_filter.h"

// Basic initialization
void kalman_init(KalmanFilter *kf, float process_noise, float measurement_noise) {
    if (!kf) return;
    
    kf->x_est = 0.0f;
    kf->P_est = 1.0f;
    kf->Q = process_noise;
    kf->R = measurement_noise;
    kf->K = 0.0f;
    kf->mutex = NULL; // No mutex for basic version
}

// Adaptive initialization
void kalman_init_adaptive(KalmanFilter *kf, float process_noise, float measurement_noise) {
    kalman_init(kf, process_noise, measurement_noise);
    // Set default adaptive parameters
    kf->Q_min = 0.001f;
    kf->Q_max = 0.1f;
    kf->R_min = 0.5f;
    kf->R_max = 5.0f;
    kf->threshold = 10.0f;
}

// RTOS initialization
void kalman_init_rtos(KalmanFilter *kf, float process_noise, float measurement_noise, bool is_adaptive) {
    if (is_adaptive) {
        kalman_init_adaptive(kf, process_noise, measurement_noise);
    } else {
        kalman_init(kf, process_noise, measurement_noise);
    }
    kf->mutex = xSemaphoreCreateMutex();
}

// Standard update (non-thread-safe)
float kalman_update(KalmanFilter *kf, float measurement) {
    if (!kf) return 0.0f;
    
    // Prediction
    float P_pred = kf->P_est + kf->Q;
    
    // Update
    kf->K = P_pred / (P_pred + kf->R);
    kf->x_est = kf->x_est + kf->K * (measurement - kf->x_est);
    kf->P_est = (1.0f - kf->K) * P_pred;
    
    return kf->x_est;
}

// Adaptive update (non-thread-safe)
float kalman_update_adaptive(KalmanFilter *kf, float measurement) {
    if (!kf) return 0.0f;
    
    // 1. Normal update
    float innovation = measurement - kf->x_est;
    kf->P_est += kf->Q;
    kf->K = kf->P_est / (kf->P_est + kf->R);
    kf->x_est += kf->K * innovation;
    kf->P_est *= (1 - kf->K);
    
    // 2. Adaptive tuning for load cells
    float abs_innov = fabsf(innovation);
    if (abs_innov > kf->threshold) {
        // Increase Q (trust measurements more)
        kf->Q = fminf(kf->Q * 1.2f, kf->Q_max);
        kf->R = fmaxf(kf->R * 0.8f, kf->R_min);
    } else {
        // Decrease Q (trust model more)
        kf->Q = fmaxf(kf->Q * 0.8f, kf->Q_min);
        kf->R = fminf(kf->R * 1.2f, kf->R_max);
    }
    
    return kf->x_est;
}

// Thread-safe standard update
float kalman_update_rtos(KalmanFilter *kf, float measurement) {
    if (!kf) return 0.0f;
    
    float result = 0.0f;
    
    if (kf->mutex && xSemaphoreTake(kf->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        result = kalman_update(kf, measurement);
        xSemaphoreGive(kf->mutex);
    } else if (!kf->mutex) {
        result = kalman_update(kf, measurement);
    }
    
    return result;
}

// Thread-safe adaptive update
float kalman_update_adaptive_rtos(KalmanFilter *kf, float measurement) {
    if (!kf) return 0.0f;
    
    float result = 0.0f;
    
    if (kf->mutex && xSemaphoreTake(kf->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        result = kalman_update_adaptive(kf, measurement);
        xSemaphoreGive(kf->mutex);
    } else if (!kf->mutex) {
        result = kalman_update_adaptive(kf, measurement);
    }
    
    return result;
}

// Cleanup resources
void kalman_deinit(KalmanFilter *kf) {
    if (!kf) return;
    
    // Free RTOS resources if they exist
    if (kf->mutex) {
        vSemaphoreDelete(kf->mutex);
        kf->mutex = NULL;
    }
}