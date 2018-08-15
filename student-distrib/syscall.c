#include "syscall.h"
#include "terminal.h"
#include "filesystem.h"
#include "paging.h"
#include "rtc.h"
#include "schedule.h"
#define VIDEO 0xB8000
#define ATTRIB 0x7
#define NUM_COLS 80
#define NUM_ROWS 24
#define VIDEO_MEMORY_IN_PTE 0xB8
#define VIDEO_MEM_PTE_INDEX 0xB8
// pid lookup array. 1:busy ; 0:free
uint8_t pcb_status_array[NUM_TOTAL_PROC]={1,0,0,0,0,0};
// pcb pointer array
uint32_t pcb_ptr_array[NUM_TOTAL_PROC]={0x7FE000,0x7FC000,0x7FA000,0x7F8000,0x7F6000,0x7F4000};


int32_t halt (uint8_t status)
{
	active_proc_num--;
	//printf("active process num: %d\n", active_proc_num);
	if(curr_pid < 3) 
	{
		printf("'You cannot leave the matrix' -The Architect\n");
		pcb_status_array[curr_pid] = 0;
		active_root_shell-=1;
		sti();
		execute((uint8_t*)"shell");
		//return status; // how about return 0?
	}
	pcb_t* curr_pcb_ptr = (pcb_t*)pcb_ptr_array[curr_pid];
	pcb_t* parent_pcb_ptr = curr_pcb_ptr->parent_pcb;
	//memset((void*)curr_pcb_ptr->args, '\0', 129);
	pcb_status_array[curr_pid] = 0;
	uint32_t shell_esp = parent_pcb_ptr->the_esp;
	uint32_t shell_ss = parent_pcb_ptr->the_ss;
	tss.ss0 = shell_ss;
	tss.esp0 = shell_esp;
	// switch to previous paging

	curr_pid = parent_pcb_ptr->the_pid;
	terms[curr_term].pid = curr_pid;
	terminal[curr_term].pid = curr_pid;
	flush_tlb(parent_pcb_ptr->the_pid);
	//printf("now, the pid is: %d\n", curr_pid);
	enable_irq(0);
	return exit_to_shell(status, curr_pid);
}

