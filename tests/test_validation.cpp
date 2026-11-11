/**
 * @file test_validation.cpp
 * @brief Unit tests for the validation module (UT-01 to UT-13).
 */
#include <gtest/gtest.h>

extern "C" {
#include "validation.h"
}

/* UT-01 */
TEST(ValidationTest, AcceptsTenCharacterNumber)
{
    EXPECT_TRUE(isValidVehicleNumber("MH12AB1234"));
}

/* UT-02 */
TEST(ValidationTest, AcceptsNineCharacterNumber)
{
    EXPECT_TRUE(isValidVehicleNumber("MH12A1234"));
}

/* UT-03 */
TEST(ValidationTest, RejectsWrongLength)
{
    EXPECT_FALSE(isValidVehicleNumber("MH12A123"));    /* 8 characters  */
    EXPECT_FALSE(isValidVehicleNumber("MH12AB12345")); /* 11 characters */
}

/* UT-04 */
TEST(ValidationTest, RejectsWrongPattern)
{
    EXPECT_FALSE(isValidVehicleNumber("12MHAB1234"));
    EXPECT_FALSE(isValidVehicleNumber("MH1AB12345"));
    EXPECT_FALSE(isValidVehicleNumber("MH12@B1234"));
}

/* UT-05 */
TEST(ValidationTest, RejectsNullAndEmpty)
{
    EXPECT_FALSE(isValidVehicleNumber(NULL));
    EXPECT_FALSE(isValidVehicleNumber(""));
}

/* UT-06 */
TEST(ValidationTest, NormalizesCaseSpacesAndDashes)
{
    char output[VEHICLE_NUMBER_LEN];
    EXPECT_EQ(STATUS_OK, normalizeVehicleNumber("mh 12-ab 1234", output, sizeof(output)));
    EXPECT_STREQ("MH12AB1234", output);
}

/* UT-07 */
TEST(ValidationTest, NormalizeRejectsSmallOutputBuffer)
{
    char output[5];
    EXPECT_EQ(STATUS_INVALID_INPUT, normalizeVehicleNumber("MH12AB1234", output, sizeof(output)));
}

/* UT-08 */
TEST(ValidationTest, AcceptsChoicesAtBothEnds)
{
    int choice = -1;
    EXPECT_EQ(STATUS_OK, parseMenuChoice("0", 0, 8, &choice));
    EXPECT_EQ(0, choice);
    EXPECT_EQ(STATUS_OK, parseMenuChoice("8", 0, 8, &choice));
    EXPECT_EQ(8, choice);
}

/* UT-09 */
TEST(ValidationTest, RejectsChoicesOutsideRange)
{
    int choice = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("9", 0, 8, &choice));
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("-1", 0, 8, &choice));
}

/* UT-10 */
TEST(ValidationTest, RejectsNonNumbers)
{
    int choice = -1;
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("abc", 0, 8, &choice));
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("3x", 0, 8, &choice));
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("2.5", 0, 8, &choice));
    EXPECT_EQ(STATUS_INVALID_INPUT, parseMenuChoice("", 0, 8, &choice));
}

/* UT-11 */
TEST(ValidationTest, AcceptsCorrectPin)
{
    EXPECT_TRUE(isValidManagerPin(MANAGER_PIN));
}

/* UT-12 */
TEST(ValidationTest, RejectsWrongPin)
{
    EXPECT_FALSE(isValidManagerPin("1234"));
}

/* UT-13 */
TEST(ValidationTest, RejectsPinWithWrongFormat)
{
    EXPECT_FALSE(isValidManagerPin("432"));
    EXPECT_FALSE(isValidManagerPin("43210"));
    EXPECT_FALSE(isValidManagerPin("abcd"));
    EXPECT_FALSE(isValidManagerPin(NULL));
}
