/**
 * @file test_report.cpp
 * @brief Unit tests for the report module and overstay check (UT-40 to UT-42).
 */
#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "billing.h"
#include "parking_lot.h"
#include "report.h"
}

/* Builds a fixed local time so tests never depend on the real clock. */
static time_t makeTime(int year, int month, int day, int hour, int minute)
{
    struct tm value = {};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_min = minute;
    value.tm_isdst = -1;
    return mktime(&value);
}

/* Builds one finished visit that started at `entry`. */
static ParkingRecord_t makeRecord(VehicleType_t type, time_t entry, long minutes, int fee)
{
    ParkingRecord_t record = {};
    std::strcpy(record.vehicleNumber, "MH12AB1234");
    std::strcpy(record.slotId, type == VEHICLE_TWO_WHEELER ? "2W-01" : "4W-01");
    record.vehicleType = type;
    record.entryTime = entry;
    record.exitTime = entry + minutes * 60;
    record.durationMinutes = minutes;
    record.feeRupees = fee;
    return record;
}

/* UT-40 */
TEST(ReportTest, DailySummaryTotals)
{
    ParkingLot_t lot;
    ASSERT_EQ(STATUS_OK, initParkingLot(&lot, PLACE_MALL));

    time_t today = makeTime(2026, 11, 11, 12, 0);
    ParkingRecord_t records[4] = {
        makeRecord(VEHICLE_TWO_WHEELER,  makeTime(2026, 11, 11, 9, 0), 30, 10),
        makeRecord(VEHICLE_FOUR_WHEELER, makeTime(2026, 11, 11, 10, 0), 90, 40),
        makeRecord(VEHICLE_FOUR_WHEELER, makeTime(2026, 11, 11, 11, 0), 180, 80),
        makeRecord(VEHICLE_FOUR_WHEELER, makeTime(2026, 11, 10, 9, 0), 60, 40) /* yesterday */
    };

    DailySummary_t summary;
    EXPECT_EQ(STATUS_OK, calculateDailySummary(records, 4, &lot, today, &summary));
    EXPECT_EQ(3, summary.exits);
    EXPECT_EQ(130, summary.totalRevenue);
    EXPECT_EQ(10, summary.revenueByType[VEHICLE_TWO_WHEELER]);
    EXPECT_EQ(120, summary.revenueByType[VEHICLE_FOUR_WHEELER]);
    EXPECT_DOUBLE_EQ(100.0, summary.averageMinutes);
}

/* UT-41 */
TEST(ReportTest, DailySummaryWithNoRecords)
{
    ParkingLot_t lot;
    ASSERT_EQ(STATUS_OK, initParkingLot(&lot, PLACE_MALL));

    DailySummary_t summary;
    EXPECT_EQ(STATUS_OK, calculateDailySummary(NULL, 0, &lot, makeTime(2026, 11, 11, 12, 0), &summary));
    EXPECT_EQ(0, summary.entries);
    EXPECT_EQ(0, summary.exits);
    EXPECT_EQ(0, summary.totalRevenue);
    EXPECT_DOUBLE_EQ(0.0, summary.averageMinutes);
}

/* UT-42 */
TEST(ReportTest, OverstayAtLimit)
{
    EXPECT_FALSE(isOverstay(PLACE_MALL, 720)); /* exactly 12 hours */
    EXPECT_TRUE(isOverstay(PLACE_MALL, 721));
}