int32_t execute (const uint8_t* command)
{
	int i=0;
	int cmd_leng,arg_leng;
	uint8_t filename[32];
	char arg[128];
	disable_irq(0);
	memset((void *)filename,'\0',32);
	memset((void *)arg,'\0',128);
	cmd_leng=0;
	//printf("command is %s\n",command );
	while(command[cmd_leng]!='\0' && command[cmd_leng]!=' ')
	{
		filename[cmd_leng]=command[cmd_leng];
		cmd_leng++;
	}
	//printf("filename length = %d\n", cmd_leng);
	//printf("filename is %s\n",filename );
	arg_leng=0;
	while(command[arg_leng+cmd_leng+1]!='\0')
	{
		arg[arg_leng]=command[arg_leng+cmd_leng+1];
		arg_leng++;
	}
	//printf("arg length = %d\n", arg_leng);
	//printf("argument is %s\n",arg );
	if (cmd_leng>32 || cmd_leng==0)
	{
		printf("execute:command length illegal\n");
		//enable_irq(0);
		return -1;
	}
	if(!strncmp((int8_t*)"clear",(int8_t*)filename,5)){
		int32_t i;
		for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
			*(uint8_t *)(VIDEO + (i << 1)) = ' ';
			*(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
		}
		reset_cursor();
		enable_irq(0);
		return 0;
	}
	dentry_t curr_entry;
	int32_t ret=read_dentry_by_name(filename, &curr_entry);
	if (ret==-1)
	{
		printf("execute:no such file\n");
		//enable_irq(0);
		return -1;
	}
	uint8_t ELFheader[50];
	ret=read_data (curr_entry.inode_num, 0, ELFheader,50);
	if (ret==-1)
	{
		printf("execute:read data fail\n");
		//enable_irq(0);
		return -1;
	}
	if (ELFheader[0]!=0x7f || ELFheader[1]!='E' || ELFheader[2]!='L' || ELFheader[3]!='F')
	{
		printf("execute:not ELF\n");
		//enable_irq(0);
		return -1;
	}

	/* Fetch eip */
	uint32_t prog_eip;
	memcpy(&prog_eip, (void*)(ELFheader + 24), 4);
	//printf("%x\n",prog_eip );
	i=0;
	while(pcb_status_array[i])
		i++;
	if (i==6)
	{
		printf("MAXIMUM PROCESSES!!!\n");
		enable_irq(0);
		return 0;
	}
	flush_tlb(i);
	//printf("pid is %d\n", i);
	pcb_status_array[i]=1;
	ret=read_data(curr_entry.inode_num, 0, (uint8_t*)0x08048000, 3899392);
	if (i<3)
	{
		active_root_shell+=1;
	}
	if(ret == -1 || ret == 0)
	{
		flush_tlb(curr_pid);
		enable_irq(0);
		return -1;
	}
	pcb_t* exec_pcb=(pcb_t*)pcb_ptr_array[i];
	exec_pcb->the_pid=i;

	active_proc_num+=1;
	if (active_proc_num > 3)
	{
		exec_pcb->parent_pcb=(pcb_t*)pcb_ptr_array[curr_pid];
	}
	else
	{
		exec_pcb->parent_pcb=NULL;
	}
	curr_pid=i;
	terms[curr_term].pid = curr_pid;
	memset((void*)exec_pcb->filename, '\0', 32);
	memcpy((void*)exec_pcb->filename,(void*)filename,32);
	//printf("executing %s\n",exec_pcb->filename);
	memset((void*)exec_pcb->args, '\0', 129);
	memcpy((void*)exec_pcb->args, (void*)arg, arg_leng);
	//printf("exec_pcb argument is %s\n",exec_pcb->args);
	/* stdin */
	(exec_pcb->filesys_desc_array[0]).file_ops_table_ptr = &terminal_ops_table;
	(exec_pcb->filesys_desc_array[0]).inode_ptr = NULL;
	(exec_pcb->filesys_desc_array[0]).file_pos = 0;
	(exec_pcb->filesys_desc_array[0]).flags = 1;
	/* stdout */
	(exec_pcb->filesys_desc_array[1]).file_ops_table_ptr = &terminal_ops_table;
	(exec_pcb->filesys_desc_array[1]).inode_ptr = NULL;
	(exec_pcb->filesys_desc_array[1]).file_pos = 0;
	(exec_pcb->filesys_desc_array[1]).flags = 1;
	uint8_t temp[1];
	((exec_pcb->filesys_desc_array[1]).file_ops_table_ptr)->open(temp);
	for(i=2; i<8; i++){
		(exec_pcb->filesys_desc_array[i]).flags = 0;
	}
	//printf("active_proc_num=%d",active_proc_num);
	if(active_root_shell < 3){
		exec_pcb->shell_flag = 1;
		exec_pcb->cmd_eip = prog_eip;
		return 0;
	}
	// TSS related 
	tss.ss0 = KERNEL_DS;
	//tss.esp0 = pcb_ptr_array[curr_pid] - 0x4;
	tss.esp0 = (0x00800000 - 0x00002000*curr_pid - 0x4);
	
	if(curr_pid > 2){
		uint32_t curr_esp, curr_ss;
		asm("\t movl %%esp,%0" : "=r"(curr_esp));
		asm("\t movl %%ss,%0" : "=r"(curr_ss));
		(exec_pcb->parent_pcb)->the_esp = curr_esp;
		(exec_pcb->parent_pcb)->the_ss = curr_ss;
	}
	//printf("curr_pid %d\n",curr_pid );
	page_table[curr_pid][VIDEO_MEM_PTE_INDEX].phys_page_addr = VIDEO_MEMORY_IN_PTE+2*curr_term;
	flush_tlb(curr_pid);
	// In-line assembly to push registers onto the curr kernel stack */
	uint32_t return_value;
	if(curr_pid < 3){
		enable_irq(0);
		return_value = jump_to_program((uint32_t)(0x08400000 - 0x4), prog_eip, -1);
	}else{
		//printf("parent pid is %d\n", (exec_pcb->parent_pcb)->the_pid);
		terminal[curr_term].pid = curr_pid;
		enable_irq(0);
		enable_irq(8);
		return_value = jump_to_program((uint32_t)(0x08400000 - 0x4), prog_eip, (exec_pcb->parent_pcb)->the_pid);
	}
	//printf("end of execute\n");
	return return_value;	
}

