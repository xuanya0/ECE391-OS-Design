#ifndef _TERMINAL_H_
#define _TERMINAL_H_
#include "lib.h"
#include "paging.h"
#include "syscall.h"
#include "user.h"
#define F1 0x80
#define F2 0x81
#define F3 0x82
#define NUM_TOTAL_PROC 6
#define PG_UP 0x83
#define PG_DN 0x84

#define UP 0
#define DN 1

extern void kbd(void);

extern int32_t terminal_open(const uint8_t* filename);

extern int32_t terminal_read(int32_t fd, uint8_t* buf, int32_t cnt);

extern int32_t terminal_write(int32_t fd, uint8_t* buf, int32_t cnt);

extern file_ops_table_t terminal_ops_table;

extern int32_t terminal_close(const uint8_t* filename);

extern void scroll_screen();

void setup_terms();

void switch_term(uint8_t to_term);


#endif 
