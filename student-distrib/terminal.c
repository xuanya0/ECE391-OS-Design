#include "terminal.h"
#include "i8259.h"
#include "schedule.h"


#define NUM_COLS 80
#define NUM_ROWS 24
#define VIDEO 0xB8000
#define VIDEO_FOR_READ 0x084B8000
#define VIDEO_MEMORY_IN_PTE 0xB8
#define VIDEO_MEM_PTE_INDEX 0xB8
#define SCREEN_SIZE 4096

file_ops_table_t terminal_ops_table =
{
	.open = (void*)terminal_open,
	.read = (void*)terminal_read,
	.write = (void*)terminal_write
};

char* video_mem=(char*)VIDEO;

uint32_t caps_lock_on = 0;
uint32_t l_shift_on = 0;
uint32_t r_shift_on = 0;
uint32_t ctrl_ready = 0;
uint32_t alt_on = 0;

uint8_t characters[122]=
{
	 0 , 0 ,'1','2','3','4','5','6','7','8','9','0','-', // 0-12
	'=', 8 ,146,'q','w','e','r','t','y','u','i','o','p', // 13-25
	'[',']', 10, 0 ,'a','s','d','f','g','h','j','k','l', // 26-38
	';', 39,'`', 0 , 92,'z','x','c','v','b','n','m',',', // 39-51
	'.','/', 0 ,'*', 0 ,' ', 0 , F1, F2, F3,'!','@','#', // 52-64
	'$','%','^','&','*','(',')','_','+', 8 ,146,'Q','W', // 65-77
	'E','R','T','Y','U','I','O','P','{','}','|', 0 ,'A', // 78-90
	'S','D','F','G','H','J','K','L',':', 34,'~', 0 ,124, // 91-103
	'Z','X','C','V','B','N','M','<','>','?', 0 ,'*', 0 , // 104-116
	' ', 0 , 0 , 0 , 0
};

void setup_terms()
{
	int i;
	for(i = 0; i < MAX_TERM; i++)
	{
		terms[i].pid = i;
		terms[i].screen_x = 0;
		terms[i].screen_y = 0;
	}
}

void switch_term(uint8_t to_term)
{
	if (term_on_screen==to_term)
		return;
	outb(0x0C,0x3D4);
	uint8_t output=0x10*to_term;
	outb(output,0x3D5);
	outb(0x0D,0x3D4);
	outb(0,0x3D5);
	term_on_screen=to_term;
	if (terminal[to_term].active==0)
		terminal[to_term].active = 1;
	/*uint8_t to_pid=terms[to_term].pid;
	pcb_t* to_pcb = (pcb_t*)pcb_ptr_array[to_pid];
	if(to_pcb->shell_flag){
		terminal[to_term].pid = to_pid;
		terminal[to_term].term = to_term;
		terminal[to_term].next = task_list.head;
		task_list.tail->next = &terminal[to_term];
		task_list.tail = &terminal[to_term];
	}*/
	update_cursor();
}


// directly use the inpack as the index, if any shift button is pressed,
// add 60 to it to get the upper case, when buttons like shift, capslock are
// pressed, do not look up in the table, just change the flags and return

uint8_t kbd_to_char(uint32_t inpack);

/*
 *	terminal_open
 *	PARAMETER:	const uint8_t* filename
 *	DESCRIPTION:	do nothing
 *	OUTPUT:	return 0
 */

int32_t
terminal_open(const uint8_t* filename)
{
	/*clear();
	puts("Terminal Opened...\n");
	terms[curr_term].screen_x=0;
	terms[curr_term].screen_y=0;
	update_cursor();
	*/
	return 0;
}


/*
 *	terminal_read
 *	PARAMETER:	int32_t fd
 *				uint8_t* buf
 *				int32_t cnt
 *	DESCRIPTION:	read user input in the terminal until an enter
 *					is pressed
 *	OUTPUT:	return the size of the input read
 */
