#ifndef GUARD_EXT2_DIR_H_
#define GUARD_EXT2_DIR_H_

#include <stdbool.h>
#include <stdint.h>

#include "disk.h"

typedef enum _Dir_Filetype {
	DIR_FT_UNKNOWN = 0, /* Unknown */
	DIR_FT_FILE = 1, /* File */
	DIR_FT_DIR = 2, /* Directory */
	DIR_FT_CHAR_DEV = 3, /* Character device */
	DIR_FT_BLOCK_DEV = 4, /* Block device */
	DIR_FT_FIFO = 5, /* Buffer */
	DIR_FT_SOCKET = 6, /* Socket */
	DIR_FT_SYMLINK = 7, /* Symbolic link */
} Dir_Filetype;

typedef struct _Dir {
	uint32_t offset;

	uint32_t inode;
	uint16_t nextEntry;

	uint8_t nameLen;
	uint8_t filetype;
	char *filename;

	struct _Dir *next;
} Dir;

Dir *dirNew(void);

void dirReadLinkedList(Disk *disk, uint32_t blockSize, Dir *dir);
void dirFreeLinkedList(Dir *dir);

char *dirGetFiletype(Dir *dir);

#endif // !GUARD_EXT2_DIR_H_
