#define UNITY_LONG_WIDTH 64

#include "unity.h"
#include "mock_twihs.h"
#include "logger.h"
#include "DS3231M.h"

void setUp(void)
{

}

void tearDown(void)
{

}

void test_init(void)
{
    // Expected calls
    twihs_probe_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);

    ds3231m_t ds3231m = {.address = DS3231_DEFAULT_ADDRESS};

    // Test init
    uint8_t result = DS3231M_init(&ds3231m);
    TEST_ASSERT_EQUAL_UINT8(TWIHS_SUCCESS, result);
}

void test_get_time(void)
{
    uint8_t buffer[]= {
        0x39, // seconds
        0x18, // minutes
        0x05, // hours
        0x01, // day of the week
        0x12, // date
        0x02, // month
        0x19  // year
        };
    twihs_packet_t packet_rx = {
    .buffer = (uint8_t *)buffer,
    };
    ds3231m_t ds3231m;

    // Expected calls
    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ReturnThruPtr_p_packet(&packet_rx);

    // Execute function
    DS3231M_get_time(&ds3231m);

    // Verify that return was correct
    TEST_ASSERT_EQUAL_UINT8(39, ds3231m.second);
    TEST_ASSERT_EQUAL_UINT8(18, ds3231m.minute);
    TEST_ASSERT_EQUAL_UINT8(5, ds3231m.hour);
    TEST_ASSERT_EQUAL_UINT8(1, ds3231m.day_of_week);
    TEST_ASSERT_EQUAL_UINT8(12, ds3231m.date);
    TEST_ASSERT_EQUAL_UINT8(2, ds3231m.month);
    TEST_ASSERT_EQUAL_UINT16(2019, ds3231m.year);
}

void test_get_temperature(void)
{
    uint8_t buffer_temperature[]= {0x19, 0x40}; // 25.25°
    uint8_t status_register= 0x80; // 0x84 has BSY bit to 1 

    twihs_packet_t packet_status = {
    .buffer = &status_register,
    };
    twihs_packet_t packet_temperature = {
    .buffer = (uint8_t *)buffer_temperature,
    };
    ds3231m_t ds3231m;

    // Expected calls
    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ReturnThruPtr_p_packet(&packet_status);

    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ReturnThruPtr_p_packet(&packet_temperature);
    

    // Execute function
    float temperature = 0;
    uint32_t result = DS3231M_get_temperature(&ds3231m, &temperature);

    // Verify that return was correct
    TEST_ASSERT_EQUAL_FLOAT(25.25, temperature);
    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_set_time(void)
{
    ds3231m_t ds3231m = {
        .second = 39,
        .minute = 18,
        .hour = 5,
        .day_of_week = 1,
        .date = 12,
        .month = 2,
        .year = 2019
    };

    // Expected calls
    twihs_master_write_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_write_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);

    // Execute function
    DS3231M_set_time(&ds3231m);

}

void test_conversion_dateTime_and_unix()
{
    ds3231m_t ds3231m = {
        .second = 39,
        .minute = 18,
        .hour = 5,
        .day_of_week = 1,
        .date = 12,
        .month = 2,
        .year = 2019
    };
    ds3231m_t ds3231m_received = {0};

    uint64_t timestamp = convert_dateTime_to_unixms(&ds3231m);
    TEST_ASSERT_EQUAL_UINT64(1549948719000, timestamp);

    convert_unixms_to_dateTime(timestamp, &ds3231m_received);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.second, ds3231m_received.second);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.minute, ds3231m_received.minute);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.hour, ds3231m_received.hour);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.date, ds3231m_received.date);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.month, ds3231m_received.month);
    TEST_ASSERT_EQUAL_UINT16(ds3231m.year, ds3231m_received.year);
}

void test_conversion_unsigned_float()
{
    uint8_t buffer_positive[2] = {0x19, 0x40}; // 25.25 (101 * 0.25)
    uint8_t buffer_zero[2] = {0x00, 0x00};
    uint8_t buffer_negative[2] = {0xE6, 0xC0}; // -25.25 (101 * 0.25)
    uint8_t buffer_negative2[2] = {0xFF, 0xC0}; // -0.25    = 1111111111
    uint8_t buffer_negative3[2] = {0x80, 0x00}; // -128     = 1000000000

    float result = convert_temperature_unsigned_to_float(buffer_positive);
    TEST_ASSERT_EQUAL_FLOAT(25.25, result);

    result = convert_temperature_unsigned_to_float(buffer_zero);
    TEST_ASSERT_EQUAL_FLOAT(0, result);

    result = convert_temperature_unsigned_to_float(buffer_negative);
    TEST_ASSERT_EQUAL_FLOAT(-25.25, result);

    result = convert_temperature_unsigned_to_float(buffer_negative2);
    TEST_ASSERT_EQUAL_FLOAT(-0.25, result);

    result = convert_temperature_unsigned_to_float(buffer_negative3);
    TEST_ASSERT_EQUAL_FLOAT(-128, result);
}