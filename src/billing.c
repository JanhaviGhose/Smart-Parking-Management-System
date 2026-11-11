/**
 * @file billing.c
 * @brief Fee rules for each place, time parked and fee calculation.
 *
 * Fee rules (see docs/detailed_design.md, section 7.3):
 *   1. Within the free minutes the fee is 0.
 *   2. Otherwise charge the first charge (covers firstBlockHours).
 *   3. Every extra hour, or part of an hour, adds extraHourFee.
 *   4. The fee for one day never exceeds dailyMaxFee.
 *   5. Each full day costs dailyMaxFee; leftover time uses rules 1-4.
 */
#include "billing.h"

/* Fee table. Amounts in rupees, arrays are {two-wheeler, four-wheeler}. */
static const FeeRule_t FEE_RULES[PLACE_TYPE_COUNT] =
{
    /* Mall: cheap long stays so people shop longer */
    { 15, 2, { 20, 40 }, { 10, 20 }, {  80, 200 }, 12 },
    /* Supermarket: short free time, price rises fast */
    { 30, 1, { 10, 30 }, { 20, 50 }, { 150, 400 },  4 },
    /* Office: fair visitor rate, flag all-day parking */
    { 10, 1, { 10, 30 }, { 15, 40 }, { 100, 300 }, 10 },
    /* Event / pandal: flat charge for the event, costly after */
    {  0, 4, { 30, 60 }, { 30, 60 }, { 300, 600 },  6 }
};

static bool isValidPlace(PlaceType_t placeType)
{
    return placeType >= PLACE_MALL && placeType < PLACE_TYPE_COUNT;
}

static bool isValidVehicleType(VehicleType_t vehicleType)
{
    return vehicleType >= VEHICLE_TWO_WHEELER && vehicleType < VEHICLE_TYPE_COUNT;
}

/* Fee for a stay of at most one day (rules 1-4). */
static int calculateOneDayFee(const FeeRule_t *rule, VehicleType_t vehicleType, long minutes)
{
    if (minutes <= rule->freeMinutes)
    {
        return 0;
    }

    long fee = rule->firstBlockFee[vehicleType];
    long firstBlockMinutes = (long)rule->firstBlockHours * MINUTES_PER_HOUR;

    if (minutes > firstBlockMinutes)
    {
        long extraMinutes = minutes - firstBlockMinutes;
        long extraHours = (extraMinutes + MINUTES_PER_HOUR - 1) / MINUTES_PER_HOUR; /* round up */
        fee += extraHours * rule->extraHourFee[vehicleType];
    }

    if (fee > rule->dailyMaxFee[vehicleType])
    {
        fee = rule->dailyMaxFee[vehicleType];
    }
    return (int)fee;
}

Status_t getFeeRule(PlaceType_t placeType, FeeRule_t *rule)
{
    if (rule == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidPlace(placeType))
    {
        return STATUS_INVALID_INPUT;
    }
    *rule = FEE_RULES[placeType];
    return STATUS_OK;
}

Status_t calculateDurationMinutes(time_t entryTime, time_t exitTime, long *minutes)
{
    if (minutes == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    double seconds = difftime(exitTime, entryTime);
    if (seconds < 0.0)
    {
        return STATUS_INVALID_INPUT; /* exit before entry */
    }

    long wholeSeconds = (long)seconds;
    *minutes = (wholeSeconds + SECONDS_PER_MINUTE - 1) / SECONDS_PER_MINUTE; /* round up */
    return STATUS_OK;
}

Status_t calculateFee(PlaceType_t placeType, VehicleType_t vehicleType,
                      long durationMinutes, int *feeRupees)
{
    if (feeRupees == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!isValidPlace(placeType) || !isValidVehicleType(vehicleType) || durationMinutes < 0)
    {
        return STATUS_INVALID_INPUT;
    }

    const FeeRule_t *rule = &FEE_RULES[placeType];
    long fullDays = durationMinutes / MINUTES_PER_DAY;
    long leftoverMinutes = durationMinutes % MINUTES_PER_DAY;

    long fee = fullDays * rule->dailyMaxFee[vehicleType]
             + calculateOneDayFee(rule, vehicleType, leftoverMinutes);

    *feeRupees = (int)fee;
    return STATUS_OK;
}

bool isOverstay(PlaceType_t placeType, long durationMinutes)
{
    if (!isValidPlace(placeType) || durationMinutes < 0)
    {
        return false;
    }
    long limitMinutes = (long)FEE_RULES[placeType].overstayLimitHours * MINUTES_PER_HOUR;
    return durationMinutes > limitMinutes;
}

const char *getPlaceName(PlaceType_t placeType)
{
    switch (placeType)
    {
        case PLACE_MALL:        return "Mall";
        case PLACE_SUPERMARKET: return "Supermarket";
        case PLACE_OFFICE:      return "Office";
        case PLACE_EVENT:       return "Event / Pandal";
        default:                return "Unknown";
    }
}

const char *getVehicleTypeName(VehicleType_t vehicleType)
{
    switch (vehicleType)
    {
        case VEHICLE_TWO_WHEELER:  return "Two-wheeler";
        case VEHICLE_FOUR_WHEELER: return "Four-wheeler";
        default:                   return "Unknown";
    }
}
