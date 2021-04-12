#ifndef DS3231M_H_
#define DS3231M_H_

#if defined(TEST)
#  include <stdint.h>
#else
#  include "compiler.h" 
#endif


/*
   ---------------------------------------
   --------------- DS3231M ---------------
   ---------------------------------------
*/
#define DS3231_DEFAULT_ADDRESS 0x68

typedef struct ds3231m_t {
	uint8_t address;		// Address of the DS3231M
	uint16_t year; 			// Year
	uint8_t month; 			// Month
	uint8_t day_of_week; 	// Day Of The Week
	uint8_t date; 			// Date of day in the month
	uint8_t hour; 			// Hour
	uint8_t minute; 		// Minute 
	uint8_t second; 		// Second
} ds3231m_t;
 
/*
   ---------------------------------------
   -------------- Functions --------------
   ---------------------------------------
*/

#if !defined(TEST)
extern uint32_t DS3231M_init(ds3231m_t *ds3231m);
extern uint32_t DS3231M_set_time(ds3231m_t *ds3231m);
extern uint32_t DS3231M_get_time(ds3231m_t *ds3231m);
extern uint32_t DS3231M_get_temperature(ds3231m_t *ds3231m, float *temperature);

extern uint64_t convert_dateTime_to_unixms(ds3231m_t *ds3231m);
extern void convert_unixms_to_dateTime(uint64_t unix_timestamp_ms, ds3231m_t *ds3231m);

#elif defined(TEST)
uint32_t DS3231M_init(ds3231m_t *ds3231m);
uint32_t DS3231M_set_time(ds3231m_t *ds3231m);
uint32_t DS3231M_get_time(ds3231m_t *ds3231m);
uint32_t DS3231M_get_temperature(ds3231m_t *ds3231m, float *temperature);
uint64_t convert_dateTime_to_unixms(ds3231m_t *ds3231m);
void convert_unixms_to_dateTime(uint64_t unix_timestamp_ms, ds3231m_t *ds3231m);
float convert_temperature_unsigned_to_float(uint8_t *buffer);
#endif

#endif /* DS3231M_H_ */