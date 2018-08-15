#include "rtc.h"
#include "lib.h"

#define DEBUG_TIME_DISPLAY 0

/* global variable for detecting a rtc interrupt*/
volatile int rtc_interrupt_occured=0;


//rtc function pointer table
file_ops_table_t rtc_ops_table =
{
	.open = (void*) open_rtc,
	.read = (void*) read_rtc,
	.write = (void*) write_rtc
};

/*-----------------------------------------*/
/* open_rtc initialize rtc rate to be 2 Hz and turn the device on
passin arg: filename(not used)
return value: always should be successful and return 0
side _effect: none
*/
/*----------------------------------------*/
int32_t open_rtc(const uint8_t* filename)
{
	//printf("open_rtc \n");
	/* set 2 HZ by setting rtc register A */
	cli();
	outb(0x8A, RTC_PORT1);			//ox8A means mask nmi while requesting register A 
	int8_t prev = inb(RTC_PORT2);	//prev stores value in device register A
	prev |= RTC_FREQ_2;				//set frequency to 2Hz by
	outb(0x8A, RTC_PORT1);
	outb(prev, RTC_PORT2);			// set last 4 bit of device register A to default 2Hz


	/* turn on rtc by setting rtc control register B */	
	outb(0x8B, RTC_PORT1);	//ox8B means mask nmi while requesting register B 
	prev = inb(RTC_PORT2);	//prev is temp value in device register B
	/* turn on PIE */
	prev |= PIE_ON;
	outb(0x8B, RTC_PORT1);
	outb(prev, RTC_PORT2);		//turn on device 
	sti();
	/* frequency is defaulted to be 2 Hz */
	return 0;
}

/*-----------------------------------------*/
/* read_rtc detects a rtc interrupt with set rate handled by rtc_handler()
passin arg: all not used
return value: always should be successful and return 0
side _effect: none
*/
/*----------------------------------------*/
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes){
	sti();
	if (rtc_interrupt_occured){
		while(rtc_interrupt_occured);
	}
	else{
		while(!rtc_interrupt_occured);
	}
	return 0;
}

/*-----------------------------------------*/
/* write_rtc writes a rtc interrupt rate into rtc device it could only writes in a rate that is a power of 2
passin arg: int32_t freq 
return value: return 0 on successful set, -1 if freq is either out of range or not a power of 2
side _effect: none
*/
/*----------------------------------------*/
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes){
	int8_t prev;		//prev is temp value in device register A
	uint32_t freq;
	memcpy(&freq, (uint8_t*)buf, nbytes);
	cli();	//turn off interrupts refer to lib.h for implement details	
	outb(0x8A, RTC_PORT1);
	prev = inb(RTC_PORT2);	//prev is temp value in device register A
	prev &= 0xF0;
	switch (freq){
		case 2:
				prev |= RTC_FREQ_2;				//set frequency to 2Hz
				break;
		case 4:
				prev |= RTC_FREQ_4;				//set frequency to 4Hz
				break;
		case 8:	
				prev |= RTC_FREQ_8;				//set frequency to 8Hz
				break;
		case 16:
				prev |= RTC_FREQ_16;				//set frequency to 16Hz
				break;
		case 32:
				prev |= RTC_FREQ_32;				//set frequency to 32Hz
				break;		
		case 64:
				prev |= RTC_FREQ_64;				//set frequency to 64Hz
				break;
		case 128:
				prev |= RTC_FREQ_128;				//set frequency to 128Hz
				break;		
		case 256:
				prev |= RTC_FREQ_256;				//set frequency to 256Hz
				break;
		case 512:
				prev |= RTC_FREQ_512;				//set frequency to 512Hz
				break;
		case 1024:
				prev |= RTC_FREQ_1024;				//set frequency to 1024Hz
				break;
		default:
			sti();
			return -1;							//invalid input
	}
	outb(0x8A, RTC_PORT1);
	outb(prev, RTC_PORT2);			// set last 4 bit of device register A to 2Hz
	sti();
	return 0;
}
/*-----------------------------------------*/
/* close_rtc does nothing but return 0 because the device is not supposed to be turned off in this mp
passin arg: none 
return value: return 0 on successful
side _effect: none
*/
/*----------------------------------------*/

