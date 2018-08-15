#include "filesystem.h"

// variable to store information of file image. Load from boot block
int num_of_entries = 0;
int num_of_inodes = 0;
int num_of_blocks = 0;

// helper pointers pointing to the start of entries, inodes and data blocks
uint32_t inode_ptr_base;
uint32_t entry_ptr_base;
uint32_t data_block_ptr_base;

file_ops_table_t file_system_ops_table =
{
	.open = (void*)filesystem_open,
	.read = (void*)filesystem_read,
	.write = (void*)filesystem_write
};

/*
filesystem_init(const uint8_t* filename)
DESCRIPTION: set the helper pointers to point to the start of dentries, inodes
			and data blocks, get the information of file image
INPUTS: none
OUTPUTS: none
RETURN VALUE: none
 */
int32_t file_system_init(const uint8_t* filename)
{
	// points to the second block: start of inode blocks
	inode_ptr_base = filesystem_base + 4096;
	// points to the second 64 bytes in the first block: start of dentries
	entry_ptr_base = filesystem_base + 64;
	//get the number of entries, inodes and blocks
	num_of_entries = *((int*)(filesystem_base));
	//printf("num of entries: %d\n", num_of_entries);
	num_of_inodes = *((int*)(filesystem_base + 4));
	//printf("num of inodes: %d\n", num_of_inodes);
	// points to the start of data blocks: following inode blocks
	data_block_ptr_base = filesystem_base+(num_of_inodes + 1)*4096;
	num_of_blocks = *((int*)(filesystem_base + 8));
	//printf("num of blocks: %d\n", num_of_blocks);
	return 0;
}

