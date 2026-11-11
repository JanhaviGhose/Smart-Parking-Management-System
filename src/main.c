/**
 * @file main.c
 * @brief Entry point: start-up, main menu and high-level flow.
 *
 * main.c only talks to the user and calls the other modules.
 * All checking, calculations and file work are done in the modules.
 */
#include "billing.h"
#include "common.h"
#include "file_handler.h"
#include "parking_lot.h"
#include "report.h"
#include "validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MENU_MIN          0
#define MENU_MAX          8
#define PLACE_MENU_MIN    1
#define PLACE_MENU_MAX    PLACE_TYPE_COUNT
#define VEHICLE_MENU_MIN  1
#define VEHICLE_MENU_MAX  VEHICLE_TYPE_COUNT
#define TIME_TEXT_LEN     32

/* Main menu options. */
typedef enum
{
    MENU_EXIT = 0,
    MENU_VEHICLE_ENTRY,
    MENU_VEHICLE_EXIT,
    MENU_SLOT_STATUS,
    MENU_SEARCH_VEHICLE,
    MENU_VEHICLE_HISTORY,
    MENU_DAILY_REPORT,
    MENU_OVERSTAY_LIST,
    MENU_CHANGE_PLACE
} MenuOption_t;

/* Result of reading one line from the keyboard. */
typedef enum
{
    INPUT_OK,
    INPUT_TOO_LONG,
    INPUT_END /* input ended (e.g. Ctrl+D) */
} InputResult_t;

/* ---------- Input helpers ---------- */

/* Shows a prompt and reads one line safely with fgets(). */
static InputResult_t readLine(const char *prompt, char *buffer, size_t size)
{
    printf("%s", prompt);
    (void)fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        return INPUT_END;
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
    {
        buffer[length - 1] = '\0';
        return INPUT_OK;
    }
    if (feof(stdin))
    {
        return INPUT_OK; /* last line without newline */
    }

    /* Line was longer than the buffer: throw away the rest. */
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
    buffer[0] = '\0';
    return INPUT_TOO_LONG;
}

/* Keeps asking until the user types a number from minValue to maxValue. */
static InputResult_t askNumberInRange(const char *prompt, int minValue, int maxValue, int *value)
{
    char input[MAX_INPUT_LEN];
    while (true)
    {
        InputResult_t result = readLine(prompt, input, sizeof(input));
        if (result == INPUT_END)
        {
            return INPUT_END;
        }
        if (result == INPUT_OK && parseMenuChoice(input, minValue, maxValue, value) == STATUS_OK)
        {
            return INPUT_OK;
        }
        printf(" Invalid choice. Please enter a number from %d to %d.\n", minValue, maxValue);
    }
}

/* Keeps asking until the user types Y or N. */
static InputResult_t askYesNo(const char *prompt, bool *answer)
{
    char input[MAX_INPUT_LEN];
    while (true)
    {
        InputResult_t result = readLine(prompt, input, sizeof(input));
        if (result == INPUT_END)
        {
            return INPUT_END;
        }
        if (result == INPUT_OK && parseYesNo(input, answer) == STATUS_OK)
        {
            return INPUT_OK;
        }
        printf(" Please type Y or N.\n");
    }
}

/* Reads, cleans and checks a vehicle number. Returns INPUT_OK only if it is valid. */
static InputResult_t askVehicleNumber(char *vehicleNumber, size_t size, bool *isValid)
{
    char input[MAX_INPUT_LEN];
    *isValid = false;

    InputResult_t result = readLine(" Enter vehicle number (e.g. MH12AB1234): ", input, sizeof(input));
    if (result == INPUT_END)
    {
        return INPUT_END;
    }
    if (result == INPUT_OK
        && normalizeVehicleNumber(input, vehicleNumber, size) == STATUS_OK
        && isValidVehicleNumber(vehicleNumber))
    {
        *isValid = true;
    }
    else
    {
        printf(" Invalid vehicle number. Example: MH12AB1234 or MH12A1234.\n");
    }
    return INPUT_OK;
}

