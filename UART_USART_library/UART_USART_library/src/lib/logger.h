/*
							        *******************
******************************* C HEADER FILE *******************************
**                           *******************                           **
**                                                                         **
** project   : RAPTORS                                                     ** 
** filename  : logger                                                      **
** date      : January 11, 2019                                            **
** author    : Julien Delvaux                                              **
** licence   : MIT                                                         **
**                                                                         **
*****************************************************************************
Based on rxi library: https://github.com/rxi/log.c/

*/


#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(__SAME70Q21__)
#   include "compiler.h"
#	include "../config/conf_board.h"
#else
#   include <stdint.h>
#endif

#if defined(SERIAL_LOG)
#   include "serial_mdw.h"
#   include "delay.h"
#else
#	warning "Logger not activated"
#endif

/*
   ---------------------------------------
   ---------- Logger options ----------
   ---------------------------------------
*/

typedef enum {LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL} log_level_t;

// Define if you want to use a more verbose option
#define ADVANCED_LOG
#define LOGGER_MESSAGE_MAX_LENGTH 100

#if defined(SERIAL_LOG)
// RAPTORS IS UART0
// SAME70-XPLD is USART1 (use default USB com port)
#   define SERIAL_LOG_ID	USART1
    const static uint8_t	DELAY_TO_PRINT = 0;
#endif

/*
   ---------------------------------------
   --------- Debugging functions ---------
   ---------------------------------------
*/

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


extern void logger_init(log_level_t log_level);

extern void logger_set_log_level(log_level_t log_level);

extern void log_buffer(const char *text, uint8_t *p_buff, uint8_t length);

extern void log_log(log_level_t level, const char *file, uint32_t line, const char *fmt, ...) __attribute__ ((format (gnu_printf, 4, 5)));


#endif /* LOGGER_H_ */