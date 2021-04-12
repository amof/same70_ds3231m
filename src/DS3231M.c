#include "DS3231M.h"

#include <stdio.h>
#include <sysclk.h>
#include <twihs.h>

#include "../config/conf_board.h"
#include "logger.h"

const uint8_t DS3231_REGISTER_CONTROL			= 0x0E;
const uint8_t DS3231_REGISTER_STATUS			= 0x0F;
const uint8_t DS3231_REGISTER_TEMPERATURE		= 0x11;

const uint8_t DS3231_REGISTER_SECONDS			= 0x00;
const uint8_t DS3231_REGISTER_MINUTES			= 0x01;
const uint8_t DS3231_REGISTER_HOUR				= 0x02;
const uint8_t DS3231_REGISTER_DAY				= 0x03;
const uint8_t DS3231_REGISTER_DATE				= 0x04;
const uint8_t DS3231_REGISTER_MONTH				= 0x05;
const uint8_t DS3231_REGISTER_YEAR				= 0x06;

#define DS3231_REGISTER_TEMPERATURE_LENGTH	2
#define DS3231_REGISTER_DATETIME_LENGTH			7

#define DS3231_MASK_STATUS_BIT							2

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

float convert_temperature_unsigned_to_float(uint8_t *buffer);

uint32_t DS3231M_init(ds3231m_t *ds3231m)
{
	ds3231m->second = 0;
	ds3231m->minute = 0;
	ds3231m->hour = 0;
	ds3231m->day_of_week = 0;
	ds3231m->date = 0;
	ds3231m->month = 0;
	ds3231m->year = 0;
	return twihs_probe(TWIHS0, ds3231m->address);
}

uint32_t DS3231M_set_time(ds3231m_t *ds3231m)
{
	uint8_t status_reg = 0;
	
	uint8_t buffer[DS3231_REGISTER_DATETIME_LENGTH] = {
		bin2bcd(ds3231m->second),
		bin2bcd(ds3231m->minute),
		bin2bcd(ds3231m->hour),
		bin2bcd(ds3231m->day_of_week),
		bin2bcd(ds3231m->date),
		bin2bcd(ds3231m->month),
		bin2bcd(ds3231m->year - 2000)
		} ;
	
	// Set the new date/time
	twihs_packet_t packet = {
		.chip = ds3231m->address,
		.addr[0] = DS3231_REGISTER_SECONDS,
		.addr_length = 1,
		.buffer = (uint8_t *)buffer,
		.length = DS3231_REGISTER_DATETIME_LENGTH
	};

	uint8_t result = twihs_master_write(TWIHS0, &packet);
	
	// Clear OSF bit in status register
	if(result == TWIHS_SUCCESS)
	{
		packet.addr[0] = DS3231_REGISTER_STATUS;
		packet.buffer = &status_reg;
		packet.length = 1;

		result = twihs_master_read(TWIHS0, &packet);
		
		if(result == TWIHS_SUCCESS)
		{
			status_reg &= ~0x80; // Clear OSF bit
			
			packet.addr[0] = DS3231_REGISTER_STATUS;
			packet.buffer = &status_reg;
			packet.length = 1;
			
			result = twihs_master_write(TWIHS0, &packet);
		}
	}

	return result;
}

uint32_t DS3231M_get_time(ds3231m_t *ds3231m)
{
	static uint8_t buffer[DS3231_REGISTER_DATETIME_LENGTH] = {0};
	
	twihs_packet_t packet = {
		.chip = ds3231m->address,
		.addr[0] = DS3231_REGISTER_SECONDS,
		.addr_length = 1,
		.buffer = (uint8_t *)buffer,
		.length = DS3231_REGISTER_DATETIME_LENGTH
	};

	uint32_t result = twihs_master_read(TWIHS0, &packet);

	#if defined(TEST)
	memcpy(buffer, packet.buffer, DS3231_REGISTER_DATETIME_LENGTH);
	#endif
	
	if (result == TWIHS_SUCCESS) 
	{
		ds3231m->second = bcd2bin(buffer[DS3231_REGISTER_SECONDS]);
		ds3231m->minute = bcd2bin(buffer[DS3231_REGISTER_MINUTES]);
		ds3231m->hour = bcd2bin(buffer[DS3231_REGISTER_HOUR]);
		ds3231m->day_of_week = bcd2bin(buffer[DS3231_REGISTER_DAY]);
		ds3231m->date = bcd2bin(buffer[DS3231_REGISTER_DATE]);
		ds3231m->month = bcd2bin(buffer[DS3231_REGISTER_MONTH]);
		ds3231m->year = 2000 + bcd2bin(buffer[DS3231_REGISTER_YEAR]);
	}

	return result;
}

