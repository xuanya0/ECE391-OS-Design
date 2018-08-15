#ifndef _RTC_H
#define _RTC_H

#include "lib.h"
#include "i8259.h"
#include "syscall.h"


#define RTC_PORT1 0x70
#define RTC_PORT2 0x71
#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_C 0x0C
#define RTC_REG_D 0x0D
/* OR with control register B to turn on PIE */
#define PIE_ON 0x40
/* AND with control register B to turn off PIE */
#define PIE_OFF 0xBF
/* OR with control register A to turn on Binary Mode (DM = 1) */
#define BIN_MODE_ON 0x04
#define RTC_FREQ_LIMIT 1024
#define RTC_FREQ_CTL 16
#define RTC_FREQ_2 0x0F		//for initialize rtc frequency to 2Hz
#define RTC_FREQ_4 0x0E			//for set rtc frequency to 4Hz
#define RTC_FREQ_8 0x0D			//for set rtc frequency to 8Hz
#define RTC_FREQ_16 0x0C		//for set rtc frequency to 16Hz
#define RTC_FREQ_32 0x0B		//for set rtc frequency to 32Hz
#define RTC_FREQ_64 0x0A		//for set rtc frequency to 64Hz
#define RTC_FREQ_128 0x09		//for set rtc frequency to 128Hz
#define RTC_FREQ_256 0x08		//for set rtc frequency to 256Hz
#define RTC_FREQ_512 0x07		//for set rtc frequency to 512Hz
#define RTC_FREQ_1024 0x06		//for set rtc frequency to 1024Hz

#define VIDEO 0xB8000
#define RTC_ADDR_PORT 0x70
#define RTC_DATA_PORT 0x71

typedef struct time
{
	int8_t second;
	int8_t minute;
	int8_t hour;
	int8_t week;
	int8_t day;
	int8_t month;
	int16_t year;
}time_t;

extern time_t gettime();
extern void display_time(time_t time);

void rtc_handler(void);
int32_t open_rtc(const uint8_t* filename);
int32_t close_rtc(int32_t fd);
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes);
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes);

//for getting time
int get_update_in_progress_flag();
uint8_t get_RTC_register(int reg);

void read_time();
void printtime();



extern file_ops_table_t rtc_ops_table;

#endif