int32_t
terminal_read(int32_t fd, uint8_t* buf, int32_t cnt)
{
	int i;
	terms[curr_term].terminal_read_on = 1;// toggle terminal read flag 
	
	terms[curr_term].terminal_cnt = 0;//reset terminal buffer and counter
	for(i=0; i<128; i++){
		terms[curr_term].terminal_buf[i] = ' ';
	}
	
	sti();
	while(terms[curr_term].terminal_read_on==1){}
	cli();
	if(terms[curr_term].terminal_cnt<=cnt || terms[curr_term].terminal_cnt<128){
		memcpy(buf,terms[curr_term].terminal_buf,terms[curr_term].terminal_cnt);
		//*(buf + terms[curr_term].terminal_cnt) = '\0';
		memset((void *)(buf + terms[curr_term].terminal_cnt),'\0',1);
		memset((void *)terms[curr_term].terminal_buf,'\0',128);
		return terms[curr_term].terminal_cnt;
	}
	else {
		memcpy(buf,terms[curr_term].terminal_buf,cnt);
		//*(buf + cnt) = '\0';
		memset((void *)(buf + cnt),'\0',1);
		memset((void *)terms[curr_term].terminal_buf,'\0',128);
		return cnt;
	}
}


/*
 *	terminal_write
 *	PARAMETER:	int32_t fd
 *				uint8_t* buf
 *				int32_t cnt
 *	DESCRIPTION:	write to the terminal
 *	OUTPUT:	return 0 upon success
 */


int32_t
terminal_write(int32_t fd, uint8_t* buf, int32_t cnt)
{
	cli();
	int i;
	for(i=0; i<cnt; i++){
		/*if (terms[term_on_screen].pid!=curr_pid)
		{
			putc_off(*(buf+i));
		}
		else*/ putc(*(buf + i));
	}
	sti();
	//asm volatile("nop;");
	return 0;
}

int32_t terminal_close(const uint8_t* filename)
{
	return 0;
}


extern void kbd(void)
{
	cli();
	uint32_t inpack;
	int offset;
	inpack=inb(0x60);
	uint8_t input;
	input=kbd_to_char(inpack);
	if(input == 255)
	{
		terms[term_on_screen].terminal_read_on = 0;
		send_eoi(1);
		sti();
		asm ("leave;\
		 	 iret;");
	}
	if(input!=NULL && terms[term_on_screen].terminal_read_on==1){
	
	
		if(input == 0x0A){// if 'Enter' is pressed 
			terms[term_on_screen].screen_y+=1;
			terms[term_on_screen].screen_x=0;
			if(terms[term_on_screen].screen_y==NUM_ROWS){
				terms[term_on_screen].screen_y-=1;
				scroll_screen();
			}	
			terms[term_on_screen].terminal_read_on = 0;
			update_cursor();
		}
	
		
		else if(input== 0x08)// if 'Backspace' is pressed 
		{
			if (terms[term_on_screen].terminal_cnt)
			{
				terms[term_on_screen].terminal_buf[terms[term_on_screen].terminal_cnt] = '\0';
				terms[term_on_screen].terminal_cnt--;
				offset=terms[term_on_screen].screen_x-1+terms[term_on_screen].screen_y*NUM_COLS;
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1)) = '\0';
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1) + 1) = 7;
				if (terms[term_on_screen].screen_x==0 && terms[term_on_screen].screen_y!=0)
				{
					terms[term_on_screen].screen_x=NUM_COLS-1;
					if(terms[term_on_screen].screen_y){terms[term_on_screen].screen_y-=1;}
				}
				else
				{
					terms[term_on_screen].screen_x-=1;
				}
				update_cursor();
			}
		}
		else if(terms[term_on_screen].terminal_cnt<=(BUF_SIZE - 1)){
			terms[term_on_screen].terminal_buf[terms[term_on_screen].terminal_cnt]=input;// copy local terminal buffer to passed in buffer	
			terms[term_on_screen].terminal_cnt++;
			terms[term_on_screen].screen_x+=1;
			if(terms[term_on_screen].screen_x==80)
			{
				terms[term_on_screen].screen_x=0;
				terms[term_on_screen].screen_y+=1;
				if (terms[term_on_screen].screen_y==NUM_ROWS)
				{
					terms[term_on_screen].screen_y-=1;
					scroll_screen();
				}
			}
			update_cursor();
			offset=terms[term_on_screen].screen_x-1+terms[term_on_screen].screen_y*NUM_COLS;
			if(!loging_password) // dont show characters when user is inputing password
			{
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1)) = input;
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1) + 1) = 7;
			}
			else
			{
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1)) = ' ';
				*(uint8_t *)(VIDEO_FOR_READ + 0x2000*term_on_screen + (offset << 1) + 1) = 7;
			}
		}
		if (terms[term_on_screen].terminal_cnt==BUF_SIZE){
			if(terms[term_on_screen].screen_y <NUM_ROWS-1){terms[term_on_screen].screen_y+=1;}
			else
			{
				scroll_screen();
			}
			terms[term_on_screen].screen_x = 0;
			update_cursor();
			terms[term_on_screen].terminal_read_on = 0;
		}
	}
	send_eoi(1);
	sti();
	asm ("leave;\
		  iret;");
}


