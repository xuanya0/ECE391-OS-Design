#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "lib.h"
#include "syscall.h"
// a directory entry structure.
typedef struct dentry_t
{
	uint8_t filename[32];
	int filetype;
	int inode_num;
	uint8_t reserved[24];
} dentry_t;

// pointer to the base of file image
uint32_t filesystem_base;





int32_t read_dentry_by_name (const uint8_t* fname, dentry_t * dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t * dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf,uint32_t length);

int32_t file_system_init(const uint8_t* filename);

int32_t filesystem_open(const uint8_t* filename);

int32_t directory_read(int32_t fd, uint8_t* buf, int32_t cnt);

int32_t filesystem_read(int32_t fd, uint8_t* buf, int32_t cnt);

int32_t filesystem_write(int32_t fd, uint8_t* buf, int32_t cnt);

int32_t filesystem_close(const uint8_t* filename);

extern file_ops_table_t file_system_ops_table;

#endif
