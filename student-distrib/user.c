#include "user.h"
//#include "syscall.h"
#include "terminal.h"
#include "i8259.h"
uint8_t loging_password;

uint8_t name_buf[128]; 
uint8_t password_buf[128]; 

void init_user_group()
{
	users[0].username = (uint8_t*)"user";
	users[0].password = (uint8_t*)"ece391";
	users[0].authrization = 0; // need authorization for running program
	users[1].username = (uint8_t*)"root";
	users[1].password = (uint8_t*)"ece391root";	
	users[1].authrization = 1;
	loging_password = 0;
}

void prompt_login()
{
	printf("user_login\nUSERNAME: ");
	terminal_read(NULL, name_buf, 128);
	if(!strncmp((int8_t*)(users[0].username), (int8_t*)name_buf, 4) 
		|| !strncmp((int8_t*)(users[1].username), (int8_t*)name_buf, 4))
	{
		loging_password = 1;
		printf("PASSWORD: ");
		terminal_read(NULL, password_buf, 128);
		if((!strncmp((int8_t*)(users[0].password), (int8_t*)password_buf, strlen((int8_t*)(users[0].password))) 
			|| !strncmp((int8_t*)(users[1].password), (int8_t*)password_buf, strlen((int8_t*)(users[1].password))))
			&& ((strlen((int8_t*)(users[0].password)) == (strlen((int8_t*)password_buf)))
			|| (strlen((int8_t*)(users[1].password)) == (strlen((int8_t*)password_buf)))))
		{
			loging_password = 0;
			printf("login successful!\n");
		}
		else
		{
			printf("password incorrect!\n");
			loging_password = 0;
			//prompt_login();
		}
	}
	else
	{
		printf("invalid username\n");
		//prompt_login();
	}
	int i = 120000000;
	while(i)
		i--;
}

/*void logout()
{
}*/
