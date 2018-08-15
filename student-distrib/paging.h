#ifndef _PAGING_H_
#define _PAGING_H_

#include "types.h"
#include "lib.h"

#define PAGE_TABLE_SIZE 1024
#define PAGE_DIRECTORY_SIZE 1024
#define NUM_OF_PROCESSES 6

typedef union page_directory_entry
{
	uint32_t pde;
	struct
	{
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t reserved : 1;
		uint32_t page_size : 1;
		uint32_t global : 1;
		uint32_t avail : 3;
		// when 4MB paging is used, "page_table_addr" points to the physical memory
		uint32_t page_table_addr : 20; 
	}; 
}page_directory_entry;

typedef union page_table_entry
{
	uint32_t pte;
	struct
	{
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t dirty : 1;
		uint32_t reserved : 1;
		uint32_t global : 1;
		uint32_t avail : 3;
		uint32_t phys_page_addr : 20;
	}; 
}page_table_entry;

page_directory_entry page_directory[NUM_OF_PROCESSES][PAGE_DIRECTORY_SIZE] __attribute__ ((aligned(4096)));
// in our OS 4KB pages are only used for mapping video memory
page_table_entry page_table[NUM_OF_PROCESSES][PAGE_TABLE_SIZE] __attribute__ ((aligned(4096)));

page_table_entry video_table[NUM_OF_PROCESSES][PAGE_TABLE_SIZE] __attribute__ ((aligned(4096)));

void init_paging();
void set_cr0();
void set_pse();
void flush_tlb(uint8_t pid);


#endif