//read syscall interface 
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
	if( fd > 7 || fd < 0 || fd==1){ return -1; }
	if (((fetch_cur_pcb_ptr())->filesys_desc_array[fd]).flags==0)
	{
		return -1;
	}
	return (((fetch_cur_pcb_ptr())->filesys_desc_array[fd]).file_ops_table_ptr)->read(fd, buf, nbytes);
}
// write syscall interface
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
	if( fd > 7 || fd < 1 ){ return -1; }
	if (((fetch_cur_pcb_ptr())->filesys_desc_array[fd]).flags==0)
	{
		return -1;
	}
	return (((fetch_cur_pcb_ptr())->filesys_desc_array[fd]).file_ops_table_ptr)->write(fd, buf, nbytes);
}

int32_t open (const uint8_t* filename)
{
	//filesys_desc_array[0] is stdin, filesys_desc_array[1] is stdout, the rest is dynamically allocated
	pcb_t* pcb = fetch_cur_pcb_ptr();
	
	// Find the corresponding directory entry
	dentry_t the_dentry;
	int32_t result = read_dentry_by_name(filename, &the_dentry);
	//named file is not found
	if(result == -1) return -1;
	 
	//file found,  Allocate a file descriptor    
	uint32_t i;		//cnt for check free file array entry
	uint32_t free_desc_found = 0;
	for(i=2; i<8; i++){
		/* Check if is free */
		if((pcb->filesys_desc_array[i]).flags == 0){
			free_desc_found = 1;
			/* Check file type */
			switch (the_dentry.filetype)
			{
				case 0:		/* real-time clock */
					(pcb->filesys_desc_array[i]).file_ops_table_ptr = &rtc_ops_table;
					(pcb->filesys_desc_array[i]).inode_ptr = NULL;
					(pcb->filesys_desc_array[i]).file_pos = 0;
					(pcb->filesys_desc_array[i]).flags = 1;
					break;
				case 1:		/* directory */
					(pcb->filesys_desc_array[i]).file_ops_table_ptr = &file_system_ops_table;
					(pcb->filesys_desc_array[i]).inode_ptr = -1;
					(pcb->filesys_desc_array[i]).file_pos = 0;
					(pcb->filesys_desc_array[i]).flags = 1;
					break;
				case 2:		/* regular file */
					(pcb->filesys_desc_array[i]).file_ops_table_ptr = &file_system_ops_table;
					(pcb->filesys_desc_array[i]).inode_ptr = the_dentry.inode_num;
					(pcb->filesys_desc_array[i]).file_pos = 0;
					(pcb->filesys_desc_array[i]).flags = 1;
					break;
				default:
					break;
			}
			break;
		}
		else continue;
	}
	
	/* Check if free descriptor found */
	if(free_desc_found){
		((pcb->filesys_desc_array[i]).file_ops_table_ptr)->open(filename);
		return i;	/* return value */
	}
	/* Otherwise, no free descriptor available */
	return -1;
	//return 0;
	//else return -1;
}
int32_t close (int32_t fd)
{
	pcb_t* cur_pcb_ptr = fetch_cur_pcb_ptr();
	//invalid fd
	if(fd < 2 || fd > 7)
		return -1;
	if (((fetch_cur_pcb_ptr())->filesys_desc_array[fd]).flags==0)
	{
		return -1;
	}
	//valid fd, set flag to be not using status
	cur_pcb_ptr->filesys_desc_array[fd].flags = 0;
	return 0;
}

int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	pcb_t* curr_pcb=(pcb_t *)pcb_ptr_array[curr_pid];
	if (nbytes<strlen((int8_t *)curr_pcb->args)+1)
		return -1;
	//printf("arg: %s\n",(int8_t *)curr_pcb->args );
	memcpy((void *)buf,(void*)curr_pcb->args,nbytes);           
	return 0;
}

int32_t vidmap (uint8_t** screen_start)
{
	if(screen_start < (uint8_t**)0x08000000 || screen_start >= (uint8_t**)0x08400000 )
		return -1;
	*screen_start = (uint8_t*)(0x084B8000+0x2000*curr_term);
		return 0;
}
int32_t set_handler (int32_t signum, void* handler_address)
{
	return -1;
}
int32_t sigreturn (void)
{
	return -1;
}

pcb_t* fetch_cur_pcb_ptr()
{
	uint32_t cur_esp;
	asm("movl %%esp,%0" 
		: "=r"(cur_esp)
		);
	return (pcb_t*)(cur_esp & 0xFFFFE000);		//get pcb address, 8kb aligned beginning of each stack
}