/* Asks for the manager PIN. Returns true only if it is correct. */
static bool askManagerPin(bool *inputEnded)
{
    char input[MAX_INPUT_LEN];
    *inputEnded = false;

    InputResult_t result = readLine(" Enter manager PIN: ", input, sizeof(input));
    if (result == INPUT_END)
    {
        *inputEnded = true;
        return false;
    }
    if (result == INPUT_OK && isValidManagerPin(input))
    {
        return true;
    }
    printf(" Access denied.\n");
    return false;
}

/* ---------- Messages ---------- */

/* Shows a general message for a status code. */
static void printStatusMessage(Status_t status)
{
    switch (status)
    {
        case STATUS_OK:                break;
        case STATUS_NULL_POINTER:      printf(" Internal error. Please try again.\n"); break;
        case STATUS_INVALID_INPUT:     printf(" Invalid input. Please try again.\n"); break;
        case STATUS_DUPLICATE_VEHICLE: printf(" This vehicle is already parked.\n"); break;
        case STATUS_LOT_FULL:          printf(" Sorry, no empty spot for this vehicle type.\n"); break;
        case STATUS_NOT_FOUND:         printf(" Vehicle not found.\n"); break;
        case STATUS_LOT_NOT_EMPTY:     printf(" Place can only be changed when the lot is empty.\n"); break;
        case STATUS_ACCESS_DENIED:     printf(" Access denied.\n"); break;
        case STATUS_HISTORY_FULL:      printf(" Note: only the latest %d records are used.\n",
                                              MAX_HISTORY_RECORDS); break;
        case STATUS_FILE_ERROR:        printf(" Could not open or save a data file.\n"); break;
        case STATUS_INVALID_FILE_DATA: printf(" A data file is damaged.\n"); break;
        default:                       printf(" Unknown error.\n"); break;
    }
}

/* Saves the spots and warns if saving failed. */
static void saveSlotsWithWarning(const ParkingLot_t *lot)
{
    if (saveSlots(SLOTS_FILE, lot) != STATUS_OK)
    {
        printf(" Warning: could not save spot data to %s.\n", SLOTS_FILE);
    }
}

/* Loads history and tells the user about skipped lines or too many records. */
static Status_t loadHistoryWithNotes(ParkingRecord_t records[], int *count)
{
    int skipped = 0;
    Status_t status = loadHistory(HISTORY_FILE, records, MAX_HISTORY_RECORDS, count, &skipped);
    if (status == STATUS_HISTORY_FULL)
    {
        printStatusMessage(status);
        status = STATUS_OK;
    }
    if (skipped > 0)
    {
        printf(" Note: %d damaged line(s) in %s were skipped.\n", skipped, HISTORY_FILE);
    }
    return status;
}

/* ---------- Start-up ---------- */

static InputResult_t choosePlace(PlaceType_t *placeType)
{
    printf("\n Choose the type of place this parking lot is for:\n");
    for (int i = 0; i < PLACE_TYPE_COUNT; i++)
    {
        printf("   %d. %s\n", i + 1, getPlaceName((PlaceType_t)i));
    }

    int choice = 0;
    InputResult_t result = askNumberInRange(" Your choice (1-4): ", PLACE_MENU_MIN, PLACE_MENU_MAX, &choice);
    if (result == INPUT_OK)
    {
        *placeType = (PlaceType_t)(choice - 1);
    }
    return result;
}

/* Loads settings and spots. Returns false if input ended during first-time setup. */
static bool startUp(ParkingLot_t *lot)
{
    PlaceType_t placeType = PLACE_MALL;
    Status_t status = loadSettings(SETTINGS_FILE, &placeType);

    if (status != STATUS_OK)
    {
        if (status == STATUS_INVALID_FILE_DATA)
        {
            printf(" Settings file is damaged. Please choose the place again.\n");
        }
        else
        {
            printf(" First-time setup (to be done by the manager / installer).\n");
        }
        if (choosePlace(&placeType) != INPUT_OK)
        {
            return false;
        }
        if (saveSettings(SETTINGS_FILE, placeType) != STATUS_OK)
        {
            printf(" Warning: could not save settings to %s.\n", SETTINGS_FILE);
        }
        printFeeRules(placeType);
    }

    if (initParkingLot(lot, placeType) != STATUS_OK)
    {
        return false;
    }

    status = loadSlots(SLOTS_FILE, lot);
    if (status == STATUS_FILE_ERROR)
    {
        printf(" No saved spot data found. Starting with all spots empty.\n");
        saveSlotsWithWarning(lot);
    }
    else if (status == STATUS_INVALID_FILE_DATA)
    {
        printf(" Spot data file is damaged. It was kept as %s.\n", SLOTS_DAMAGED_FILE);
        printf(" Starting with all spots empty.\n");
        if (rename(SLOTS_FILE, SLOTS_DAMAGED_FILE) != 0)
        {
            printf(" Warning: could not keep a copy of the damaged file.\n");
        }
        saveSlotsWithWarning(lot);
    }
    return true;
}

