/**
 * @file file_handler.c
 * @brief Saves and loads settings, spots and history using text files.
 *
 * File formats (see docs/detailed_design.md, section 10):
 *   settings.txt : PLACE=<0..3>
 *   slots.txt    : heading line, then slotId,vehicleType,status,vehicleNumber,entryTime
 *   history.txt  : heading line, then
 *                  vehicleNumber,vehicleType,slotId,entryTime,exitTime,durationMinutes,feeRupees
 */
#include "file_handler.h"
#include "parking_lot.h"
#include "validation.h"

#include <stdio.h>
#include <string.h>

#define SLOTS_HEADER    "slotId,vehicleType,status,vehicleNumber,entryTime"
#define HISTORY_HEADER  "vehicleNumber,vehicleType,slotId,entryTime,exitTime,durationMinutes,feeRupees"
#define EMPTY_VEHICLE   "-"
#define TEMP_SUFFIX     ".tmp"
#define MAX_FILE_NAME   256

/* Line-read results. */
typedef enum
{
    LINE_OK,
    LINE_END,
    LINE_TOO_LONG
} LineResult_t;

/* Reads one line and removes the newline at the end. */
static LineResult_t readFileLine(FILE *file, char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, file) == NULL)
    {
        return LINE_END;
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
    {
        buffer[length - 1] = '\0';
        length--;
        if (length > 0 && buffer[length - 1] == '\r')
        {
            buffer[length - 1] = '\0'; /* file saved with Windows line endings */
        }
        return LINE_OK;
    }

    if (!feof(file))
    {
        /* Line did not fit: throw away the rest of it. */
        int c;
        while ((c = fgetc(file)) != '\n' && c != EOF)
        {
        }
        return LINE_TOO_LONG;
    }
    return LINE_OK; /* last line without a newline */
}

/* Closes a file that was written and reports whether everything was saved. */
static Status_t closeWrittenFile(FILE *file, bool writeOk)
{
    if (ferror(file))
    {
        writeOk = false;
    }
    if (fclose(file) != 0)
    {
        writeOk = false;
    }
    return writeOk ? STATUS_OK : STATUS_FILE_ERROR;
}

Status_t loadSettings(const char *fileName, PlaceType_t *placeType)
{
    if (fileName == NULL || placeType == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        return STATUS_FILE_ERROR;
    }

    char line[MAX_LINE_LEN];
    LineResult_t result = readFileLine(file, line, sizeof(line));
    (void)fclose(file); /* file was only read; nothing to lose */

    if (result != LINE_OK)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    int value = -1;
    int used = 0;
    if (sscanf(line, "PLACE=%d%n", &value, &used) != 1 || line[used] != '\0'
        || value < PLACE_MALL || value >= PLACE_TYPE_COUNT)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    *placeType = (PlaceType_t)value;
    return STATUS_OK;
}

