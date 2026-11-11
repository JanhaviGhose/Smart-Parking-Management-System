/**
 * @file test_billing.cpp
 * @brief Unit tests for the billing module (UT-14 to UT-25).
 */
#include <gtest/gtest.h>

extern "C" {
#include "billing.h"
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

/* UT-14 */
TEST(BillingTest, FreeWithinGracePeriod)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_MALL, VEHICLE_FOUR_WHEELER, 10, &fee));
    EXPECT_EQ(0, fee);
}

/* UT-15 */
TEST(BillingTest, FreeAtExactGraceLimit)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_SUPERMARKET, VEHICLE_TWO_WHEELER, 30, &fee));
    EXPECT_EQ(0, fee);
}

/* UT-16 */
TEST(BillingTest, ChargedOneMinuteAfterGrace)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_SUPERMARKET, VEHICLE_TWO_WHEELER, 31, &fee));
    EXPECT_EQ(10, fee);
}

/* UT-17 */
TEST(BillingTest, FirstChargeAtEndOfFirstBlock)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_MALL, VEHICLE_FOUR_WHEELER, 120, &fee));
    EXPECT_EQ(40, fee);
}

/* UT-18 */
TEST(BillingTest, ExtraHourChargedForPartHour)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_MALL, VEHICLE_FOUR_WHEELER, 121, &fee));
    EXPECT_EQ(60, fee);
}

/* UT-19 */
TEST(BillingTest, FeeLimitedToDailyMaximum)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_SUPERMARKET, VEHICLE_FOUR_WHEELER, 600, &fee));
    EXPECT_EQ(400, fee); /* 30 + 9 x 50 = 480, limited to 400 */
}

/* UT-20 */
TEST(BillingTest, MultiDayStay)
{
    int fee = -1;
    EXPECT_EQ(STATUS_OK, calculateFee(PLACE_MALL, VEHICLE_FOUR_WHEELER, 1500, &fee));
    EXPECT_EQ(240, fee); /* 1 day x 200 + 40 for the leftover hour */
}

/* UT-21 */
TEST(BillingTest, RejectsNegativeDuration)
{
    int fee = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, calculateFee(PLACE_MALL, VEHICLE_FOUR_WHEELER, -5, &fee));
}

/* UT-22 */
TEST(BillingTest, RejectsInvalidPlace)
{
    int fee = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, calculateFee(PLACE_TYPE_COUNT, VEHICLE_FOUR_WHEELER, 60, &fee));
}

/* UT-23 */
TEST(BillingTest, OneHourIsSixtyMinutes)
{
    time_t entry = makeTime(2026, 11, 11, 10, 0);
    long minutes = -1;
    EXPECT_EQ(STATUS_OK, calculateDurationMinutes(entry, entry + 3600, &minutes));
    EXPECT_EQ(60, minutes);
}

/* UT-24 */
TEST(BillingTest, WorksAcrossMidnight)
{
    time_t entry = makeTime(2026, 11, 11, 23, 30);
    time_t exitTime = makeTime(2026, 11, 12, 0, 45);
    long minutes = -1;
    EXPECT_EQ(STATUS_OK, calculateDurationMinutes(entry, exitTime, &minutes));
    EXPECT_EQ(75, minutes);
}

/* UT-25 */
TEST(BillingTest, RejectsExitBeforeEntry)
{
    time_t entry = makeTime(2026, 11, 11, 10, 0);
    long minutes = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, calculateDurationMinutes(entry, entry - 60, &minutes));
}
