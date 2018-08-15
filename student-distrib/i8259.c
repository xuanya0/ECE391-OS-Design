/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
	// use a lock here maybe
/* write ICWs */
	printf("Initializing 8259\n");
	unsigned long flags;
	cli_and_save(flags);
	
	master_mask = 0xff;		/* initialize master mask to 0xff */
	slave_mask = 0xff;		/* initialize slave mask to 0xff */
	
	outb(master_mask, MASTER_8259_PORT2);	/* mask all of 8259A-1 */
	outb(slave_mask, SLAVE_8259_PORT2);		/* mask all of 8259A-2 */
	
	outb(ICW1, MASTER_8259_PORT);					/* ICW1: select 8259A-1 init */
	outb(ICW2_MASTER, MASTER_8259_PORT2);	/* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
	outb(ICW3_MASTER, MASTER_8259_PORT2);		/* 8259A-1 (the master) has a slave on IR2 */
	
	outb(ICW4, MASTER_8259_PORT2);		/* master expects normal EOI */
	
	outb(ICW1, SLAVE_8259_PORT);			/* ICW1: select 8259A-2 init */
	outb(ICW2_SLAVE, SLAVE_8259_PORT2);	/* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
	outb(ICW3_SLAVE, SLAVE_8259_PORT2);	/* 8259A-2 is a slave on master's IR2 */
	outb(ICW4, SLAVE_8259_PORT2);		/* (slave's support for AEOI in flat mode
											   is to be investigated) */
	restore_flags(flags);
	// if a lock is present, release the lock at this point
}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	//printf("enabling %d\n",irq_num);
	if((irq_num & 8)){
		irq_num = irq_num & 7;
		slave_mask = slave_mask & (0xff - (1 << irq_num));
		outb(slave_mask, SLAVE_8259_PORT2);
		master_mask = master_mask & (0xff - (1 << 2));
		outb(master_mask, MASTER_8259_PORT2);
	}else{
		master_mask = master_mask & (0xff - (1 << irq_num));
		outb(master_mask, MASTER_8259_PORT2);
	}
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
// Possible lock here
	if((irq_num & 8)){
		irq_num = irq_num & 7;
		slave_mask = slave_mask | (1 << irq_num);
		outb(slave_mask, SLAVE_8259_PORT2);
	}else{
		master_mask = master_mask | (1 << irq_num);
		outb(master_mask, MASTER_8259_PORT2);
	}	
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
// Possible lock here
	unsigned char intr_finished;
	if((irq_num & 8)){
		intr_finished = (unsigned char)(irq_num - 8) | EOI;
		outb(intr_finished, SLAVE_8259_PORT);
		outb((2 | EOI), MASTER_8259_PORT);
	}else{
		intr_finished = (unsigned char)irq_num | EOI;
		outb(intr_finished, MASTER_8259_PORT);
	}
}

