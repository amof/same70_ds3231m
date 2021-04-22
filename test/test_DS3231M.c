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

/**
 * @ingroup test_ds3231m
 * @brief Check if probe returns successfully; if initialization puts every time value to zero
 */
void test_init(void)
{
    // Expected calls
    twihs_probe_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);

    ds3231m_t ds3231m = {.address = DS3231_DEFAULT_ADDRESS,
                         .second = 25}; //change the second value to check if it gets reset to zero

    // Test init
    uint32_t result = DS3231M_init(&ds3231m);
    TEST_ASSERT_EQUAL_UINT32(TWIHS_SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(0, ds3231m.second);
}

/**
 * @ingroup test_ds3231m
 * @brief Create mock buffer packet and pass it to get_time, check if values match. Test if function behaves correctly after timeout.
 */
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

    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_TIMEOUT);
    uint32_t result = DS3231M_get_time(&ds3231m);
    TEST_ASSERT_EQUAL_UINT32(TWIHS_TIMEOUT, result);

}

/**
 * @ingroup test_ds3231m
 * @brief Create mock buffer and register, check if get_temperature behaves as expected and the result is correct. Also check results for no TWIHS success or register busy.
 */
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

    status_register= 0x84; //checking case if BSY bit = 1

    packet_status.buffer = &status_register;

    // Expected calls
    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ReturnThruPtr_p_packet(&packet_status);
    
    // Execute function
    temperature = 0;
    result = DS3231M_get_temperature(&ds3231m, &temperature);

    // Verify that return was correct
    TEST_ASSERT_EQUAL_FLOAT(0, temperature);
    TEST_ASSERT_EQUAL_UINT32(8, result);

    // Testing if no TWIHS success
    // Expected calls
    twihs_master_read_ExpectAnyArgsAndReturn(TWIHS_TIMEOUT);
    
    // Execute function
    temperature = 0;
    result = DS3231M_get_temperature(&ds3231m, &temperature);

    // Verify that return was correct
    TEST_ASSERT_EQUAL_FLOAT(0, temperature);
    TEST_ASSERT_EQUAL_UINT32(TWIHS_TIMEOUT, result);

}


/**
 * @ingroup test_ds3231m
 * @brief Check that it's possible to set a time on the RTC if TWIHS returns successfully
 */
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
    uint32_t result = DS3231M_set_time(&ds3231m);
    TEST_ASSERT_EQUAL_UINT32(TWIHS_SUCCESS, result);

}

/**
 * @ingroup test_ds3231m
 * @brief Check that function behaves as expected and returns correct error code when twihs unsuccessful
 */
void test_set_time_timeout(void)
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
    twihs_master_write_ExpectAnyArgsAndReturn(1); //test case not equal to zero
    // Execute function
    uint32_t result = DS3231M_set_time(&ds3231m);
    TEST_ASSERT_EQUAL_UINT32(1, result); 

    // Expected calls
    twihs_master_write_ExpectAnyArgsAndReturn(TWIHS_SUCCESS);
    twihs_master_read_ExpectAnyArgsAndReturn(1);

    // Execute function
    result = DS3231M_set_time(&ds3231m);
    TEST_ASSERT_EQUAL_UINT32(1, result);

}

/**
 * @ingroup test_ds3231m
 * @brief Check that datetime conversion between unixms timestamp and full date returns correct result in every case
 */
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

    convert_unixms_to_dateTime(0, &ds3231m_received);
    ds3231m_t ds3231m_zero = {0};
    convert_unixms_to_dateTime(0, &ds3231m_zero);
    //check whether test is necessary, unsigned int would never be negative

    TEST_ASSERT_EQUAL_UINT8(ds3231m_zero.second, ds3231m_received.second);
    TEST_ASSERT_EQUAL_UINT8(ds3231m_zero.minute, ds3231m_received.minute);
    TEST_ASSERT_EQUAL_UINT8(ds3231m_zero.hour, ds3231m_received.hour);
    TEST_ASSERT_EQUAL_UINT8(ds3231m_zero.date, ds3231m_received.date);
    TEST_ASSERT_EQUAL_UINT8(ds3231m_zero.month, ds3231m_received.month);
    TEST_ASSERT_EQUAL_UINT16(ds3231m_zero.year, ds3231m_received.year);

    ds3231m.second = 23;
    ds3231m.minute = 38;
    ds3231m.hour = 14;
    ds3231m.day_of_week = 4;
    ds3231m.date = 26;
    ds3231m.month = 9;
    ds3231m.year = 2019;

    timestamp = convert_dateTime_to_unixms(&ds3231m);
    TEST_ASSERT_EQUAL_UINT64(1569508703000, timestamp);

    convert_unixms_to_dateTime(timestamp, &ds3231m_received);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.second, ds3231m_received.second);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.minute, ds3231m_received.minute);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.hour, ds3231m_received.hour);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.date, ds3231m_received.date);
    TEST_ASSERT_EQUAL_UINT8(ds3231m.month, ds3231m_received.month);
    TEST_ASSERT_EQUAL_UINT16(ds3231m.year, ds3231m_received.year);
    

}

/**
 * @ingroup test_ds3231m
 * @brief Create mock buffer and register, check if get_temperature behaves as expected and the result is correct.
 */
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