/**
 * @file report.c
 * @brief Daily totals, overstay list and all printed outputs.
 */
#include "report.h"
#include "billing.h"
#include "parking_lot.h"

#include <stdio.h>
#include <string.h>

#define TIME_TEXT_LEN    32
#define SLOTS_PER_ROW    5
#define DIVIDER          "------------------------------------------------------------"

/* Returns true if both times fall on the same calendar day (local time). */
static bool isSameDay(time_t first, time_t second)
{
    struct tm firstDay;
    struct tm secondDay;
    const struct tm *temp = localtime(&first);
    if (temp == NULL)
    {
        return false;
    }
    firstDay = *temp; /* copy: localtime() reuses one shared buffer */

    temp = localtime(&second);
    if (temp == NULL)
    {
        return false;
    }
    secondDay = *temp;

    return firstDay.tm_year == secondDay.tm_year && firstDay.tm_yday == secondDay.tm_yday;
}

void formatTime(time_t value, char *buffer, size_t size)
{
    if (buffer == NULL || size == 0)
    {
        return;
    }
    const struct tm *local = localtime(&value);
    if (local == NULL || strftime(buffer, size, "%d-%m-%Y %H:%M", local) == 0)
    {
        (void)snprintf(buffer, size, "unknown");
    }
}

/* Prints minutes as "Xh Ym". */
static void printDuration(long minutes)
{
    printf("%ldh %02ldm", minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR);
}

Status_t calculateDailySummary(const ParkingRecord_t records[], int recordCount,
                               const ParkingLot_t *lot, time_t day,
                               DailySummary_t *summary)
{
    if (summary == NULL || lot == NULL || (records == NULL && recordCount > 0))
    {
        return STATUS_NULL_POINTER;
    }
    if (recordCount < 0)
    {
        return STATUS_INVALID_INPUT;
    }

    memset(summary, 0, sizeof(*summary));
    long totalMinutes = 0;

    for (int i = 0; i < recordCount; i++)
    {
        const ParkingRecord_t *record = &records[i];
        if (isSameDay(record->entryTime, day))
        {
            summary->entries++;
        }
        if (isSameDay(record->exitTime, day))
        {
            summary->exits++;
            summary->totalRevenue += record->feeRupees;
            if (record->vehicleType >= VEHICLE_TWO_WHEELER
                && record->vehicleType < VEHICLE_TYPE_COUNT)
            {
                summary->revenueByType[record->vehicleType] += record->feeRupees;
            }
            totalMinutes += record->durationMinutes;
        }
    }

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        const ParkingSlot_t *slot = &lot->slots[i];
        if (slot->status == SLOT_FILLED && isSameDay(slot->entryTime, day))
        {
            summary->entries++; /* entered today, still parked */
        }
    }

    summary->averageMinutes = (summary->exits > 0)
                            ? (double)totalMinutes / summary->exits
                            : 0.0;
    return STATUS_OK;
}

Status_t findOverstayVehicles(const ParkingLot_t *lot, time_t now, int slotIndexes[],
                              int maxCount, int *foundCount)
{
    if (lot == NULL || slotIndexes == NULL || foundCount == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (maxCount < 0)
    {
        return STATUS_INVALID_INPUT;
    }

    *foundCount = 0;
    for (int i = 0; i < TOTAL_SLOTS && *foundCount < maxCount; i++)
    {
        const ParkingSlot_t *slot = &lot->slots[i];
        long minutes = 0;
        if (slot->status == SLOT_FILLED
            && calculateDurationMinutes(slot->entryTime, now, &minutes) == STATUS_OK
            && isOverstay(lot->placeType, minutes))
        {
            slotIndexes[*foundCount] = i;
            (*foundCount)++;
        }
    }
    return STATUS_OK;
}

void printSlotStatus(const ParkingLot_t *lot)
{
    if (lot == NULL)
    {
        return;
    }

    printf("\n%s\n SPOT STATUS  (Place: %s)\n%s\n", DIVIDER, getPlaceName(lot->placeType), DIVIDER);
    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        const ParkingSlot_t *slot = &lot->slots[i];
        if (i == 0)
        {
            printf("Two-wheeler spots:\n");
        }
        else if (i == TWO_WHEELER_SLOTS)
        {
            printf("Four-wheeler spots:\n");
        }

        const char *shown = (slot->status == SLOT_FILLED) ? slot->vehicleNumber : "  empty";
        printf(" %s [%-10s]", slot->slotId, shown);

        bool rowEnd = ((i + 1) % SLOTS_PER_ROW == 0) || (i + 1 == TWO_WHEELER_SLOTS)
                   || (i + 1 == TOTAL_SLOTS);
        if (rowEnd)
        {
            printf("\n");
        }
    }

    for (int type = 0; type < VEHICLE_TYPE_COUNT; type++)
    {
        int freeCount = 0;
        int filledCount = 0;
        if (countSlots(lot, (VehicleType_t)type, &freeCount, &filledCount) == STATUS_OK)
        {
            printf("%-13s: %2d filled, %2d empty\n",
                   getVehicleTypeName((VehicleType_t)type), filledCount, freeCount);
        }
    }
    printf("%s\n", DIVIDER);
}

