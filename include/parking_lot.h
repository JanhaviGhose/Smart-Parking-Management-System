/**
 * @file parking_lot.h
 * @brief Manages the parking spots: give, find, free, count, change place.
 */
#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets up a fresh lot: 20 empty two-wheeler spots ("2W-01".."2W-20")
 * and 30 empty four-wheeler spots ("4W-01".."4W-30").
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT (bad place)
 */
Status_t initParkingLot(ParkingLot_t *lot, PlaceType_t placeType);

/**
 * Gives the first empty spot of the right type and records the entry.
 * @param slotIndex set to the position of the spot given
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_INVALID_INPUT,
 *         STATUS_DUPLICATE_VEHICLE or STATUS_LOT_FULL
 */
Status_t allocateSlot(ParkingLot_t *lot, const char *vehicleNumber,
                      VehicleType_t vehicleType, time_t entryTime, int *slotIndex);

/**
 * Finds the spot where a vehicle is parked.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_NOT_FOUND
 */
Status_t findVehicleSlot(const ParkingLot_t *lot, const char *vehicleNumber, int *slotIndex);

/**
 * Fills in the finished visit record and makes the spot empty.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t releaseSlot(ParkingLot_t *lot, int slotIndex, time_t exitTime,
                     long durationMinutes, int feeRupees, ParkingRecord_t *record);

/**
 * Counts empty and filled spots of one vehicle type.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t countSlots(const ParkingLot_t *lot, VehicleType_t vehicleType,
                    int *freeCount, int *filledCount);

/** @return true if no vehicle is parked (false for NULL). */
bool isLotEmpty(const ParkingLot_t *lot);

/**
 * Changes the place type, only when no vehicle is parked.
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_INVALID_INPUT or STATUS_LOT_NOT_EMPTY
 */
Status_t changePlaceType(ParkingLot_t *lot, PlaceType_t newPlaceType);

#ifdef __cplusplus
}
#endif

#endif /* PARKING_LOT_H */