uint8_t kbd_to_char(uint32_t inpack)
{
	int i;
	switch (inpack)
	{
		case 58:
			/* caps lock switch 58/186 */
			if(caps_lock_on){caps_lock_on = 0;}
			else{caps_lock_on = 1;}
			//printf("caps_lock_on = %d\n", caps_lock_on );
			return NULL;
		case 42:
			/* left shift pressed */
			l_shift_on = 1;
			//printf("l_shift_on = %d\n", l_shift_on);
			return NULL;
		case 170:
			/* left shift released */
			l_shift_on = 0;
			return NULL;
		case 54:
			/* right shift pressed */
			r_shift_on = 1;
			return NULL;
		case 182:
			/* right shift released */
			r_shift_on = 0;
			return NULL;
		case 29:
			/* left control ready */
			/* right control pressed */
			ctrl_ready = 1;
			return NULL;
		case 157:
			/* left control done */
			/* right control released */
			ctrl_ready = 0;
			return NULL;
		case 91:
			/* left windows 224/91 224/219 */
			return NULL;
		case 56:
			/* left alt 56 184 */
			/* right alt 224/56 224/184 */
			/* alt pressed */
			alt_on = 1;
			return NULL;
		case 184:
			/* alt released */
			alt_on = 0;
			return NULL;
		case 92:
			/* right windows 224/92 224/220 */
			return NULL;
		case 93:
			/* unknown button 224/93 224/221 */
			return NULL;
		case 38:
			if(ctrl_ready){
				clear_key();
				for(i=0; i<128; i++){
					terms[term_on_screen].terminal_buf[i] = ' ';
				}
				terms[term_on_screen].terminal_cnt = 0;
				return 255;
			}
		case 46:
			if(ctrl_ready)
			{
				send_eoi(1);
				if (curr_pid>2)
				{
					while(term_on_screen!=curr_term){}
					disable_irq(0);
					halt(0);
				}
				puts("'You cannot leave the matrix' -The Architect\n");
				return 255;
			}
		case 59: // alt + F1
			if(alt_on)
			{
				//puts("switch to terminal 0 to be supported\n");
				//printf("curr term: %d\n", curr_term);
				send_eoi(1);
				switch_term(0);
				return NULL;
			}
		case 60: // alt + F2
			if(alt_on)
			{
				//puts("switch to terminal 1 to be supported\n");
				//printf("curr term: %d\n", curr_term);
				send_eoi(1);
				switch_term(1);
				return NULL;
			}
		case 61: // alt + F3
			if(alt_on)
			{
				//puts("switch to terminal 2 to be supported\n");
				//printf("curr term: %d\n", curr_term);
				send_eoi(1);
				switch_term(2);
				return NULL;
			}
	}
	if(caps_lock_on && !(l_shift_on || r_shift_on)){
		if((inpack>15&&inpack<26) || (inpack>29&&inpack<39) || (inpack>43&&inpack<51))
			return characters[inpack+60];
		else
			return characters[inpack];
	}
	else if ((l_shift_on||r_shift_on) && !caps_lock_on)
	{
		return characters[inpack+60];
	}
	else{
		return characters[inpack];
	}
	return NULL;
}

void scroll_screen()
{
	int i,j;
	int offset=0;
	for (i = 1; i < NUM_ROWS; i++)
	{
		for (j=0 ; j < NUM_COLS; j++)
			*(uint8_t*)(video_mem+((offset+(i-1)*NUM_COLS+j)<<1))=*(uint8_t *)(video_mem + ((offset+i*NUM_COLS+j) << 1));
	}
	for (j = 0; j < NUM_COLS; j++)
		*(uint8_t*)(video_mem+((offset+(i-1)*NUM_COLS+j)<<1))='\0';
}
