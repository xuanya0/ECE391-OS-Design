#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include "lib.h"
#include "x86_desc.h"
//#include "sys_call.h"

#define NUM_TOTAL_PROC 6
#define BUF_SIZE 128
/* define terminal structure */
#define NEW_ESP 0x083FFFFC
#define NEW_EFLAGS 512
#define MAX_TERM 3

extern uint8_t pcb_status_array[NUM_TOTAL_PROC];

extern uint32_t pcb_ptr_array[NUM_TOTAL_PROC];

typedef struct
{
	uint8_t pid;
	uint8_t terminal_buf[BUF_SIZE];
	int32_t terminal_cnt;
	volatile uint32_t terminal_read_on;
	int screen_x;
	int screen_y;
} term_t;

// define terminal structure 
term_t terms[3];
//term_t term;

// current terminal 
volatile uint8_t curr_term;

// terminal being displayed 
volatile uint8_t term_on_screen;

// current pid 
volatile uint8_t curr_pid;

// total process running 
uint8_t active_proc_num;

// total root shell 
uint8_t active_root_shell;

typedef struct file_ops_table
{
	int32_t (*open)(const uint8_t*);
	int32_t (*read)(int32_t, void*, int32_t);
	int32_t (*write)(int32_t, const void*, int32_t);
} file_ops_table_t;

typedef struct filesys_desc
{
	file_ops_table_t* file_ops_table_ptr;
	uint32_t inode_ptr;
	uint32_t file_pos;
	uint32_t flags;
} filesys_desc_t;

typedef struct pcb
{
	filesys_desc_t filesys_desc_array[8];
	uint8_t args[129];
	uint32_t the_esp;
	uint32_t the_ss;
	struct pcb* parent_pcb;
	uint32_t the_pid;
	uint32_t shell_flag;
	uint32_t great_esp;
	uint32_t cmd_eip;
	uint8_t filename[32];
} pcb_t;

pcb_t* fetch_cur_pcb_ptr();

int32_t halt (uint8_t status); 
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** screen_start);
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);

extern uint32_t jump_to_program(uint32_t new_ebp, uint32_t cmd_eip, int32_t save_pid);

extern void sys_call_handler();
uint32_t jump_to_program(uint32_t new_ebp, uint32_t cmd_eip, int32_t save_pid);

extern int32_t exit_to_shell(uint8_t status, uint8_t pid);

#endif 
