/**
 * @file validation.h
 * @brief Checks and cleans everything the user types.
 */
#ifndef VALIDATION_H
#define VALIDATION_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Cleans a typed vehicle number: lowercase -> uppercase, spaces and dashes removed.
 * @param input      text typed by the user
 * @param output     buffer for the cleaned number
 * @param outputSize size of output (VEHICLE_NUMBER_LEN or more)
 * @return STATUS_OK, STATUS_NULL_POINTER, or STATUS_INVALID_INPUT (empty / too long)
 */
Status_t normalizeVehicleNumber(const char *input, char *output, size_t outputSize);

/**
 * Checks the Indian format: 2 letters + 2 digits + 1-2 letters + 4 digits.
 * The number must already be cleaned (uppercase, no spaces).
 * @return true if the format is correct, false otherwise (also for NULL)
 */
bool isValidVehicleNumber(const char *vehicleNumber);

/**
 * Converts typed text to a whole number between minChoice and maxChoice.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t parseMenuChoice(const char *input, int minChoice, int maxChoice, int *choice);

/**
 * Reads a yes/no answer. Only "Y", "y", "N" or "n" are accepted.
 * @param answer set to true for yes, false for no
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t parseYesNo(const char *input, bool *answer);

/**
 * Checks that the PIN is exactly PIN_LEN digits and matches MANAGER_PIN.
 * @return true if correct, false otherwise (also for NULL)
 */
bool isValidManagerPin(const char *pin);

#ifdef __cplusplus
}
#endif

#endif /* VALIDATION_H */
