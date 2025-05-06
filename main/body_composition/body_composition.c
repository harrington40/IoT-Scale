#include "body_composition.h"
#include "cJSON.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// For file writing
#include "esp_spiffs.h"
#include "esp_log.h"

#define TAG "BODY_COMPOSITION"
#define JSON_FILE_PATH "/spiffs/body_comp.json"

#define LOG10(x) log10f((x))

static bool write_json_to_file(const char *json_str) {
    FILE *file = fopen(JSON_FILE_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", JSON_FILE_PATH);
        return false;
    }
    fprintf(file, "%s", json_str);
    fclose(file);
    ESP_LOGI(TAG, "Saved body composition data to %s", JSON_FILE_PATH);
    return true;
}

static void save_body_composition_json(const body_input_t *input, const body_composition_t *output) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "gender", input->gender == GENDER_MALE ? "male" : "female");
    cJSON_AddNumberToObject(root, "age", input->age);
    cJSON_AddNumberToObject(root, "height_cm", input->height_cm);
    cJSON_AddNumberToObject(root, "neck_cm", input->neck_cm);
    cJSON_AddNumberToObject(root, "waist_cm", input->waist_cm);
    if (input->gender == GENDER_FEMALE) {
        cJSON_AddNumberToObject(root, "hip_cm", input->hip_cm);
    }

    cJSON_AddNumberToObject(root, "body_fat_percentage", output->body_fat_percentage);
    cJSON_AddNumberToObject(root, "fat_mass_kg", output->fat_mass_kg);
    cJSON_AddNumberToObject(root, "lean_mass_kg", output->lean_mass_kg);

    char *json_str = cJSON_Print(root);
    if (json_str) {
        write_json_to_file(json_str);
        free(json_str);
    }

    cJSON_Delete(root);
}

bool calculate_body_composition(const body_input_t *input, body_composition_t *output) {
    if (!input || !output) return false;

    float body_fat = 0.0f;
    float weight_kg = 70.0f; // Replace with HX711 readout
    float fat_mass, lean_mass;

    if (input->gender == GENDER_MALE) {
        body_fat = 495.0f / (1.0324f - 0.19077f * LOG10(input->waist_cm - input->neck_cm) +
                             0.15456f * LOG10(input->height_cm)) - 450.0f;
    } else {
        if (input->hip_cm <= 0) return false;
        body_fat = 495.0f / (1.29579f - 0.35004f * LOG10(input->waist_cm + input->hip_cm - input->neck_cm) +
                             0.22100f * LOG10(input->height_cm)) - 450.0f;
    }

    fat_mass = (body_fat / 100.0f) * weight_kg;
    lean_mass = weight_kg - fat_mass;

    output->body_fat_percentage = body_fat;
    output->fat_mass_kg = fat_mass;
    output->lean_mass_kg = lean_mass;

    save_body_composition_json(input, output);
    return true;
}
