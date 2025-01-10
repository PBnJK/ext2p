/* ext2p
 * Directory
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dir.h"
#include "disk.h"

Dir *dirNew(void) {
	Dir *dir = malloc(sizeof(*dir));
	return dir;
}

Dir *dirReadLinkedList(Disk *disk, uint32_t blockSize) {
	Dir *dir = dirNew();

	Dir *prev = dir;
	Dir *curr = dir;

	uint32_t sentinel = 0;
	while( true ) {
		uint32_t ino = diskRead32(disk);
		if( ino == 0 ) {
			free(prev->next);
			prev->next = NULL;
			break;
		}

		curr->inode = ino;
		curr->nextEntry = diskRead16(disk);
		curr->nameLen = diskRead8(disk);
		curr->filetype = diskRead8(disk);

		curr->filename = malloc(curr->nameLen + 1);
		for( uint8_t i = 0; i < curr->nameLen; ++i ) {
			curr->filename[i] = diskRead8(disk);
		}
		curr->filename[curr->nameLen] = '\0';

		sentinel += curr->nextEntry;
		if( sentinel >= blockSize ) {
			curr->next = NULL;
			break;
		}

		diskRewind(disk, 8 + curr->nameLen);
		diskSkip(disk, curr->nextEntry);

		curr->next = dirNew();

		prev = curr;
		curr = curr->next;
	}

	return dir;
}

void dirFreeLinkedList(Dir *dir) {
	if( dir == NULL ) {
		return;
	}

	Dir *next = dir->next;
	while( true ) {
		free(dir->filename);
		free(dir);
		dir = next;

		if( dir == NULL ) {
			return;
		}
		next = dir->next;
	}
}

char *dirGetFiletype(Dir *dir) {
	switch( dir->filetype ) {
	case DIR_FT_UNKNOWN:
		return "unknown";
	case DIR_FT_FILE:
		return "file";
	case DIR_FT_DIR:
		return "dir";
	case DIR_FT_CHAR_DEV:
		return "chrdev";
	case DIR_FT_BLOCK_DEV:
		return "blkdev";
	case DIR_FT_FIFO:
		return "buffer";
	case DIR_FT_SOCKET:
		return "socket";
	case DIR_FT_SYMLINK:
		return "symlink";
	}

	return "invalid";
}
