/**
 * @file common.h
 * @brief Shared types, status codes and constants used by all modules.
 *
 * Every module includes this file, so all modules use the same
 * structures, enumerations and fixed values.
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Parking layout ---------- */
#define TWO_WHEELER_SLOTS   20
#define FOUR_WHEELER_SLOTS  30
#define TOTAL_SLOTS         (TWO_WHEELER_SLOTS + FOUR_WHEELER_SLOTS)

/* ---------- String sizes ---------- */
#define VEHICLE_NUMBER_MIN_LEN  9   /* e.g. MH12A1234  */
#define VEHICLE_NUMBER_MAX_LEN  10  /* e.g. MH12AB1234 */
#define VEHICLE_NUMBER_LEN      (VEHICLE_NUMBER_MAX_LEN + 1) /* + end mark */
#define SLOT_ID_LEN             6   /* "4W-30" + end mark */
#define MAX_INPUT_LEN           64  /* longest line the user may type */

/* ---------- Limits ---------- */
#define MAX_HISTORY_RECORDS     1000
#define MAX_LINE_LEN            128 /* longest line read from a data file */

/* ---------- Manager PIN (known limitation: fixed, not real security) ---------- */
#define MANAGER_PIN  "4321"
#define PIN_LEN      4

/* ---------- Time ---------- */
#define SECONDS_PER_MINUTE  60
#define MINUTES_PER_HOUR    60
#define MINUTES_PER_DAY     1440

/* ---------- Data files ---------- */
#define SETTINGS_FILE       "data/settings.txt"
#define SLOTS_FILE          "data/slots.txt"
#define HISTORY_FILE        "data/history.txt"
#define SLOTS_DAMAGED_FILE  "data/slots_damaged.txt"

/** Type of place the parking lot is for. */
typedef enum
{
    PLACE_MALL = 0,
    PLACE_SUPERMARKET,
    PLACE_OFFICE,
    PLACE_EVENT,
    PLACE_TYPE_COUNT /* number of places - not a real place */
} PlaceType_t;

/** Type of vehicle. */
typedef enum
{
    VEHICLE_TWO_WHEELER = 0,
    VEHICLE_FOUR_WHEELER,
    VEHICLE_TYPE_COUNT /* number of vehicle types - not a real type */
} VehicleType_t;

/** Whether a spot is in use. */
typedef enum
{
    SLOT_EMPTY = 0,
    SLOT_FILLED
} SlotStatus_t;

/** Result returned by functions that can fail. */
typedef enum
{
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_INVALID_INPUT,
    STATUS_DUPLICATE_VEHICLE,
    STATUS_LOT_FULL,
    STATUS_NOT_FOUND,
    STATUS_LOT_NOT_EMPTY,
    STATUS_ACCESS_DENIED,
    STATUS_HISTORY_FULL,
    STATUS_FILE_ERROR,
    STATUS_INVALID_FILE_DATA
} Status_t;

/** One parking spot. */
typedef struct
{
    char          slotId[SLOT_ID_LEN];               /* e.g. "2W-05"                  */
    VehicleType_t vehicleType;                       /* vehicles allowed in the spot  */
    SlotStatus_t  status;                            /* empty or filled               */
    char          vehicleNumber[VEHICLE_NUMBER_LEN]; /* "" when empty                 */
    time_t        entryTime;                         /* 0 when empty                  */
} ParkingSlot_t;

/** The whole parking lot: all spots and the chosen place. */
typedef struct
{
    ParkingSlot_t slots[TOTAL_SLOTS]; /* 0-19 two-wheeler, 20-49 four-wheeler */
    PlaceType_t   placeType;
} ParkingLot_t;

/** One finished visit (saved in the history file). */
typedef struct
{
    char          vehicleNumber[VEHICLE_NUMBER_LEN];
    VehicleType_t vehicleType;
    char          slotId[SLOT_ID_LEN];
    time_t        entryTime;
    time_t        exitTime;
    long          durationMinutes;
    int           feeRupees;
} ParkingRecord_t;

/** Fee rules for one place. Arrays are indexed by VehicleType_t. */
typedef struct
{
    int freeMinutes;
    int firstBlockHours;
    int firstBlockFee[VEHICLE_TYPE_COUNT];
    int extraHourFee[VEHICLE_TYPE_COUNT];
    int dailyMaxFee[VEHICLE_TYPE_COUNT];
    int overstayLimitHours;
} FeeRule_t;

/** Totals for the daily report. */
typedef struct
{
    int    entries;
    int    exits;
    int    totalRevenue;
    int    revenueByType[VEHICLE_TYPE_COUNT];
    double averageMinutes;
} DailySummary_t;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
