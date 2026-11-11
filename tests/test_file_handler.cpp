/**
 * @file test_file_handler.cpp
 * @brief Unit tests for the file_handler module (UT-36 to UT-39).
 *
 * Tests use their own files in tests/ and delete them afterwards,
 * so the real data/ files are never touched.
 */
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

extern "C" {
#include "file_handler.h"
#include "parking_lot.h"
}

static const char *TEST_SLOTS_FILE    = "tests/tmp_slots.txt";
static const char *TEST_HISTORY_FILE  = "tests/tmp_history.txt";
static const char *TEST_MISSING_FILE  = "tests/tmp_does_not_exist.txt";
static const time_t ENTRY_TIME = 1794394000; /* fixed time in the past */

class FileHandlerTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        std::remove(TEST_SLOTS_FILE);
        std::remove(TEST_HISTORY_FILE);
    }
};

/* UT-36 */
TEST_F(FileHandlerTest, SaveThenLoadSlotsGivesSameData)
{
    ParkingLot_t saved;
    int index = -1;
    ASSERT_EQ(STATUS_OK, initParkingLot(&saved, PLACE_OFFICE));
    ASSERT_EQ(STATUS_OK, allocateSlot(&saved, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
    ASSERT_EQ(STATUS_OK, allocateSlot(&saved, "KA01Z9999", VEHICLE_TWO_WHEELER, ENTRY_TIME + 60, &index));
    ASSERT_EQ(STATUS_OK, saveSlots(TEST_SLOTS_FILE, &saved));

    ParkingLot_t loaded;
    ASSERT_EQ(STATUS_OK, initParkingLot(&loaded, PLACE_OFFICE));
    EXPECT_EQ(STATUS_OK, loadSlots(TEST_SLOTS_FILE, &loaded));

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        EXPECT_STREQ(saved.slots[i].slotId, loaded.slots[i].slotId);
        EXPECT_EQ(saved.slots[i].status, loaded.slots[i].status);
        EXPECT_STREQ(saved.slots[i].vehicleNumber, loaded.slots[i].vehicleNumber);
        EXPECT_EQ(saved.slots[i].entryTime, loaded.slots[i].entryTime);
    }
}

/* UT-37 */
TEST_F(FileHandlerTest, LoadReportsMissingFile)
{
    PlaceType_t place = PLACE_MALL;
    EXPECT_EQ(STATUS_FILE_ERROR, loadSettings(TEST_MISSING_FILE, &place));

    ParkingLot_t lot;
    ASSERT_EQ(STATUS_OK, initParkingLot(&lot, PLACE_MALL));
    EXPECT_EQ(STATUS_FILE_ERROR, loadSlots(TEST_MISSING_FILE, &lot));
}

/* UT-38 */
TEST_F(FileHandlerTest, LoadRejectsDamagedSlotsFile)
{
    ParkingLot_t lot;
    ASSERT_EQ(STATUS_OK, initParkingLot(&lot, PLACE_MALL));
    ASSERT_EQ(STATUS_OK, saveSlots(TEST_SLOTS_FILE, &lot));

    /* Damage the file: change the status of the first spot to 7. */
    FILE *file = std::fopen(TEST_SLOTS_FILE, "r+");
    ASSERT_NE(nullptr, file);
    char header[128];
    ASSERT_NE(nullptr, std::fgets(header, sizeof(header), file));
    ASSERT_EQ(0, std::fseek(file, (long)std::strlen(header) + 7, SEEK_SET)); /* "2W-01,0," */
    ASSERT_EQ('7', std::fputc('7', file));
    std::fclose(file);

    EXPECT_EQ(STATUS_INVALID_FILE_DATA, loadSlots(TEST_SLOTS_FILE, &lot));
    EXPECT_TRUE(isLotEmpty(&lot));
}

/* UT-39 */
TEST_F(FileHandlerTest, AppendThenLoadHistory)
{
    ParkingRecord_t first = {};
    std::strcpy(first.vehicleNumber, "MH12AB1234");
    first.vehicleType = VEHICLE_FOUR_WHEELER;
    std::strcpy(first.slotId, "4W-01");
    first.entryTime = ENTRY_TIME;
    first.exitTime = ENTRY_TIME + 200 * 60;
    first.durationMinutes = 200;
    first.feeRupees = 80;

    ParkingRecord_t second = first;
    std::strcpy(second.vehicleNumber, "KA01Z9999");
    second.vehicleType = VEHICLE_TWO_WHEELER;
    std::strcpy(second.slotId, "2W-01");
    second.feeRupees = 30;

    ASSERT_EQ(STATUS_OK, appendHistoryRecord(TEST_HISTORY_FILE, &first));
    ASSERT_EQ(STATUS_OK, appendHistoryRecord(TEST_HISTORY_FILE, &second));

    ParkingRecord_t records[5];
    int count = 0;
    int skipped = -1;
    EXPECT_EQ(STATUS_OK, loadHistory(TEST_HISTORY_FILE, records, 5, &count, &skipped));
    EXPECT_EQ(2, count);
    EXPECT_EQ(0, skipped);
    EXPECT_STREQ("MH12AB1234", records[0].vehicleNumber);
    EXPECT_EQ(80, records[0].feeRupees);
    EXPECT_EQ(200, records[0].durationMinutes);
    EXPECT_STREQ("KA01Z9999", records[1].vehicleNumber);
    EXPECT_EQ(VEHICLE_TWO_WHEELER, records[1].vehicleType);
    EXPECT_EQ(30, records[1].feeRupees);
}
