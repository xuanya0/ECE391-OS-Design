#ifndef _USER_H_
#define _USER_H_
#include "lib.h"
#include "types.h"

extern uint8_t loging_password;


typedef struct user_t
{
	uint8_t* username;
	uint8_t* password;
	uint8_t authrization;
}user_t;

user_t users[2];

void init_user_group();

void prompt_login();

//void logout();





#endif
