#ifndef GUARD_EXT2_BGDESCRIPTOR_H_
#define GUARD_EXT2_BGDESCRIPTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "disk.h"

typedef struct _BlockGroupDescriptor {
	uint32_t blockBitmap; /* ID of the first block of the block bitmap */
	uint32_t inodeBitmap; /* ID of the first block of the inode bitmap */
	uint32_t inodeTable; /* ID of the first block of the inode table */

	uint16_t freeBlocks; /* Free blocks in this group */
	uint16_t freeInodes; /* Free inodes in this group */

	uint16_t dirInodes; /* Number of inodes allocated to dirs in this group */

	uint16_t padding;
	uint8_t reserved[12];
} BlockGroupDescriptor;

bool bgtableRead(uint32_t count, BlockGroupDescriptor *bgtable, Disk *disk);

#endif // !GUARD_EXT2_BGDESCRIPTOR_H_