/* ---------- Menu actions ---------- */

static InputResult_t handleVehicleEntry(ParkingLot_t *lot)
{
    char vehicleNumber[VEHICLE_NUMBER_LEN];
    bool isValid = false;
    if (askVehicleNumber(vehicleNumber, sizeof(vehicleNumber), &isValid) == INPUT_END)
    {
        return INPUT_END;
    }
    if (!isValid)
    {
        return INPUT_OK;
    }

    int existingIndex = 0;
    if (findVehicleSlot(lot, vehicleNumber, &existingIndex) == STATUS_OK)
    {
        printf(" This vehicle is already parked in spot %s.\n", lot->slots[existingIndex].slotId);
        return INPUT_OK;
    }

    int typeChoice = 0;
    if (askNumberInRange(" Vehicle type (1 = Two-wheeler, 2 = Four-wheeler): ",
                         VEHICLE_MENU_MIN, VEHICLE_MENU_MAX, &typeChoice) == INPUT_END)
    {
        return INPUT_END;
    }
    VehicleType_t vehicleType = (VehicleType_t)(typeChoice - 1);

    int slotIndex = 0;
    time_t now = time(NULL);
    Status_t status = allocateSlot(lot, vehicleNumber, vehicleType, now, &slotIndex);
    if (status == STATUS_LOT_FULL)
    {
        printf(" Sorry, no empty %s spot.\n", getVehicleTypeName(vehicleType));
        return INPUT_OK;
    }
    if (status != STATUS_OK)
    {
        printStatusMessage(status);
        return INPUT_OK;
    }

    saveSlotsWithWarning(lot);
    char timeText[TIME_TEXT_LEN];
    formatTime(now, timeText, sizeof(timeText));
    printf(" Entry recorded. Please park in spot %s. (Entry time: %s)\n",
           lot->slots[slotIndex].slotId, timeText);
    return INPUT_OK;
}

static InputResult_t handleVehicleExit(ParkingLot_t *lot)
{
    char vehicleNumber[VEHICLE_NUMBER_LEN];
    bool isValid = false;
    if (askVehicleNumber(vehicleNumber, sizeof(vehicleNumber), &isValid) == INPUT_END)
    {
        return INPUT_END;
    }
    if (!isValid)
    {
        return INPUT_OK;
    }

    int slotIndex = 0;
    if (findVehicleSlot(lot, vehicleNumber, &slotIndex) != STATUS_OK)
    {
        printf(" This vehicle is not in the parking lot.\n");
        return INPUT_OK;
    }

    const ParkingSlot_t *slot = &lot->slots[slotIndex];
    time_t exitTime = time(NULL);
    long minutes = 0;
    int fee = 0;
    if (calculateDurationMinutes(slot->entryTime, exitTime, &minutes) != STATUS_OK
        || calculateFee(lot->placeType, slot->vehicleType, minutes, &fee) != STATUS_OK)
    {
        printf(" Could not work out the fee (check the computer clock).\n");
        return INPUT_OK;
    }

    printf(" Spot %s | Time parked: %ldh %02ldm | Fee: Rs. %d\n", slot->slotId,
           minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR, fee);

    bool paid = false;
    if (askYesNo(" Payment received? (Y/N): ", &paid) == INPUT_END)
    {
        return INPUT_END;
    }
    if (!paid)
    {
        printf(" Payment not received. Vehicle is still parked.\n");
        return INPUT_OK;
    }

    ParkingRecord_t record;
    Status_t status = releaseSlot(lot, slotIndex, exitTime, minutes, fee, &record);
    if (status != STATUS_OK)
    {
        printStatusMessage(status);
        return INPUT_OK;
    }

    if (appendHistoryRecord(HISTORY_FILE, &record) != STATUS_OK)
    {
        printf(" Warning: could not save this visit to %s.\n", HISTORY_FILE);
    }
    saveSlotsWithWarning(lot);
    printReceipt(&record);
    return INPUT_OK;
}

