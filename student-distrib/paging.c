#include "paging.h"

#define KERNAL_ADDR_IN_PDE 0x400
#define PROG_IMG_ADDR_IN_PDE 0x800
#define VIDEO_MEMORY_IN_PTE 0xB8
#define VIDEO_MEM_PTE_INDEX 0xB8

void init_paging()
{
	int process_index;
	int index;
	// initializing
	for(process_index = 0; process_index < NUM_OF_PROCESSES; ++process_index)
	{
		for(index = 0; index < PAGE_DIRECTORY_SIZE; ++index)
		{
			page_directory[process_index][index].pde = 0;
			page_table[process_index][index].pte = 0;
		}
	}
	// map the pages
	for(process_index = 0; process_index < NUM_OF_PROCESSES; ++process_index)
	{
		// setup kernal page 
		// 4MB(0x00400000) virtual memory addr., equal to physical memory addr. for kernel
		//| PDE index  |     phys. offset           |       
		//|0000,0000,01|00,0000,0000,|0000,0000,0000|
		//|1 denotes the second PDE
		page_directory[process_index][1].present = 1;
		page_directory[process_index][1].page_size = 1;
		page_directory[process_index][1].global = 1;
		page_directory[process_index][1].page_table_addr = KERNAL_ADDR_IN_PDE;	
		// setup program page
		// virtual mem. addr. at 128MB(0x08000000) mapped to 8MB(0x00800000) physical memory addr.
		//| PDE index  |     phys. offset           |       
		//|0000,1000,00|00,0000,0000,|0000,0000,0000|
		//| 1000,00 denotes the 32th PDE
		page_directory[process_index][32].present = 1;
		page_directory[process_index][32].page_size = 1;
		page_directory[process_index][32].read_write = 1;
		page_directory[process_index][32].user_supervisor = 1;
		page_directory[process_index][32].page_table_addr = PROG_IMG_ADDR_IN_PDE + process_index*0x400;	
		// setup video memory
		// 0xB8000 virtual memory addr., equal to physical memory addr. for video memory
		// bit 22 is 0, which denotes the first PDE
		page_directory[process_index][0].present = 1;
		page_directory[process_index][0].read_write = 1;
		page_directory[process_index][0].user_supervisor = 1;
		page_directory[process_index][0].page_table_addr = ((uint32_t)page_table[process_index] >> 12);
		
		page_directory[process_index][33].present = 1;
		page_directory[process_index][33].read_write = 1;
		page_directory[process_index][33].user_supervisor = 1;
		page_directory[process_index][33].page_table_addr = ((uint32_t)video_table[process_index] >> 12);
		// 0xB8 in the second 10 bits denotes 0xB8th PTE
		// the PTE should points to 0xB8000 in physical address
		page_table[process_index][VIDEO_MEM_PTE_INDEX].present = 1;
		page_table[process_index][VIDEO_MEM_PTE_INDEX].read_write = 1;
		page_table[process_index][VIDEO_MEM_PTE_INDEX].user_supervisor = 1;
		if (process_index < 3){
			page_table[process_index][VIDEO_MEM_PTE_INDEX].phys_page_addr = VIDEO_MEMORY_IN_PTE+2*process_index;
		}
		else{
			page_table[process_index][VIDEO_MEM_PTE_INDEX].phys_page_addr = VIDEO_MEMORY_IN_PTE;	
		}
		for (index = 0; index < 3; index++)
		{
			video_table[process_index][VIDEO_MEM_PTE_INDEX+2*index].present = 1;
			video_table[process_index][VIDEO_MEM_PTE_INDEX+2*index].read_write = 1;
			video_table[process_index][VIDEO_MEM_PTE_INDEX+2*index].user_supervisor = 1;
			video_table[process_index][VIDEO_MEM_PTE_INDEX+2*index].phys_page_addr = VIDEO_MEMORY_IN_PTE+2*index;
		}
	}
	
	set_pse(); // set the pse in cr4 to change page size 
	flush_tlb(0); // flush the TLB, set correct cr3
	set_cr0(); // set the cr0 to enable paging 
}

void flush_tlb(uint8_t process_index)
{
	page_directory_entry* pd_ptr = page_directory[process_index];
	asm volatile
	(
		"movl %%cr3, %%eax		\n"
		"movl %0, %%ecx			\n"
		"andl $0xFFFFF000, %%ecx	\n"
		"andl $0x00000FFF, %%eax	\n"
		"addl %%ecx, %%eax		\n"
		"movl %%eax, %%cr3"
		:
		: "r"(pd_ptr)
		: "%eax", "%ecx"
	);
}

void set_cr0() 
{
	asm volatile
	(
		"movl %%cr0, %%eax		\n"		
		"orl $0x80000000, %%eax	\n"	
		"movl %%eax, %%cr0		\n"		
		:
		:
		: "%eax"
	);
}

void set_pse()
{
	asm volatile
	(
		"movl %%cr4, %%eax		\n"	
		"orl $0x00000010, %%eax	\n"
		"movl %%eax, %%cr4		\n"		
		:
		:
		: "%eax"
	);
}


