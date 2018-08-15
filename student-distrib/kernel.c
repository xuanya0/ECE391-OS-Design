/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "inthandler.h"
#include "terminal.h"
#include "paging.h"
#include "rtc.h"
#include "filesystem.h"
#include "syscall.h"
#include "schedule.h"
#define VIDEO 0xB8000
#define ATTRIB 0x7
#define NUM_COLS 80
#define NUM_ROWS 24


void* exception_table[20]={
	div_by_0,debugger,nmi,breakpoint,
	over_flow,bounds,opcode_invalid,no_coprocessor,
	double_fault,coprocessor_segment,invalid_task_state,
	segment_not_present,stack_fault,
	general_protection_fault,page_fault,reserved,
	math_fault,align_check,machine_check, SIMD_floating 
};
void* int_table[16]={
	timer_hand,kbd,dfthand,dfthand,dfthand,dfthand,
	dfthand,dfthand,rtc_handler,dfthand,dfthand,dfthand,
	dfthand,dfthand,dfthand,dfthand
};

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
	int i;

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;

		filesystem_base = mod->mod_start;

		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}
	for (i = 0; i < 20; i++)
	{
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].size = 1;
		idt[i].reserved0 = 0;
		idt[i].dpl = 0;
		idt[i].present = 1;
		SET_IDT_ENTRY(idt[i], exception_table[i]);
	}
	for (i = 32; i < 48; i++) {
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].size = 1;
		idt[i].reserved0 = 0;
		idt[i].dpl = 0;
		idt[i].present = 1;
		SET_IDT_ENTRY(idt[i], int_table[i-32]);
	}
	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}
	// set the syscall entry
	idt[128].seg_selector = KERNEL_CS;
	idt[128].reserved4 = 0;
	idt[128].reserved3 = 1;
	idt[128].reserved2 = 1;
	idt[128].reserved1 = 1;
	idt[128].size = 1;
	idt[128].reserved0 = 0;
	idt[128].dpl = 3;
	idt[128].present = 1;
	SET_IDT_ENTRY(idt[128], sys_call_handler);
	lidt(idt_desc_ptr);
	


	/* Init the PIC */
	clear();

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	terminal_open(NULL);
	i8259_init();
	open_rtc(NULL);

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	sti();
	enable_irq(1);
	enable_irq(8);
	init_timer();
	// init paging
	init_paging();
	active_proc_num=0;
	// testing paging
	/*uint8_t* valid_ptr = (uint8_t*)0xB8FFF;
	printf("%x, %x\n", valid_ptr, *valid_ptr);
	printf("valid video ptr dereferenced!\n");
	
	uint8_t* valid_ptr2 = (uint8_t*)0x007FFFFF;
	printf("%x, %x\n", valid_ptr2, *valid_ptr2);
	printf("valid kernal ptr dereferenced!\n");*/
	
	/*uint8_t string[129]="This is a test.";
	for(i=0;i<10;i++){
		terminal_write(NULL,string,10-i);
	}
	scroll_screen();
	terminal_read(NULL,string,128);
	printf("%d\n", term.screen_x);
	printf("%d\n", term.screen_y);*/

	//clear();
	/*uint8_t string[129]="This is a test.";
	while(1){
		terminal_read(NULL,string,128);
		putc('\n');
		//printf("%d\n", term.screen_x);
		//printf("%d\n", term.screen_y);
	}*/
	//clear();
	/*printf("test rtc methods... \n");
	int rtc_freq=8;
	int rtc_test_counter=0;
	for (rtc_test_counter=0;rtc_test_counter<1;rtc_test_counter++)
	{
		int counter=0;	// for display number on display
		rtc_freq=rtc_freq*2;
		printf("test write_rtc in %d Hz: \n",rtc_freq);
		
		int flag=write_rtc(NULL,NULL,rtc_freq);		//check whether writing is successful
		//write_rtc failed
		if(flag==-1){
		disable_irq(8);
		printf("set failed!\n");
		}
		//write successful
		while(1){
		if(!read_rtc(NULL,NULL,NULL))
			//printf("%d ",counter);

			printf("==");
			counter++;
			if (counter==15) 
			{
				printf("\n");
				break;
			}
		}

	}*/
	/*int counter = 0;
	printf("boot success!\n");
	while(counter < 10000)
		counter++; // spin*/
	file_system_init(NULL);
	init_schedule();
	/*dentry_t temp1;
	// test read_dentry_by_name
	uint8_t input[32]="frame1.txt";
	if(!read_dentry_by_name (input, &temp1))
	{	
		printf("filename: ");
		for(i = 0; i < 32; i++)
			printf("%c", temp1.filename[i]);
		printf("\n");
		printf("filetype: %d\n", temp1.filetype);
		printf("inode_num: %d\n\n", temp1.inode_num);
	}
	else printf("search failed!\n");*/
	
	// test read_dentry_by_index
	/*dentry_t temp2;
	if(!read_dentry_by_index (0, &temp2))
	{	
		printf("filename: ");
		for(i = 0; i < 32; i++)
			printf("%c", temp2.filename[i]);
		printf("\n");
		printf("filetype: %d\n", temp2.filetype);
		printf("inode_num: %d\n\n", temp2.inode_num);
	}
	else printf("search failed!\n");
	*/
	// test read_data
	/*uint8_t buff[4000];
	int rtv = read_data(16, 5222, buff, 20);
	if(rtv == -1)
		printf("read_data invalid!\n\n");
	else
	{
		printf("%d bytes of data read\n", rtv);
		i=0;
		while(buff[i])
		{
			printf("%c", buff[i]);
			i++;
		}
		printf("\n");
	}*/
	/*clear();
	if(directory_read(NULL, NULL, NULL))
		printf("read directory failed!\n");
	
	printf("test program memory page\n");
	uint8_t* valid_ptr2 = (uint8_t*)0x08048000;
	printf("0x%x, %x\n", valid_ptr2, *valid_ptr2);
	printf("valid program ptr dereferenced!\n");
	*/
	/*if(1){
		terminal_read(NULL,string,128);
		terminal_write(0, string, strlen(string));
		//putc('\n');
		//printf("%d\n", term.screen_x);
		//printf("%d\n", term.screen_y);
	}*/
	setup_terms();
	curr_term = 0;
	active_root_shell=0;
	//uint8_t* trial1="frame0.txt";
	//uint8_t* trial2="shell";

	init_user_group();
	//prompt_login();
	//clear();
	execute ((uint8_t*)"shell"); // Enter the first shell
	execute ((uint8_t*)"shell"); // Enter the first shell
	pcb_status_array[0]=0;
	//while(1);
	flush_tlb(0);
	for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
		*(uint8_t *)(VIDEO + (i << 1)) = ' ';
		*(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
	}
	reset_cursor();
	execute ((uint8_t*)"shell"); // Enter the first shell

	//ret=execute (string);
	//printf("%d\n",ret );
	//uint32_t* invalid_ptr = (uint32_t*)0x00000000;
	
	//printf("%x, %x\n", invalid_ptr, *invalid_ptr);
	//printf("page fault not generated!\n");
	
/*	page_directory_entry* pd_ptr = page_directory;
	printf("page directory information\n");
	printf("%x\n", pd_ptr);
*/

	
	
	
	/* Execute the first program (`shell') ... */
	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}