static InputResult_t handleSearchVehicle(const ParkingLot_t *lot)
{
    char vehicleNumber[VEHICLE_NUMBER_LEN];
    bool isValid = false;
    if (askVehicleNumber(vehicleNumber, sizeof(vehicleNumber), &isValid) == INPUT_END)
    {
        return INPUT_END;
    }
    if (!isValid)
    {
        return INPUT_OK;
    }

    int slotIndex = 0;
    if (findVehicleSlot(lot, vehicleNumber, &slotIndex) != STATUS_OK)
    {
        printf(" This vehicle is not in the parking lot.\n");
        return INPUT_OK;
    }

    const ParkingSlot_t *slot = &lot->slots[slotIndex];
    char timeText[TIME_TEXT_LEN];
    long minutes = 0;
    int feeSoFar = 0;
    formatTime(slot->entryTime, timeText, sizeof(timeText));
    printf(" %s is parked in spot %s since %s.\n", vehicleNumber, slot->slotId, timeText);
    if (calculateDurationMinutes(slot->entryTime, time(NULL), &minutes) == STATUS_OK
        && calculateFee(lot->placeType, slot->vehicleType, minutes, &feeSoFar) == STATUS_OK)
    {
        printf(" Time parked so far: %ldh %02ldm | Fee so far: Rs. %d\n",
               minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR, feeSoFar);
    }
    return INPUT_OK;
}

static InputResult_t handleVehicleHistory(void)
{
    char vehicleNumber[VEHICLE_NUMBER_LEN];
    bool isValid = false;
    if (askVehicleNumber(vehicleNumber, sizeof(vehicleNumber), &isValid) == INPUT_END)
    {
        return INPUT_END;
    }
    if (!isValid)
    {
        return INPUT_OK;
    }

    static ParkingRecord_t records[MAX_HISTORY_RECORDS]; /* static: too big for the stack */
    int count = 0;
    if (loadHistoryWithNotes(records, &count) != STATUS_OK)
    {
        printf(" Could not read %s.\n", HISTORY_FILE);
        return INPUT_OK;
    }
    if (printVehicleHistory(records, count, vehicleNumber) == STATUS_NOT_FOUND)
    {
        printf(" No past visits found for %s.\n", vehicleNumber);
    }
    return INPUT_OK;
}

static InputResult_t handleDailyReport(const ParkingLot_t *lot)
{
    bool inputEnded = false;
    if (!askManagerPin(&inputEnded))
    {
        return inputEnded ? INPUT_END : INPUT_OK;
    }

    static ParkingRecord_t records[MAX_HISTORY_RECORDS];
    int count = 0;
    if (loadHistoryWithNotes(records, &count) != STATUS_OK)
    {
        printf(" Could not read %s.\n", HISTORY_FILE);
        return INPUT_OK;
    }

    DailySummary_t summary;
    time_t now = time(NULL);
    Status_t status = calculateDailySummary(records, count, lot, now, &summary);
    if (status != STATUS_OK)
    {
        printStatusMessage(status);
        return INPUT_OK;
    }
    printDailyReport(&summary, now);
    return INPUT_OK;
}

static void handleOverstayList(const ParkingLot_t *lot)
{
    int slotIndexes[TOTAL_SLOTS];
    int found = 0;
    time_t now = time(NULL);
    Status_t status = findOverstayVehicles(lot, now, slotIndexes, TOTAL_SLOTS, &found);
    if (status != STATUS_OK)
    {
        printStatusMessage(status);
        return;
    }
    printOverstayList(lot, slotIndexes, found, now);
}

