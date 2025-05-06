#ifndef BODY_COMPOSITION_H
#define BODY_COMPOSITION_H

#include <stdbool.h>

typedef enum {
    GENDER_MALE,
    GENDER_FEMALE
} gender_t;

typedef struct {
    gender_t gender;
    int age;            // in years
    float height_cm;    // in cm
    float neck_cm;      // in cm
    float waist_cm;     // in cm
    float hip_cm;       // Only for females
} body_input_t;

typedef struct {
    float body_fat_percentage;
    float fat_mass_kg;
    float lean_mass_kg;
} body_composition_t;

bool calculate_body_composition(const body_input_t *input, body_composition_t *output);

#endif // BODY_COMPOSITION_H
