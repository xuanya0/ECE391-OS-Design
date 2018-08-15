/*
schedule.h
*/

#include "syscall.h"
#include "lib.h"

/* esp variable being filled with the esp directly 
after all the regs have been pushed onto the stack */
extern uint32_t sched_esp;

/* define task element structure */
typedef struct the_task
{
	uint32_t pid;
	uint32_t term;
	struct the_task* next;
	volatile uint8_t active;
} task_t;

/* define circular linked list structure */
typedef struct the_list
{
	task_t* head;
	task_t* tail;
} list_t;

extern task_t terminal[3];

extern list_t task_list;

extern void init_schedule();

extern void init_timer();

extern void context_switch();

extern void switch_to_prog(pcb_t* the_pcb);

extern void execute_root_shell(pcb_t* the_pcb);
