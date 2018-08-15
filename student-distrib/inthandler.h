#ifndef _HANDLER_H
#define _HANDLER_H
#include "lib.h"
#include "i8259.h"
#define IRQ0 0
#define IRQ1 1
#define IRQ2 2
#define IRQ3 3
#define IRQ4 4
#define IRQ5 5
#define IRQ6 6
#define IRQ7 7
#define IRQ8 8
#define IRQ9 9
#define IRQ10 10
#define IRQ11 11
#define IRQ12 12
#define IRQ13 13
#define IRQ14 14
#define IRQ15 15

void div_by_0 (void);
void debugger (void);
void nmi (void);
void breakpoint (void);
void over_flow (void);
void bounds (void);
void opcode_invalid (void);
void no_coprocessor (void);
void double_fault (void);
void coprocessor_segment (void);
void invalid_task_state (void); 
void segment_not_present (void);
void stack_fault (void);
void general_protection_fault (void);
void page_fault (void);
void reserved (void);
void math_fault (void);
void align_check (void);
void machine_check (void);
void SIMD_floating (void); 
void timer_hand(void);
void timer(void);
void dfthand (void);

#endif
