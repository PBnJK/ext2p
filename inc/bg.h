#ifndef GUARD_EXT2_BLOCK_H_
#define GUARD_EXT2_BLOCK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bgdescriptor.h"
#include "dir.h"
#include "disk.h"
#include "inode.h"
#include "superblock.h"

typedef struct _BlockGroup {
	Superblock sb;
	BlockGroupDescriptor *bgtable;

	char *blockBitmap;
	char *inodeBitmap;

	Inode *inodes;
	Disk *data;

	uint32_t blocksRead;
} BlockGroup;

bool bgRead(BlockGroup *bg, Disk *disk);
bool bgReadAll(BlockGroup *bgs, Disk *disk);

void bgFree(BlockGroup *block);
void bgFreeAll(BlockGroup *blocks, size_t count);

Dir *bgGetDir(BlockGroup *bg, uint32_t inodenum);

uint32_t bgOffsetBlock(BlockGroup *bg, uint32_t block);

#endif // !GUARD_EXT2_BLOCK_H_
