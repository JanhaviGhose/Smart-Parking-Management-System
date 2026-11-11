/**
 * @file validation.c
 * @brief Checks and cleans everything the user types.
 */
#include "validation.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define STATE_CODE_LEN    2
#define DISTRICT_CODE_LEN 2
#define SERIAL_DIGITS_LEN 4

/* Returns true if every character in text[start .. start+count-1] is an uppercase letter. */
static bool allUpperLetters(const char *text, size_t start, size_t count)
{
    for (size_t i = start; i < start + count; i++)
    {
        if (!isupper((unsigned char)text[i]))
        {
            return false;
        }
    }
    return true;
}

/* Returns true if every character in text[start .. start+count-1] is a digit. */
static bool allDigits(const char *text, size_t start, size_t count)
{
    for (size_t i = start; i < start + count; i++)
    {
        if (!isdigit((unsigned char)text[i]))
        {
            return false;
        }
    }
    return true;
}

/* Skips spaces at the start of text. */
static const char *skipSpaces(const char *text)
{
    while (*text != '\0' && isspace((unsigned char)*text))
    {
        text++;
    }
    return text;
}

Status_t normalizeVehicleNumber(const char *input, char *output, size_t outputSize)
{
    if (input == NULL || output == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (outputSize == 0)
    {
        return STATUS_INVALID_INPUT;
    }

    size_t length = 0;
    for (size_t i = 0; input[i] != '\0'; i++)
    {
        char c = input[i];
        if (c == ' ' || c == '-')
        {
            continue; /* spaces and dashes are allowed but removed */
        }
        if (length + 1 >= outputSize)
        {
            output[0] = '\0';
            return STATUS_INVALID_INPUT; /* too long for the output buffer */
        }
        output[length] = (char)toupper((unsigned char)c);
        length++;
    }
    output[length] = '\0';

    return (length == 0) ? STATUS_INVALID_INPUT : STATUS_OK;
}

bool isValidVehicleNumber(const char *vehicleNumber)
{
    if (vehicleNumber == NULL)
    {
        return false;
    }

    size_t length = strlen(vehicleNumber);
    if (length < VEHICLE_NUMBER_MIN_LEN || length > VEHICLE_NUMBER_MAX_LEN)
    {
        return false;
    }

    /* Series letters = whatever is left after state, district and serial number (1 or 2). */
    size_t seriesLen = length - STATE_CODE_LEN - DISTRICT_CODE_LEN - SERIAL_DIGITS_LEN;
    size_t seriesStart = STATE_CODE_LEN + DISTRICT_CODE_LEN;

    return allUpperLetters(vehicleNumber, 0, STATE_CODE_LEN)
        && allDigits(vehicleNumber, STATE_CODE_LEN, DISTRICT_CODE_LEN)
        && allUpperLetters(vehicleNumber, seriesStart, seriesLen)
        && allDigits(vehicleNumber, seriesStart + seriesLen, SERIAL_DIGITS_LEN);
}

Status_t parseMenuChoice(const char *input, int minChoice, int maxChoice, int *choice)
{
    if (input == NULL || choice == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (minChoice > maxChoice)
    {
        return STATUS_INVALID_INPUT;
    }

    const char *start = skipSpaces(input);
    if (*start == '\0')
    {
        return STATUS_INVALID_INPUT; /* empty input */
    }

    char *end = NULL;
    errno = 0;
    long value = strtol(start, &end, 10);
    if (end == start || errno == ERANGE)
    {
        return STATUS_INVALID_INPUT; /* not a number, or far too large */
    }
    if (*skipSpaces(end) != '\0')
    {
        return STATUS_INVALID_INPUT; /* extra characters such as "3x" or "2.5" */
    }
    if (value < minChoice || value > maxChoice)
    {
        return STATUS_INVALID_INPUT;
    }

    *choice = (int)value;
    return STATUS_OK;
}

Status_t parseYesNo(const char *input, bool *answer)
{
    if (input == NULL || answer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    const char *start = skipSpaces(input);
    if (*start == '\0' || *skipSpaces(start + 1) != '\0')
    {
        return STATUS_INVALID_INPUT; /* must be exactly one character */
    }

    char c = (char)toupper((unsigned char)*start);
    if (c == 'Y')
    {
        *answer = true;
        return STATUS_OK;
    }
    if (c == 'N')
    {
        *answer = false;
        return STATUS_OK;
    }
    return STATUS_INVALID_INPUT;
}

bool isValidManagerPin(const char *pin)
{
    if (pin == NULL)
    {
        return false;
    }
    if (strlen(pin) != PIN_LEN || !allDigits(pin, 0, PIN_LEN))
    {
        return false;
    }
    return strcmp(pin, MANAGER_PIN) == 0;
}
