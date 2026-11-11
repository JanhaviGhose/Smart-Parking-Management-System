/**
 * @file file_handler.h
 * @brief Saves and loads settings, spots and history using text files.
 *
 * Every function takes the file name as input, so tests can use their own
 * test files and never touch the real data files.
 */
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Loads the chosen place from the settings file.
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_FILE_ERROR (missing: first run)
 *         or STATUS_INVALID_FILE_DATA
 */
Status_t loadSettings(const char *fileName, PlaceType_t *placeType);

/**
 * Saves the chosen place to the settings file.
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_INVALID_INPUT or STATUS_FILE_ERROR
 */
Status_t saveSettings(const char *fileName, PlaceType_t placeType);

/**
 * Saves all spots, one line per spot. Writes to "<fileName>.tmp" first and then
 * renames it, so a crash while saving cannot damage the old file.
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_FILE_ERROR
 */
Status_t saveSlots(const char *fileName, const ParkingLot_t *lot);

/**
 * Loads all spots into lot. lot must already be set up with initParkingLot().
 * If loading fails, lot is left as a fresh empty lot.
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_FILE_ERROR or STATUS_INVALID_FILE_DATA
 */
Status_t loadSlots(const char *fileName, ParkingLot_t *lot);

/**
 * Adds one finished visit at the end of the history file (creates it if needed).
 * @return STATUS_OK, STATUS_NULL_POINTER or STATUS_FILE_ERROR
 */
Status_t appendHistoryRecord(const char *fileName, const ParkingRecord_t *record);

/**
 * Loads finished visits. A missing file means 0 records. Damaged lines are skipped.
 * If there are more than maxRecords, only the most recent maxRecords are kept.
 * @param count        set to the number of records loaded
 * @param skippedLines set to the number of damaged lines skipped (may be NULL)
 * @return STATUS_OK, STATUS_NULL_POINTER, STATUS_INVALID_INPUT or STATUS_HISTORY_FULL
 */
Status_t loadHistory(const char *fileName, ParkingRecord_t records[], int maxRecords,
                     int *count, int *skippedLines);

#ifdef __cplusplus
}
#endif

#endif /* FILE_HANDLER_H */
