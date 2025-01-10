/* ext2p
 * Block group
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bgdescriptor.h"
#include "dir.h"
#include "disk.h"
#include "fault.h"
#include "inode.h"
#include "superblock.h"

#include "bg.h"

bool bgRead(BlockGroup *bg, Disk *disk) {
	if( !sbRead(&bg->sb, disk) ) {
		return false;
	}

	const uint32_t BLOCK_SIZE = (1024 << bg->sb.logBlockSize);

	uint32_t blockCount = BLOCK_SIZE / 32;
	bg->bgtable = malloc(sizeof(*bg->bgtable) * blockCount);
	if( !bgtableRead(blockCount, bg->bgtable, disk) ) {
		return false;
	}

	/* Skip over reserved GDT blocks */
	for( uint16_t i = 0; i < bg->sb.reservedGDT; ++i ) {
		diskSkip(disk, BLOCK_SIZE);
	}

	bg->blockBitmap = malloc(BLOCK_SIZE);
	diskCopy(disk, bg->blockBitmap, BLOCK_SIZE);

	bg->inodeBitmap = malloc(BLOCK_SIZE);
	diskCopy(disk, bg->inodeBitmap, BLOCK_SIZE);

	bg->inodes = malloc(sizeof(*bg->inodes) * bg->sb.inodesPerGroup);
	for( uint32_t i = 0; i < bg->sb.inodesPerGroup; ++i ) {
		inodeRead(&bg->inodes[i], disk);
	}

	bg->data = diskClone(disk);
	bg->blocksRead = diskGetPos(disk) / BLOCK_SIZE;

	return true;
}

bool bgReadAll(BlockGroup *bgs, Disk *disk) {
	(void)bgs;
	(void)disk;

	return true;
}

void bgFree(BlockGroup *bg) {
	free(bg->blockBitmap);
	free(bg->inodeBitmap);
	free(bg->inodes);
	free(bg->data);
	free(bg->bgtable);
}

void bgFreeAll(BlockGroup *bgs, size_t count) {
	for( size_t i = 0; i < count; ++i ) {
		bgFree(&bgs[i]);
	}

	free(bgs);
}

Dir *bgGetDir(BlockGroup *bg, uint32_t inodenum) {
	inodenum = (inodenum - 1) % bg->sb.inodesPerGroup;
	Inode *inode = &bg->inodes[inodenum];

	if( (inode->mode & INODE_FM_DIR) == 0 ) {
		ERR("tried to get contents of non-directory inode");
		return NULL;
	}

	diskSeek(bg->data, bgOffsetBlock(bg, inode->block[0]));
	return dirReadLinkedList(bg->data, (1024 << bg->sb.logBlockSize));
}

uint32_t bgOffsetBlock(BlockGroup *bg, uint32_t block) {
	return (block - bg->blocksRead) * (1024 << bg->sb.logBlockSize);
}
