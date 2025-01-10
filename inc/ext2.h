#ifndef GUARD_EXT2P_EXT2_H_
#define GUARD_EXT2P_EXT2_H_

#include <stdint.h>

#include "bg.h"
#include "disk.h"

/* Structure representing an ext2 filesystem */
typedef struct _Ext2 {
	Disk *disk;

	size_t bgCount;
	BlockGroup *bgs;
} Ext2;

Ext2 *ext2Open(const char *FILEPATH);
void ext2Free(Ext2 *ext2);

Dir *ext2GetDir(Ext2 *ext2, uint32_t inodenum);

#endif // GUARD_EXT2P_EXT2_H_
