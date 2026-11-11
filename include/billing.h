/**
 * @file billing.h
 * @brief Fee rules for each place, time parked and fee calculation.
 */
#ifndef BILLING_H
#define BILLING_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copies the fee rules of a place into rule.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT (bad place)
 */
Status_t getFeeRule(PlaceType_t placeType, FeeRule_t *rule);

/**
 * Works out whole minutes between entry and exit. Leftover seconds round up.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT (exit before entry)
 */
Status_t calculateDurationMinutes(time_t entryTime, time_t exitTime, long *minutes);

/**
 * Works out the parking fee using the rules of the chosen place.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t calculateFee(PlaceType_t placeType, VehicleType_t vehicleType,
                      long durationMinutes, int *feeRupees);

/**
 * @return true if durationMinutes is longer than the overstay limit of the place,
 *         false otherwise (also for bad input)
 */
bool isOverstay(PlaceType_t placeType, long durationMinutes);

/** @return a readable name such as "Mall", or "Unknown" for a bad value. */
const char *getPlaceName(PlaceType_t placeType);

/** @return "Two-wheeler", "Four-wheeler", or "Unknown" for a bad value. */
const char *getVehicleTypeName(VehicleType_t vehicleType);

#ifdef __cplusplus
}
#endif

#endif /* BILLING_H */
