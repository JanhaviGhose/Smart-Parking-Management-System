/**
 * @file parking_lot.c
 * @brief Manages the parking spots: give, find, free, count, change place.
 */
#include "parking_lot.h"
#include "validation.h"

#include <stdio.h>
#include <string.h>

static bool isValidPlace(PlaceType_t placeType)
{
    return placeType >= PLACE_MALL && placeType < PLACE_TYPE_COUNT;
}

static bool isValidVehicleType(VehicleType_t vehicleType)
{
    return vehicleType >= VEHICLE_TWO_WHEELER && vehicleType < VEHICLE_TYPE_COUNT;
}

/* Makes one spot empty. */
static void clearSlot(ParkingSlot_t *slot)
{
    slot->status = SLOT_EMPTY;
    slot->vehicleNumber[0] = '\0';
    slot->entryTime = 0;
}

Status_t initParkingLot(ParkingLot_t *lot, PlaceType_t placeType)
{
    if (lot == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidPlace(placeType))
    {
        return STATUS_INVALID_INPUT;
    }

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        ParkingSlot_t *slot = &lot->slots[i];
        if (i < TWO_WHEELER_SLOTS)
        {
            slot->vehicleType = VEHICLE_TWO_WHEELER;
            (void)snprintf(slot->slotId, sizeof(slot->slotId), "2W-%02d", i + 1);
        }
        else
        {
            slot->vehicleType = VEHICLE_FOUR_WHEELER;
            (void)snprintf(slot->slotId, sizeof(slot->slotId), "4W-%02d",
                           i - TWO_WHEELER_SLOTS + 1);
        }
        clearSlot(slot);
    }
    lot->placeType = placeType;
    return STATUS_OK;
}

Status_t findVehicleSlot(const ParkingLot_t *lot, const char *vehicleNumber, int *slotIndex)
{
    if (lot == NULL || vehicleNumber == NULL || slotIndex == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        const ParkingSlot_t *slot = &lot->slots[i];
        if (slot->status == SLOT_FILLED && strcmp(slot->vehicleNumber, vehicleNumber) == 0)
        {
            *slotIndex = i;
            return STATUS_OK;
        }
    }
    return STATUS_NOT_FOUND;
}

Status_t allocateSlot(ParkingLot_t *lot, const char *vehicleNumber,
                      VehicleType_t vehicleType, time_t entryTime, int *slotIndex)
{
    if (lot == NULL || vehicleNumber == NULL || slotIndex == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidVehicleNumber(vehicleNumber) || !isValidVehicleType(vehicleType))
    {
        return STATUS_INVALID_INPUT;
    }

    int existingIndex = 0;
    if (findVehicleSlot(lot, vehicleNumber, &existingIndex) == STATUS_OK)
    {
        return STATUS_DUPLICATE_VEHICLE;
    }

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        ParkingSlot_t *slot = &lot->slots[i];
        if (slot->vehicleType == vehicleType && slot->status == SLOT_EMPTY)
        {
            slot->status = SLOT_FILLED;
            (void)snprintf(slot->vehicleNumber, sizeof(slot->vehicleNumber), "%s", vehicleNumber);
            slot->entryTime = entryTime;
            *slotIndex = i;
            return STATUS_OK;
        }
    }
    return STATUS_LOT_FULL;
}

Status_t releaseSlot(ParkingLot_t *lot, int slotIndex, time_t exitTime,
                     long durationMinutes, int feeRupees, ParkingRecord_t *record)
{
    if (lot == NULL || record == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (slotIndex < 0 || slotIndex >= TOTAL_SLOTS)
    {
        return STATUS_INVALID_INPUT;
    }

    ParkingSlot_t *slot = &lot->slots[slotIndex];
    if (slot->status != SLOT_FILLED || durationMinutes < 0 || feeRupees < 0
        || difftime(exitTime, slot->entryTime) < 0.0)
    {
        return STATUS_INVALID_INPUT;
    }

    (void)snprintf(record->vehicleNumber, sizeof(record->vehicleNumber), "%s", slot->vehicleNumber);
    (void)snprintf(record->slotId, sizeof(record->slotId), "%s", slot->slotId);
    record->vehicleType = slot->vehicleType;
    record->entryTime = slot->entryTime;
    record->exitTime = exitTime;
    record->durationMinutes = durationMinutes;
    record->feeRupees = feeRupees;

    clearSlot(slot);
    return STATUS_OK;
}

Status_t countSlots(const ParkingLot_t *lot, VehicleType_t vehicleType,
                    int *freeCount, int *filledCount)
{
    if (lot == NULL || freeCount == NULL || filledCount == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidVehicleType(vehicleType))
    {
        return STATUS_INVALID_INPUT;
    }

    *freeCount = 0;
    *filledCount = 0;
    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        if (lot->slots[i].vehicleType != vehicleType)
        {
            continue;
        }
        if (lot->slots[i].status == SLOT_FILLED)
        {
            (*filledCount)++;
        }
        else
        {
            (*freeCount)++;
        }
    }
    return STATUS_OK;
}

bool isLotEmpty(const ParkingLot_t *lot)
{
    if (lot == NULL)
    {
        return false;
    }
    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        if (lot->slots[i].status == SLOT_FILLED)
        {
            return false;
        }
    }
    return true;
}

Status_t changePlaceType(ParkingLot_t *lot, PlaceType_t newPlaceType)
{
    if (lot == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidPlace(newPlaceType))
    {
        return STATUS_INVALID_INPUT;
    }
    if (!isLotEmpty(lot))
    {
        return STATUS_LOT_NOT_EMPTY;
    }
    lot->placeType = newPlaceType;
    return STATUS_OK;
}
