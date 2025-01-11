#ifndef GUARD_EXT2P_EXT2_H_
#define GUARD_EXT2P_EXT2_H_

#include <stdint.h>

#include "bg.h"
#include "dir.h"
#include "disk.h"
#include "inode.h"
#include "util.h"

/* Structure representing an ext2 filesystem */
typedef struct _Ext2 {
	Disk *disk;

	size_t bgCount;
	BlockGroup *bgs;
} Ext2;

Ext2 *ext2Open(const char *FILEPATH);
void ext2Free(Ext2 *ext2);

void ext2GetInode(Ext2 *ext2, uint32_t inodenum, Inode *inode);
uint64_t ext2GetInodeSize(Ext2 *ext2, uint32_t inodenum, Inode *inode);

bool ext2GetDir(Ext2 *ext2, uint32_t inodenum, Dir *dir);
bool ext2ReadFile(Ext2 *ext2, uint32_t inodenum, FP *fp);

bool ext2DeleteFile(Ext2 *ext2, Dir *root, Dir *file);
bool ext2DeleteDir(Ext2 *ext2, Dir *root, Dir *dir);

void ext2SaveToFile(Ext2 *ext2, const char *FILEPATH);

#endif // GUARD_EXT2P_EXT2_H_