Status_t saveSettings(const char *fileName, PlaceType_t placeType)
{
    if (fileName == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (placeType < PLACE_MALL || placeType >= PLACE_TYPE_COUNT)
    {
        return STATUS_INVALID_INPUT;
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL)
    {
        return STATUS_FILE_ERROR;
    }
    bool writeOk = fprintf(file, "PLACE=%d\n", (int)placeType) > 0;
    return closeWrittenFile(file, writeOk);
}

Status_t saveSlots(const char *fileName, const ParkingLot_t *lot)
{
    if (fileName == NULL || lot == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    char tempName[MAX_FILE_NAME];
    int nameLength = snprintf(tempName, sizeof(tempName), "%s%s", fileName, TEMP_SUFFIX);
    if (nameLength < 0 || (size_t)nameLength >= sizeof(tempName))
    {
        return STATUS_FILE_ERROR;
    }

    FILE *file = fopen(tempName, "w");
    if (file == NULL)
    {
        return STATUS_FILE_ERROR;
    }

    bool writeOk = fprintf(file, "%s\n", SLOTS_HEADER) > 0;
    for (int i = 0; i < TOTAL_SLOTS && writeOk; i++)
    {
        const ParkingSlot_t *slot = &lot->slots[i];
        const char *vehicle = (slot->status == SLOT_FILLED) ? slot->vehicleNumber : EMPTY_VEHICLE;
        writeOk = fprintf(file, "%s,%d,%d,%s,%lld\n", slot->slotId, (int)slot->vehicleType,
                          (int)slot->status, vehicle, (long long)slot->entryTime) > 0;
    }

    if (closeWrittenFile(file, writeOk) != STATUS_OK)
    {
        (void)remove(tempName);
        return STATUS_FILE_ERROR;
    }
    if (rename(tempName, fileName) != 0)
    {
        (void)remove(tempName);
        return STATUS_FILE_ERROR;
    }
    return STATUS_OK;
}

/* Checks one slots.txt line and copies it into the expected slot. */
static Status_t parseSlotLine(const char *line, ParkingLot_t *lot, int index, time_t now)
{
    char slotId[SLOT_ID_LEN] = "";
    char vehicle[VEHICLE_NUMBER_LEN] = "";
    int type = -1;
    int status = -1;
    long long entry = -1;
    int used = 0;

    if (sscanf(line, "%5[^,],%d,%d,%10[^,],%lld%n", slotId, &type, &status, vehicle,
               &entry, &used) != 5 || line[used] != '\0')
    {
        return STATUS_INVALID_FILE_DATA;
    }

    ParkingSlot_t *slot = &lot->slots[index];
    if (strcmp(slotId, slot->slotId) != 0 || type != (int)slot->vehicleType)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    if (status == SLOT_EMPTY)
    {
        if (strcmp(vehicle, EMPTY_VEHICLE) != 0 || entry != 0)
        {
            return STATUS_INVALID_FILE_DATA;
        }
        return STATUS_OK; /* slot is already empty after initParkingLot() */
    }

    if (status != SLOT_FILLED || !isValidVehicleNumber(vehicle) || entry <= 0
        || (time_t)entry > now)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    int otherIndex = 0;
    if (findVehicleSlot(lot, vehicle, &otherIndex) == STATUS_OK)
    {
        return STATUS_INVALID_FILE_DATA; /* same vehicle in two spots */
    }

    slot->status = SLOT_FILLED;
    (void)snprintf(slot->vehicleNumber, sizeof(slot->vehicleNumber), "%s", vehicle);
    slot->entryTime = (time_t)entry;
    return STATUS_OK;
}

/* Returns true if the line has only spaces. */
static bool isBlankLine(const char *line)
{
    for (; *line != '\0'; line++)
    {
        if (*line != ' ' && *line != '\t')
        {
            return false;
        }
    }
    return true;
}

Status_t loadSlots(const char *fileName, ParkingLot_t *lot)
{
    if (fileName == NULL || lot == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    PlaceType_t placeType = lot->placeType;
    ParkingLot_t loaded;
    if (initParkingLot(&loaded, placeType) != STATUS_OK)
    {
        return STATUS_INVALID_INPUT;
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        (void)initParkingLot(lot, placeType);
        return STATUS_FILE_ERROR;
    }

    char line[MAX_LINE_LEN];
    Status_t status = STATUS_OK;
    time_t now = time(NULL);

    if (readFileLine(file, line, sizeof(line)) != LINE_OK || strcmp(line, SLOTS_HEADER) != 0)
    {
        status = STATUS_INVALID_FILE_DATA;
    }

    for (int i = 0; i < TOTAL_SLOTS && status == STATUS_OK; i++)
    {
        if (readFileLine(file, line, sizeof(line)) != LINE_OK)
        {
            status = STATUS_INVALID_FILE_DATA; /* too few lines or a line too long */
        }
        else
        {
            status = parseSlotLine(line, &loaded, i, now);
        }
    }

    /* Anything after the 50 spot lines (other than blank lines) means the file is damaged. */
    LineResult_t extra;
    while (status == STATUS_OK && (extra = readFileLine(file, line, sizeof(line))) != LINE_END)
    {
        if (extra != LINE_OK || !isBlankLine(line))
        {
            status = STATUS_INVALID_FILE_DATA;
        }
    }
    (void)fclose(file);

    if (status != STATUS_OK)
    {
        (void)initParkingLot(lot, placeType);
        return status;
    }
    *lot = loaded;
    return STATUS_OK;
}

Status_t appendHistoryRecord(const char *fileName, const ParkingRecord_t *record)
{
    if (fileName == NULL || record == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    FILE *file = fopen(fileName, "a");
    if (file == NULL)
    {
        return STATUS_FILE_ERROR;
    }

    bool writeOk = true;
    if (fseek(file, 0, SEEK_END) != 0)
    {
        writeOk = false;
    }
    else if (ftell(file) == 0)
    {
        writeOk = fprintf(file, "%s\n", HISTORY_HEADER) > 0; /* new file: add heading */
    }

    if (writeOk)
    {
        writeOk = fprintf(file, "%s,%d,%s,%lld,%lld,%ld,%d\n", record->vehicleNumber,
                          (int)record->vehicleType, record->slotId,
                          (long long)record->entryTime, (long long)record->exitTime,
                          record->durationMinutes, record->feeRupees) > 0;
    }
    return closeWrittenFile(file, writeOk);
}

/* Checks one history line and fills record. Returns false if the line is damaged. */
static bool parseHistoryLine(const char *line, ParkingRecord_t *record)
{
    int type = -1;
    long long entry = -1;
    long long exitValue = -1;
    int used = 0;

    if (sscanf(line, "%10[^,],%d,%5[^,],%lld,%lld,%ld,%d%n", record->vehicleNumber, &type,
               record->slotId, &entry, &exitValue, &record->durationMinutes,
               &record->feeRupees, &used) != 7 || line[used] != '\0')
    {
        return false;
    }
    if (!isValidVehicleNumber(record->vehicleNumber) || type < VEHICLE_TWO_WHEELER
        || type >= VEHICLE_TYPE_COUNT || entry <= 0 || exitValue < entry
        || record->durationMinutes < 0 || record->feeRupees < 0)
    {
        return false;
    }

    record->vehicleType = (VehicleType_t)type;
    record->entryTime = (time_t)entry;
    record->exitTime = (time_t)exitValue;
    return true;
}

Status_t loadHistory(const char *fileName, ParkingRecord_t records[], int maxRecords,
                     int *count, int *skippedLines)
{
    if (fileName == NULL || records == NULL || count == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (maxRecords <= 0)
    {
        return STATUS_INVALID_INPUT;
    }

    *count = 0;
    int skipped = 0;
    bool tooMany = false;

    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        if (skippedLines != NULL)
        {
            *skippedLines = 0;
        }
        return STATUS_OK; /* no history yet */
    }

    char line[MAX_LINE_LEN];
    LineResult_t result;
    bool firstLine = true;

    while ((result = readFileLine(file, line, sizeof(line))) != LINE_END)
    {
        if (firstLine)
        {
            firstLine = false;
            if (result == LINE_OK && strcmp(line, HISTORY_HEADER) == 0)
            {
                continue; /* heading line */
            }
        }
        if (result == LINE_OK && isBlankLine(line))
        {
            continue;
        }

        ParkingRecord_t record;
        if (result != LINE_OK || !parseHistoryLine(line, &record))
        {
            skipped++;
            continue;
        }

        if (*count == maxRecords)
        {
            /* Array full: drop the oldest record to keep the most recent ones. */
            memmove(&records[0], &records[1], (size_t)(maxRecords - 1) * sizeof(records[0]));
            records[maxRecords - 1] = record;
            tooMany = true;
        }
        else
        {
            records[*count] = record;
            (*count)++;
        }
    }
    (void)fclose(file);

    if (skippedLines != NULL)
    {
        *skippedLines = skipped;
    }
    return tooMany ? STATUS_HISTORY_FULL : STATUS_OK;
}