/*
read_dentry_by_name (const uint8_t* fname, dentry_t * dentry)
DESCRIPTION: find the correct file based on filename. Copy the information from 
			the file's dentry to the dentry struct passed in.
INPUTS: const uint8_t* fname - filename, maximum 32 bytes
		dentry_t* dentry - pointer pointing to a dentry struct
OUTPUTS: none
RETURN VALUE: 0 on success, -1 on failure
SIDE EFFECT: set the dentry 
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
	int i=0; 
	// use strncmp to see if the filename matches the to be found file
	while(strncmp((int8_t*)(entry_ptr_base+64*i), (int8_t*)fname, 32) && i < num_of_entries)
		i += 1;
	if (i < num_of_entries)
	{
		// read the data from dentry in file image
		memcpy(&(dentry->filename[0]), (int8_t*)(entry_ptr_base + 64*i), 32);
		memcpy(&(dentry->filetype), (void*)(entry_ptr_base + 64*i + 32), 4);
		memcpy(&(dentry->inode_num), (void*)(entry_ptr_base + 64*i + 36), 4);
		return 0;
	}
	else return -1;
}

/*
read_dentry_by_name (const uint8_t* fname, dentry_t * dentry)
DESCRIPTION: use the index passed in to get data from the file image
INPUTS: uint32_t index - index to the target dentry
		dentry_t* dentry - pointer pointing to a dentry struct
OUTPUTS: none
RETURN VALUE: 0 on success, -1 on failure
SIDE EFFECT: set the dentry 
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t * dentry)
{
	if (index >= num_of_entries) 
		return -1;
	else
	{
		// read the data from dentry in file image
		memcpy((int8_t*)dentry->filename, (int8_t*)(entry_ptr_base + 64*index), 32);
		dentry->filetype = *((int*)(entry_ptr_base + 64*index + 32));
		dentry->inode_num = *((int*)(entry_ptr_base + 64*index + 36));
	}
	return 0;
}

/*
read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
DESCRIPTION: read the number of "length" bytes of data from the file denoted by the number of inode, starting from
			offset, and store data from the "buf" buffer
INPUTS: uint32_t inode - inode number
		uint32_t offset - starting location
		uint8_t* buf - buffer the data to be stored to
		uint32_t length - length of data to be copied
		dentry_t* dentry - pointer pointing to a dentry struct
OUTPUTS: none
RETURN VALUE: -1 if bad block number, number of bytes copied
SIDE EFFECT: data stored to "buf" 
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	// find the correct inode using the inode number passed in
	uint32_t inode_ptr = inode_ptr_base + inode*4096;
	
	// get the length of the file
	int filelength = *((int*)inode_ptr);
	//printf("file length: %d\n", filelength);
	
	// invalid parameters
	if(offset > filelength) //|| length > filelength)
		return 0;
	else
	{
		if (length>filelength)
			length=filelength;
		// find the correct starting block number from inode
		int block_number = *((int *)(inode_ptr + ((offset/4096) + 1)*4));
		if(block_number > num_of_blocks) // bad block number
			return -1;
		// points to the correct data block
		uint32_t block_ptr = data_block_ptr_base + block_number*4096;
		if(length == 0) return 0;
		else 
		{
			int offset_temp = offset % 4096;
			//printf("offset mod: %d\n", offset_temp);
			int i = 0; int k = 1; 
			//int bytes_to_be_copied = length; // for debugging
			while(i < length && offset + i < filelength)
			{
				memcpy((buf + i), (int8_t*)(block_ptr + offset_temp), 1);
				i++; offset_temp++;
				// if the end of a data block is reached while copy is not completed,
				// find the next data block of this file from inode
				if(offset_temp > 4095)
				{
					//printf("reached the end of a block. Switch to another one.\n");
					k++; // use k to keep track of data block number in inode
					// switch to the next block
					block_number = *((int *)(inode_ptr + ((offset/4096) + k)*4));
					block_ptr = data_block_ptr_base + (block_number)*4096;
					//bytes_to_be_copied -= i; 
					//printf("bytes to be copied after switching block %d\n", bytes_to_be_copied);
					offset_temp = 0; 
				}	
			}
			// return the bytes copied
			return i;
		}
	}
}

int32_t directory_read(int32_t fd, uint8_t* buf, int32_t cnt)
{
	uint32_t entry_ptr = entry_ptr_base;
	int i, j;
	for(i = 0; i < num_of_entries; i++)
	{
		for(j = 0; j < 32; j++)	
			putc(*((int8_t*)(entry_ptr + j)));
		putc('\n');
		entry_ptr += 64;
	}
	update_cursor();
	return 0;
}


int32_t filesystem_read(int32_t fd, uint8_t* buf, int32_t cnt)
{

	uint32_t curr_esp;
	asm(" movl %%esp,%0" : "=r"(curr_esp));//get the esp value
	
	pcb_t* curr_pcb = (pcb_t*)(curr_esp & 0xFFFFE000);//pcb points to corresponding location
	if((curr_pcb->filesys_desc_array[fd]).inode_ptr != -1)
	{
		//it is regular file 
		uint32_t offset = (curr_pcb->filesys_desc_array[fd]).file_pos;
		uint32_t inode = (curr_pcb->filesys_desc_array[fd]).inode_ptr;
		int32_t ret_val = read_data(inode, offset, buf, cnt);
		if(ret_val != -1)
			(curr_pcb->filesys_desc_array[fd]).file_pos += ret_val;
		return ret_val;	
	}
	//else the inode is a pointer?
	else
	{
		if((curr_pcb->filesys_desc_array[fd]).file_pos < num_of_entries){
			/* fp is valid */
			uint32_t fp = (curr_pcb->filesys_desc_array[fd]).file_pos;
			/* Check the filename's length */
			uint32_t fn_len = strlen((int8_t*)(entry_ptr_base + 64*fp));
			memcpy(buf, (void*)(entry_ptr_base + 64*fp), fn_len);
			(curr_pcb->filesys_desc_array[fd]).file_pos ++;
			return fn_len;
		}else{
			(curr_pcb->filesys_desc_array[fd]).file_pos = 0;
		}
		return 0;
	}
	return 0;
}
/*
	file_system_write:
	parameter: filename: useless
	output: -1: we cannot do file system write
	descriptor: we cannot write to the file in file system, just return -1
*/
int32_t filesystem_write(int32_t fd, uint8_t* buf, int32_t cnt)
{
	return -1;
}
/*
	file_system_open:
	parameter: filename: useless
	output: -1: we cannot do file system open
	descriptor: we cannot open file system, just return -1
*/
int32_t filesystem_open(const uint8_t* filename)
{
	return -1;
}
/*
	file_system_close:
	parameter: filename: useless
	output: 
	descriptor:
*/
int32_t filesystem_close(const uint8_t* filename)
{
	return 0;
}