void printReceipt(const ParkingRecord_t *record)
{
    if (record == NULL)
    {
        return;
    }

    char entryText[TIME_TEXT_LEN];
    char exitText[TIME_TEXT_LEN];
    formatTime(record->entryTime, entryText, sizeof(entryText));
    formatTime(record->exitTime, exitText, sizeof(exitText));

    printf("\n%s\n PARKING RECEIPT\n%s\n", DIVIDER, DIVIDER);
    printf(" Vehicle number : %s\n", record->vehicleNumber);
    printf(" Vehicle type   : %s\n", getVehicleTypeName(record->vehicleType));
    printf(" Spot           : %s\n", record->slotId);
    printf(" Entry time     : %s\n", entryText);
    printf(" Exit time      : %s\n", exitText);
    printf(" Time parked    : ");
    printDuration(record->durationMinutes);
    printf("\n Fee paid       : Rs. %d\n%s\n", record->feeRupees, DIVIDER);
}

Status_t printVehicleHistory(const ParkingRecord_t records[], int count,
                             const char *vehicleNumber)
{
    if (vehicleNumber == NULL || (records == NULL && count > 0))
    {
        return STATUS_NULL_POINTER;
    }

    int found = 0;
    for (int i = 0; i < count; i++)
    {
        const ParkingRecord_t *record = &records[i];
        if (strcmp(record->vehicleNumber, vehicleNumber) != 0)
        {
            continue;
        }
        if (found == 0)
        {
            printf("\n%s\n PAST VISITS OF %s\n%s\n", DIVIDER, vehicleNumber, DIVIDER);
            printf(" %-5s %-16s %-16s %-8s %s\n", "Spot", "Entry", "Exit", "Time", "Fee");
        }
        found++;

        char entryText[TIME_TEXT_LEN];
        char exitText[TIME_TEXT_LEN];
        formatTime(record->entryTime, entryText, sizeof(entryText));
        formatTime(record->exitTime, exitText, sizeof(exitText));
        printf(" %-5s %-16s %-16s ", record->slotId, entryText, exitText);
        printDuration(record->durationMinutes);
        printf("  Rs. %d\n", record->feeRupees);
    }

    if (found == 0)
    {
        return STATUS_NOT_FOUND;
    }
    printf(" Total visits: %d\n%s\n", found, DIVIDER);
    return STATUS_OK;
}

void printDailyReport(const DailySummary_t *summary, time_t day)
{
    if (summary == NULL)
    {
        return;
    }

    char dayText[TIME_TEXT_LEN];
    const struct tm *local = localtime(&day);
    if (local == NULL || strftime(dayText, sizeof(dayText), "%d-%m-%Y", local) == 0)
    {
        (void)snprintf(dayText, sizeof(dayText), "today");
    }

    printf("\n%s\n DAILY REPORT  (%s)\n%s\n", DIVIDER, dayText, DIVIDER);
    printf(" Vehicles entered      : %d\n", summary->entries);
    printf(" Vehicles exited       : %d\n", summary->exits);
    printf(" Total money collected : Rs. %d\n", summary->totalRevenue);
    printf("   from two-wheelers   : Rs. %d\n", summary->revenueByType[VEHICLE_TWO_WHEELER]);
    printf("   from four-wheelers  : Rs. %d\n", summary->revenueByType[VEHICLE_FOUR_WHEELER]);
    printf(" Average time parked   : ");
    printDuration((long)(summary->averageMinutes + 0.5));
    printf("\n%s\n", DIVIDER);
}

void printOverstayList(const ParkingLot_t *lot, const int slotIndexes[], int count, time_t now)
{
    if (lot == NULL || (slotIndexes == NULL && count > 0))
    {
        return;
    }

    FeeRule_t rule;
    int limitHours = (getFeeRule(lot->placeType, &rule) == STATUS_OK) ? rule.overstayLimitHours : 0;

    printf("\n%s\n VEHICLES PARKED LONGER THAN %d HOURS\n%s\n", DIVIDER, limitHours, DIVIDER);
    if (count == 0)
    {
        printf(" No vehicles parked too long.\n%s\n", DIVIDER);
        return;
    }

    for (int i = 0; i < count; i++)
    {
        int index = slotIndexes[i];
        if (index < 0 || index >= TOTAL_SLOTS)
        {
            continue;
        }
        const ParkingSlot_t *slot = &lot->slots[index];
        char entryText[TIME_TEXT_LEN];
        long minutes = 0;
        formatTime(slot->entryTime, entryText, sizeof(entryText));
        (void)calculateDurationMinutes(slot->entryTime, now, &minutes);

        printf(" %s  %-10s  since %s  (", slot->slotId, slot->vehicleNumber, entryText);
        printDuration(minutes);
        printf(")\n");
    }
    printf("%s\n", DIVIDER);
}

void printFeeRules(PlaceType_t placeType)
{
    FeeRule_t rule;
    if (getFeeRule(placeType, &rule) != STATUS_OK)
    {
        return;
    }

    printf("\n Fee rules for %s:\n", getPlaceName(placeType));
    printf("   Free time          : %d minutes\n", rule.freeMinutes);
    printf("   First %d hour(s)    : Rs. %d (2W) / Rs. %d (4W)\n", rule.firstBlockHours,
           rule.firstBlockFee[VEHICLE_TWO_WHEELER], rule.firstBlockFee[VEHICLE_FOUR_WHEELER]);
    printf("   Each extra hour    : Rs. %d (2W) / Rs. %d (4W)\n",
           rule.extraHourFee[VEHICLE_TWO_WHEELER], rule.extraHourFee[VEHICLE_FOUR_WHEELER]);
    printf("   Maximum per day    : Rs. %d (2W) / Rs. %d (4W)\n",
           rule.dailyMaxFee[VEHICLE_TWO_WHEELER], rule.dailyMaxFee[VEHICLE_FOUR_WHEELER]);
    printf("   Parked too long at : more than %d hours\n", rule.overstayLimitHours);
}