int32_t close_rtc(int32_t fd){
	return 0;
}

/*-----------------------------------------*/
/* rtc_handler routinely sending a rtc interrupt with set rate by periodically change rtc_interrupt_occured value
passin arg: none 
return value: none
side _effect: none
*/
/*----------------------------------------*/
void rtc_handler(void)
{
	time_t cur_time = gettime();
	display_time(cur_time);
	//printf("rtchandler");
	outb(0x0C, RTC_PORT1);
	inb(RTC_PORT2);
	//test_interrupts();
	if(rtc_interrupt_occured)
		rtc_interrupt_occured = 0;
	else
		rtc_interrupt_occured = 1;		
	send_eoi(8);
	asm ("leave;\
		  iret;");
}

// from OSdev
/*int get_upday_in_progress_flag() 
{
    outb(0x0A, RTC_PORT1);
    return (inb(RTC_PORT2) & 0x80);
}*/

uint8_t get_reg_value(int reg) 
{
    outb(reg, RTC_PORT1);
    return inb(RTC_PORT2);
}

time_t gettime()
{
	/*	 0B  RTC Status register B:
	    |7|6|5|4|3|2|1|0|  RTC Status Register B
	     | | | | | | | `---- 1=enable daylight savings, 0=disable (default)
	     | | | | | | `----- 1=24 hour mode, 0=12 hour mode (24 default)
	     | | | | | `------ 1=time/day in binary, 0=BCD (BCD default)
	     | | | | `------- 1=enable square wave frequency, 0=disable
	     | | | `-------- 1=enable upday ended interrupt, 0=disable
	     | | `--------- 1=enable alarm interrupt, 0=disable
	     | `---------- 1=enable periodic interrupt, 0=disable
	     `----------- 1=disable clock upday, 0=upday count normally*/
	uint8_t reg_b;

	int second, minute, hour, week, day, month, year;
	
	time_t time;

	cli();
	outb(0x0B, RTC_ADDR_PORT);
	reg_b = inb(RTC_DATA_PORT); // get CRB
	
#if(DEBUG_TIME_DISPLAY == 1)	
	printf("data mode %d, hour mode %d, daylight savings %d\n", ((reg_b & 0x4) >> 2), ((reg_b & 0x2) >> 1), (reg_b & 0x1));
#endif

	reg_b |= 0x80; //disable clock upday
	outb(reg_b, RTC_DATA_PORT);
	
	second = get_reg_value(0x00);
	minute = get_reg_value(0x02);
	hour = get_reg_value(0x04);
	week = get_reg_value(0x06);
	day = get_reg_value(0x07);
	month = get_reg_value(0x08);
	year = get_reg_value(0x09);

	// enable updating
	reg_b &= 0x7F;
	outb(0x0B, RTC_ADDR_PORT);
	outb(reg_b, RTC_DATA_PORT);
#if(DEBUG_TIME_DISPLAY == 1)	
	printf("original second %x, minute %x, hour %x, week %x, day %x, month %x, year %x\n",
	second, minute, hour, week, day, month, year);
#endif

	time.second = (second & 0x0F) + 10*((second & 0x70) >> 4);
	time.minute = (minute & 0x0F) + 10*((minute & 0x70) >> 4);
	time.hour = (hour & 0x0F) + 10*((hour & 0x30) >> 4);
	time.week = week & 0x07;

	time.day = (day & 0x0F) + 10*((day & 0x70) >> 4);
	time.month = (month & 0x0F) + 10*((month & 0x70) >> 4);
	time.year = 2000 + (year & 0x0F) + 10*((year & 0xF0) >> 4);
	
	
#if(DEBUG_TIME_DISPLAY == 1)	
	printf("converted GMT second %d, minute %d, hour %d, week %d, day %d, month %d, year %d\n",
	time.second, time.minute, time.hour, time.week, time.day, time.month, time.year);
#endif

	// convert from GMT to CST
	if(time.hour - 5 < 0)
	{
		time.hour = time.hour - 5 + 24; // yesterday
		time.week -= 1; time.day -= 1;
		if(time.week < 1)
			time.week = 7; // week: 1 - Sunday; 7 Saturday
		if (time.day < 0)
		{
			switch (time.month)
			{
				case 1: time.day = 31; time.month = 12; time.year -= 1;
					break;
				case 2: time.day = 31; time.month = 1;
					break;
				case 3: if(time.year % 4 == 0)
						time.day = 29;
					else
						time.day = 28;
					time.month = 2;
					break;
				case 4: time.day = 31; time.month = 3;
					break;
				case 5: time.day = 30; time.month = 4;
					break;
				case 6: time.day = 31; time.month = 5;
					break;
				case 7: time.day = 30; time.month = 6;
					break;
				case 8: time.day = 31; time.month = 7;
					break;
				case 9: time.day = 31; time.month = 8;
					break;
				case 10: time.day = 30; time.month = 9;
					break;
				case 11: time.day = 31; time.month = 10;
					break;
				case 12: time.day = 30; time.month = 11;
					break;
				default:
					break;
			}
		}
	}
	else
		time.hour -= 5;
#if(DEBUG_TIME_DISPLAY == 1)	
	printf("converted CST second %d, minute %d, hour %d, week %d, day %d, month %d, year %d\n",
	time.second, time.minute, time.hour, time.week, time.day, time.month, time.year);
	while(1);
#endif
	sti();
	return time;
}