uint32_t DS3231M_get_temperature(ds3231m_t *ds3231m, float *temperature)
{
	uint8_t status_reg = 0;

	twihs_packet_t packet = {
	.chip = ds3231m->address,
	.addr[0] = DS3231_REGISTER_STATUS,
	.addr_length = 1,
	.buffer = &status_reg,
	.length = 1
	};

	uint32_t result = twihs_master_read(TWIHS0, &packet);

	#if defined(TEST)
	memcpy(&status_reg, packet.buffer, 1);
	#endif
	
	// Check if BSY flag is 0 (no conversion on going)
	uint8_t bsy_bit = status_reg & (1<<DS3231_MASK_STATUS_BIT);

	if(result == TWIHS_SUCCESS && bsy_bit == 0)
	{
			uint8_t buffer[DS3231_REGISTER_TEMPERATURE_LENGTH] = {0};
			packet.addr[0] = DS3231_REGISTER_TEMPERATURE;
			packet.buffer = (uint8_t *)buffer;
			packet.length = DS3231_REGISTER_TEMPERATURE_LENGTH;
			
			result = twihs_master_read(TWIHS0, &packet);

			#if defined(TEST)
			memcpy(buffer, packet.buffer, DS3231_REGISTER_TEMPERATURE_LENGTH);
			#endif

			*temperature = convert_temperature_unsigned_to_float(buffer);
	}else if(result == TWIHS_SUCCESS)
	{
		result = 8; // Forcing value to handle case when ds3231m's temperature is not ready
	}
	

	return result;
}

// 

float convert_temperature_unsigned_to_float(uint8_t *buffer)
{
	// Temperature is 10bit with resolution of 0.25°
	float result = 0;
	uint16_t raw_temperature = buffer[0] << 8;
	raw_temperature |= buffer[1];
	raw_temperature >>= 6;

	result = (0x200 & raw_temperature) ? (-(1024-raw_temperature)):raw_temperature; // if 10th bit = 1 -> negative numver (complement of 2)
	result *= 0.25; 
	return result;
}

uint64_t convert_dateTime_to_unixms(ds3231m_t *ds3231m)
{
	uint16_t y;
	uint8_t m;
	uint8_t d;
	uint64_t unix_timestamp;

	// Year
	y = ds3231m->year;
	// Month of year
	m = ds3231m->month;
	// Day of month
	d = ds3231m->date;

	// January and February are counted as months 13 and 14 of the previous year
	if(m <= 2)
	{
			m += 12;
			y -= 1;
	}

	// Convert years to days
	unix_timestamp = (365 * y) + (y / 4) - (y / 100) + (y / 400);
	// Convert months to days
	unix_timestamp += (30 * m) + (3 * (m + 1) / 5) + d;
	// Unix time starts on January 1st, 1970
	unix_timestamp -= 719561;
	// Convert days to seconds
	unix_timestamp *= 86400;
	// Add hours, minutes and seconds
	unix_timestamp += (3600 * ds3231m->hour) + (60 * ds3231m->minute) + ds3231m->second;
	
	// Convert to ms
	unix_timestamp = unix_timestamp*1000;
 
    // Return Unix time
  return unix_timestamp;
}

void convert_unixms_to_dateTime(uint64_t unix_timestamp_ms, ds3231m_t *ds3231m)
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
	    
	//Negative Unix time values are not supported
	if(unix_timestamp_ms < 1){
		unix_timestamp_ms = 0;
	}
	    
	//Retrieve hours, minutes and seconds
	uint64_t unix_timestamp = unix_timestamp_ms /1000;
	ds3231m->second = unix_timestamp % 60;
	unix_timestamp /= 60;
	ds3231m->minute = unix_timestamp % 60;
	unix_timestamp /= 60;
	ds3231m->hour = unix_timestamp % 24;
	unix_timestamp /= 24;
	    
	//Convert Unix time to date
	a = (uint32_t) ((4 * unix_timestamp + 102032) / 146097 + 15);
	b = (uint32_t) (unix_timestamp + 2442113 + a - (a / 4));
	c = (20 * b - 2442) / 7305;
	d = b - 365 * c - (c / 4);
	e = d * 1000 / 30601;
	f = d - e * 30 - e * 601 / 1000;
	    
	//January and February are counted as months 13 and 14 of the previous year
	if(e <= 13)
	{
		c -= 4716;
		e -= 1;
	}
	else
	{
		c -= 4715;
		e -= 13;
	}
	    
	//Retrieve year, month and day
	ds3231m->year = c;
	ds3231m->month = e;
	ds3231m->date = f;
}




