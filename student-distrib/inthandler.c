#include "inthandler.h"
#include "schedule.h"
#include "terminal.h"
#include "syscall.h"


void div_by_0 (void)
{
	clear();
	printf("Exception 0:divided by 0!\n");
	while(1);
}
void debugger (void)
{
	clear();
	printf("Exception 1:debugger\n");
	while(1);
}
void nmi (void)
{
	clear();
	printf("Exception 2:NMI\n");
	while(1);
}
void breakpoint (void)
{
	clear();
	printf("Exception 3:breakpoint\n");
	while(1);
}
void over_flow (void)
{
	clear();
	printf("Exception 4:overflow\n");
	while(1);
}
void bounds (void)
{
	clear();
	printf("Exception 5:bounds\n");
	while(1);
}
void opcode_invalid (void)
{
	clear();
	printf("Exception 6:opcode invalid\n");
	while(1);
}
void no_coprocessor (void)
{
	clear();
	printf("Exception 7:coprocessor not available\n");
	while(1);
}
void double_fault (void)
{
	clear();
	printf("Exception 8:double fault\n");
	while(1);
}
void coprocessor_segment (void)
{
	clear();
	printf("Exception 9:coprocessor segment overrun\n");
	while(1);
}
void invalid_task_state (void)
{
	clear();
	printf("Exception 10:invalid task state segment\n");
	while(1);
}
void segment_not_present (void)
{
	clear();
	printf("Exception 11:segment not present\n");
	while(1);
}
void stack_fault (void)
{
	clear();
	printf("Exception 12:stack fault\n");
	while(1);
}
void general_protection_fault (void)
{
	clear();
	printf("Exception 13:general protection fault\n");
	while(1);
}
void page_fault (void)
{
	flush_tlb(0);
	printf("Exception 14:page fault\n");
	//halt(0);
	//exit_to_shell(0, 0);
	while(1);
}
void reserved (void)
{
	clear();
	printf("Exception 15:reserved\n");
	while(1);
}
void math_fault (void)
{
	clear();
	printf("Exception 16:math fault\n");
	while(1);
}
void align_check (void)
{
	clear();
	printf("Exception 16:alignment check\n");
	while(1);
}
void machine_check (void)
{
	clear();
	printf("Exception 17:machine check\n");
	while(1);
}
void SIMD_floating (void)
{
	clear();
	printf("Exception 18:SIMD floating point exception\n");
	while(1);
}
void dfthand (void)
{
	clear();
	printf("Exception & Interrupt:default handler\n");
}
void timer(void)
{
	context_switch();
	send_eoi(0);
}