void display_time(time_t time){
	int8_t buf[28] = "UIUC 0000-00-00 DDD 00:00:00";
	int32_t i;
	int8_t* screen_start = (int8_t*)(VIDEO + (80*24 + 52) *2);
	switch (time.week - 1){
		case 0: buf[16] = 'S'; buf[17] = 'U'; buf[18] = 'N';
			break;
		case 1: buf[16] = 'M'; buf[17] = 'O'; buf[18] = 'N';
			break;
		case 2: buf[16] = 'T'; buf[17] = 'U'; buf[18] = 'E';
			break;
		case 3: buf[16] = 'W'; buf[17] = 'E'; buf[18] = 'D';
			break;
		case 4: buf[16] = 'T'; buf[17] = 'H'; buf[18] = 'U';
			break;
		case 5: buf[16] = 'F'; buf[17] = 'R'; buf[18] = 'I';
			break;
		case 6: buf[16] = 'S'; buf[17] = 'A'; buf[18] = 'T';
			break;
		default: printf("Invalid week of the week.\n");
			 return;
	}

	// get single digit, convert to ascii code	
	buf[5] = (time.year/1000) + 0x30; time.year %= 1000;
	buf[6] = (time.year/100) + 0x30; time.year %= 100;
	buf[7] = (time.year/10) + 0x30; time.year %= 10;
	buf[8] = time.year + 0x30;
	
	buf[10] = (time.month/10) + 0x30; time.month %= 10;
	buf[11] = time.month + 0x30;

	buf[13] = (time.day/10) + 0x30; time.day %= 10;
	buf[14] = time.day + 0x30;
	if(time.hour == 0)
	{
		buf[20] = 0x32; 
		buf[21] = 0x33; // manually set daylight saving time
	}
	else
	{
		buf[20] = (time.hour/10) + 0x30; time.hour %= 10;
		buf[21] = time.hour - 1 + 0x30; // manually set daylight saving time
	}
	buf[23] = (time.minute/10) + 0x30; time.minute %= 10;
	buf[24] = time.minute + 0x30;

	buf[26] = (time.second/10) + 0x30; time.second %= 10;
	buf[27] = time.second + 0x30;
	
	for (i = 0; i < 28; i++){
		*screen_start++ = buf[i];
		*screen_start++ = 0x7;
	}
}