static InputResult_t handleChangePlace(ParkingLot_t *lot)
{
    bool inputEnded = false;
    if (!askManagerPin(&inputEnded))
    {
        return inputEnded ? INPUT_END : INPUT_OK;
    }

    if (!isLotEmpty(lot))
    {
        int freeTwo = 0, filledTwo = 0, freeFour = 0, filledFour = 0;
        (void)countSlots(lot, VEHICLE_TWO_WHEELER, &freeTwo, &filledTwo);
        (void)countSlots(lot, VEHICLE_FOUR_WHEELER, &freeFour, &filledFour);
        printf(" Place can only be changed when the lot is empty (%d vehicles parked).\n",
               filledTwo + filledFour);
        return INPUT_OK;
    }

    printf(" Current place: %s\n", getPlaceName(lot->placeType));
    PlaceType_t newPlace = PLACE_MALL;
    if (choosePlace(&newPlace) == INPUT_END)
    {
        return INPUT_END;
    }
    if (newPlace == lot->placeType)
    {
        printf(" This is already the current place.\n");
        return INPUT_OK;
    }

    Status_t status = changePlaceType(lot, newPlace);
    if (status != STATUS_OK)
    {
        printStatusMessage(status);
        return INPUT_OK;
    }
    if (saveSettings(SETTINGS_FILE, newPlace) != STATUS_OK)
    {
        printf(" Warning: could not save settings to %s.\n", SETTINGS_FILE);
    }
    printf(" Place changed to %s.\n", getPlaceName(newPlace));
    printFeeRules(newPlace);
    return INPUT_OK;
}

/* ---------- Main ---------- */

static void printMenu(const ParkingLot_t *lot)
{
    int freeTwo = 0, filledTwo = 0, freeFour = 0, filledFour = 0;
    (void)countSlots(lot, VEHICLE_TWO_WHEELER, &freeTwo, &filledTwo);
    (void)countSlots(lot, VEHICLE_FOUR_WHEELER, &freeFour, &filledFour);

    printf("\n==================== SMART PARKING ====================\n");
    printf(" Place: %s | Empty spots: 2W %d/%d, 4W %d/%d\n", getPlaceName(lot->placeType),
           freeTwo, TWO_WHEELER_SLOTS, freeFour, FOUR_WHEELER_SLOTS);
    printf("--------------------------------------------------------\n");
    printf(" 1. Vehicle Entry\n");
    printf(" 2. Vehicle Exit\n");
    printf(" 3. View Spot Status\n");
    printf(" 4. Search Parked Vehicle\n");
    printf(" 5. View Vehicle History\n");
    printf(" 6. Daily Report        (manager PIN)\n");
    printf(" 7. Overstay List\n");
    printf(" 8. Change Place Type   (manager PIN)\n");
    printf(" 0. Save and Exit\n");
    printf("========================================================\n");
}

int main(void)
{
    ParkingLot_t lot;

    printf("Smart Parking Management System with Slot Allocation and Billing\n");
    if (!startUp(&lot))
    {
        printf("\n Setup was not completed. Exiting.\n");
        return EXIT_FAILURE;
    }

    bool running = true;
    while (running)
    {
        printMenu(&lot);

        int choice = 0;
        if (askNumberInRange(" Enter your choice (0-8): ", MENU_MIN, MENU_MAX, &choice) == INPUT_END)
        {
            break;
        }

        InputResult_t result = INPUT_OK;
        switch ((MenuOption_t)choice)
        {
            case MENU_VEHICLE_ENTRY:   result = handleVehicleEntry(&lot); break;
            case MENU_VEHICLE_EXIT:    result = handleVehicleExit(&lot); break;
            case MENU_SLOT_STATUS:     printSlotStatus(&lot); break;
            case MENU_SEARCH_VEHICLE:  result = handleSearchVehicle(&lot); break;
            case MENU_VEHICLE_HISTORY: result = handleVehicleHistory(); break;
            case MENU_DAILY_REPORT:    result = handleDailyReport(&lot); break;
            case MENU_OVERSTAY_LIST:   handleOverstayList(&lot); break;
            case MENU_CHANGE_PLACE:    result = handleChangePlace(&lot); break;
            case MENU_EXIT:            running = false; break;
            default:                   printf(" Invalid choice.\n"); break;
        }
        if (result == INPUT_END)
        {
            running = false;
        }
    }

    saveSlotsWithWarning(&lot);
    printf("\n Data saved. Goodbye!\n");
    return EXIT_SUCCESS;
}
