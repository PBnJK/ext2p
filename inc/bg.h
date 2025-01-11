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
#include "util.h"

typedef struct _BlockGroup {
	Superblock sb;
	BlockGroupDescriptor desc;

	char *blockBitmap;
	char *inodeBitmap;
	Inode *inodes;

	Disk *data;
	uint32_t blocksRead;
} BlockGroup;

bool bgRead(int num, BlockGroup *bg, Disk *disk);
bool bgReadAll(BlockGroup *bgs, Disk *disk);

void bgFree(BlockGroup *block);
void bgFreeAll(BlockGroup *blocks, size_t count);

void bgGetInode(BlockGroup *bg, uint32_t inodenum, Inode *inode);
uint64_t bgGetInodeSize(BlockGroup *bg, Inode *inode);

bool bgGetDir(BlockGroup *bg, uint32_t inodenum, Dir *dir);
bool bgReadFile(BlockGroup *bg, uint32_t inodenum, FP *fp);

void bgDeleteFile(BlockGroup *bg, Dir *root, Dir *dir);
void bgDeleteDir(BlockGroup *bg, Dir *root, Dir *dir);

uint32_t bgOffsetBlock(BlockGroup *bg, uint32_t block);

#endif // !GUARD_EXT2_BLOCK_H_
