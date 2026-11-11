/**
 * @file test_parking_lot.cpp
 * @brief Unit tests for the parking_lot module (UT-26 to UT-35).
 */
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

extern "C" {
#include "parking_lot.h"
}

static const time_t ENTRY_TIME = 1794394000; /* fixed time, not the real clock */

/* Each test starts with a fresh, empty Mall parking lot. */
class ParkingLotTest : public ::testing::Test
{
protected:
    ParkingLot_t lot;

    void SetUp() override
    {
        ASSERT_EQ(STATUS_OK, initParkingLot(&lot, PLACE_MALL));
    }
};

/* UT-26 */
TEST_F(ParkingLotTest, AllocatesFirstEmptySlotOfType)
{
    int index = -1;
    EXPECT_EQ(STATUS_OK, allocateSlot(&lot, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
    EXPECT_EQ(TWO_WHEELER_SLOTS, index);
    EXPECT_STREQ("4W-01", lot.slots[index].slotId);
    EXPECT_EQ(SLOT_FILLED, lot.slots[index].status);
    EXPECT_STREQ("MH12AB1234", lot.slots[index].vehicleNumber);
    EXPECT_EQ(ENTRY_TIME, lot.slots[index].entryTime);
}

/* UT-27 */
TEST_F(ParkingLotTest, RejectsDuplicateVehicle)
{
    int index = -1;
    ASSERT_EQ(STATUS_OK, allocateSlot(&lot, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
    EXPECT_EQ(STATUS_DUPLICATE_VEHICLE,
              allocateSlot(&lot, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
}

/* UT-28 */
TEST_F(ParkingLotTest, ReportsLotFullForType)
{
    int index = -1;
    char number[VEHICLE_NUMBER_LEN];
    for (int i = 0; i < TWO_WHEELER_SLOTS; i++)
    {
        std::snprintf(number, sizeof(number), "MH12AB%04d", i + 1);
        ASSERT_EQ(STATUS_OK, allocateSlot(&lot, number, VEHICLE_TWO_WHEELER, ENTRY_TIME, &index));
    }
    EXPECT_EQ(STATUS_LOT_FULL, allocateSlot(&lot, "MH12CD0001", VEHICLE_TWO_WHEELER, ENTRY_TIME, &index));
    EXPECT_EQ(STATUS_OK, allocateSlot(&lot, "MH12CD0002", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
}

/* UT-29 */
TEST_F(ParkingLotTest, RejectsInvalidVehicleNumber)
{
    int index = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, allocateSlot(&lot, "ABC", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
    EXPECT_TRUE(isLotEmpty(&lot));
}

/* UT-30 */
TEST_F(ParkingLotTest, FindsParkedVehicle)
{
    int index = -1;
    ASSERT_EQ(STATUS_OK, allocateSlot(&lot, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));
    int found = -1;
    EXPECT_EQ(STATUS_OK, findVehicleSlot(&lot, "MH12AB1234", &found));
    EXPECT_EQ(TWO_WHEELER_SLOTS, found);
}

/* UT-31 */
TEST_F(ParkingLotTest, ReportsVehicleNotFound)
{
    int found = -1;
    EXPECT_EQ(STATUS_NOT_FOUND, findVehicleSlot(&lot, "MH12AB1234", &found));
}

/* UT-32 */
TEST_F(ParkingLotTest, ReleaseFreesSlotAndFillsRecord)
{
    int index = -1;
    ASSERT_EQ(STATUS_OK, allocateSlot(&lot, "MH12AB1234", VEHICLE_FOUR_WHEELER, ENTRY_TIME, &index));

    ParkingRecord_t record;
    time_t exitTime = ENTRY_TIME + 200 * 60;
    EXPECT_EQ(STATUS_OK, releaseSlot(&lot, index, exitTime, 200, 80, &record));
    EXPECT_EQ(SLOT_EMPTY, lot.slots[index].status);
    EXPECT_STREQ("MH12AB1234", record.vehicleNumber);
    EXPECT_STREQ("4W-01", record.slotId);
    EXPECT_EQ(VEHICLE_FOUR_WHEELER, record.vehicleType);
    EXPECT_EQ(ENTRY_TIME, record.entryTime);
    EXPECT_EQ(exitTime, record.exitTime);
    EXPECT_EQ(200, record.durationMinutes);
    EXPECT_EQ(80, record.feeRupees);
}

/* UT-33 */
TEST_F(ParkingLotTest, ReleaseRejectsEmptyOrInvalidSlot)
{
    ParkingRecord_t record;
    EXPECT_EQ(STATUS_INVALID_INPUT, releaseSlot(&lot, 0, ENTRY_TIME, 10, 0, &record));
    EXPECT_EQ(STATUS_INVALID_INPUT, releaseSlot(&lot, TOTAL_SLOTS, ENTRY_TIME, 10, 0, &record));
}

/* UT-34 */
TEST_F(ParkingLotTest, ChangesPlaceWhenLotEmpty)
{
    EXPECT_EQ(STATUS_OK, changePlaceType(&lot, PLACE_EVENT));
    EXPECT_EQ(PLACE_EVENT, lot.placeType);
}

/* UT-35 */
TEST_F(ParkingLotTest, RefusesPlaceChangeWhenNotEmpty)
{
    int index = -1;
    ASSERT_EQ(STATUS_OK, allocateSlot(&lot, "MH12AB1234", VEHICLE_TWO_WHEELER, ENTRY_TIME, &index));
    EXPECT_EQ(STATUS_LOT_NOT_EMPTY, changePlaceType(&lot, PLACE_EVENT));
    EXPECT_EQ(PLACE_MALL, lot.placeType);
}
