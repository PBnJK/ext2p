/* ext2p
 * ext2 filesystem parser
 */

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bg.h"
#include "disk.h"
#include "superblock.h"
#include "util.h"

#include "ext2.h"

static uint32_t _inodeToBG(Ext2 *ext2, uint32_t inodenum);

Ext2 *ext2Open(const char *FILEPATH) {
	Ext2 *ext2 = malloc(sizeof(*ext2));

	ext2->disk = diskOpen(FILEPATH);
	if( ext2->disk == NULL ) {
		return NULL;
	}

	/* Skip group 0 */
	diskSkip(ext2->disk, 1024);

	/* Read first block group */
	ext2->bgs = malloc(sizeof(*ext2->bgs));
	if( !bgRead(0, ext2->bgs, ext2->disk) ) {
		return NULL;
	}

	Superblock *sb = &ext2->bgs->sb;

	const double BG_COUNT = (double)sb->blockCount / (double)sb->blocksPerGroup;
	ext2->bgCount = (size_t)ceil(BG_COUNT);
	ext2->bgs = realloc(ext2->bgs, ext2->bgCount * sizeof(*ext2->bgs));

	return ext2;
}

void ext2Free(Ext2 *ext2) {
	diskClose(ext2->disk);
	bgFreeAll(ext2->bgs, ext2->bgCount);

	free(ext2);
}

void ext2GetInode(Ext2 *ext2, uint32_t inodenum, Inode *inode) {
	uint32_t bg = _inodeToBG(ext2, inodenum);
	bgGetInode(&ext2->bgs[bg], inodenum, inode);
}

uint64_t ext2GetInodeSize(Ext2 *ext2, uint32_t inodenum, Inode *inode) {
	uint32_t bg = _inodeToBG(ext2, inodenum);
	return bgGetInodeSize(&ext2->bgs[bg], inode);
}

bool ext2GetDir(Ext2 *ext2, uint32_t inodenum, Dir *dir) {
	uint32_t bg = _inodeToBG(ext2, inodenum);
	return bgGetDir(&ext2->bgs[bg], inodenum, dir);
}

bool ext2ReadFile(Ext2 *ext2, uint32_t inodenum, FP *fp) {
	uint32_t bg = _inodeToBG(ext2, inodenum);
	return bgReadFile(&ext2->bgs[bg], inodenum, fp);
}

static uint32_t _inodeToBG(Ext2 *ext2, uint32_t inodenum) {
	return (inodenum - 1) / ext2->bgs->sb.inodesPerGroup;
}
