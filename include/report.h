/**
 * @file report.h
 * @brief Daily totals, overstay list and all printed outputs.
 *
 * calculate/find functions only work things out (no printing).
 * print functions only print (no working out).
 */
#ifndef REPORT_H
#define REPORT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Works out the totals for the day that contains `day`.
 * Entries are counted from history and from vehicles still parked.
 * Exits, money and average time come from history records that exited that day.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t calculateDailySummary(const ParkingRecord_t records[], int recordCount,
                               const ParkingLot_t *lot, time_t day,
                               DailySummary_t *summary);

/**
 * Finds parked vehicles that have been parked longer than the place's limit.
 * @param slotIndexes array that receives the positions of those spots
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_INVALID_INPUT
 */
Status_t findOverstayVehicles(const ParkingLot_t *lot, time_t now, int slotIndexes[],
                              int maxCount, int *foundCount);

/** Shows every spot as empty or filled, with counts for each type. */
void printSlotStatus(const ParkingLot_t *lot);

/** Shows the receipt for a finished visit. */
void printReceipt(const ParkingRecord_t *record);

/**
 * Shows past visits of one vehicle.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_NOT_FOUND (no visits)
 */
Status_t printVehicleHistory(const ParkingRecord_t records[], int count,
                             const char *vehicleNumber);

/** Shows the daily report. */
void printDailyReport(const DailySummary_t *summary, time_t day);

/** Shows the vehicles parked too long. */
void printOverstayList(const ParkingLot_t *lot, const int slotIndexes[], int count, time_t now);

/** Shows the fee rules of a place. */
void printFeeRules(PlaceType_t placeType);

/** Formats a time as "dd-mm-yyyy hh:mm". */
void formatTime(time_t value, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* REPORT_H */
