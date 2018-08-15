/*
schedule.c
*/

#include "schedule.h"
#include "paging.h"
#include "i8259.h"
/* Set PIT frequency to 50Hz */
#define PIT_FREQ 100
#define VIDEO_MEM_PTE_INDEX 0xB8

uint32_t sched_esp;

task_t terminal[3];

list_t task_list;



/*
 *	init_timer
 *	PARAMETER:	none
 *	DESCRIPTION:	set the timer
 *	OUTPUT:	none
 */

void
init_timer()
{
	//The value we send to the PIT is the value to divide it's input clock
	//(1193180 Hz) by, to get our required frequency. Important to note is
	//that the divisor must be small enough to fit into 16-bits. 
	uint32_t divisor = 1193180 / PIT_FREQ;
	/* Send the command byte. */
	outb(0x36, 0x43);
	/* Divisor has to be sent byte-wise, so split here into upper/lower bytes. */
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)((divisor>>8) & 0xFF);
	/* Send the frequency divisor. */
	outb(l, 0x40);
	outb(h, 0x40);
}




/*
 *	init_schedule
 *	PARAMETER:	none
 *	DESCRIPTION:	initializes the schduler, sets up the correct paging
 *	OUTPUT: none
 */

void
init_schedule()
{
	for(curr_term=0;curr_term<3;curr_term++)
	{
	/* Initialize task and list structures */
		terminal[curr_term].pid = curr_term;
		terminal[curr_term].term = curr_term;
		terminal[curr_term].next = &terminal[(curr_term+1)%3];
		terminal[curr_term].active = 0;
	}
	terminal[0].active = 1;
	curr_term=0;
	term_on_screen=0;
	task_list.head = &terminal[0];
	task_list.tail = &terminal[2];
}



/*
 *	do_schdule
 *	PARAMETER:	none
 *	DESCRIPTION:	performs scheduling
 *	OUTPUT:	none
 */


void
context_switch()
{
	/* Save esp to curr_pid's PCB */
	pcb_t* curr_pcb = (pcb_t*)pcb_ptr_array[curr_pid];
	curr_pcb->great_esp = sched_esp;//?
	
	task_t* future = task_list.head;
	/* Modify task_list head, tail pointers */
	task_list.head = task_list.head->next;
	task_list.tail = task_list.tail->next;
	/* Get the pcb of the to_do task */
	uint32_t future_pid = future->pid;
	
	if(future_pid == curr_pid || future->active==0)
		return;
	//while(1);
	pcb_t* future_pcb = (pcb_t*)pcb_ptr_array[future_pid];
	/* TSS related */
	tss.ss0 = KERNEL_DS;
	tss.esp0 = (0x00800000 - 0x00002000*future_pid - 0x4);
	
	/* Toggle global curr_term, curr_pid */
	curr_term = future->term;//?
	//update_cursor();
	curr_pid = future_pid;
	/* Flush TLB here */
	flush_tlb(future_pid);
	//set interrupt flag
	
	/* Check if the process is the unexecuted shell */
	if(future_pcb->shell_flag){
		future_pcb->shell_flag = 0;
		execute_root_shell(future_pcb);
	}else{
		switch_to_prog(future_pcb);
	}
}



/*
 *	switch_to_prog
 *	PARAMETER:	pcb_t* the_pcb
 *	DESCRIPTION:	switches to the program designated by its pcb
 *	OUTPUT:	none
 */

void
switch_to_prog(pcb_t* the_pcb)
{
	/* The process is a regular, currently running program */
	send_eoi(0);
	uint32_t curr_esp;
	curr_esp = the_pcb->great_esp;
	asm volatile("                  \n\
			movl    %0, %%esp       \n\
			popl	%%ebx			\n\
			popl	%%ecx			\n\
			popl	%%edx			\n\
			popl	%%esi			\n\
			popl	%%edi			\n\
			popl	%%ebp			\n\
			popl	%%eax			\n\
			pop		%%ds			\n\
			pop		%%es			\n\
			sti						\n\
			iret					\n\
			"
			:
			: "r"(curr_esp)
			: "memory", "cc"
			);
}


/*
 *	switch_to_prog
 *	PARAMETER:	pcb_t* the_pcb
 *	DESCRIPTION:	switches to the first shell by its pcb
 *	OUTPUT:	none
 */


void
execute_root_shell(pcb_t* the_pcb)
{
	/* The process is the unexecuted shell */
	send_eoi(0);
	uint32_t cmd_eip = the_pcb->cmd_eip;
	jump_to_program((uint32_t)(0x08400000 - 0x4), cmd_eip, -1);
}
