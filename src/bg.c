/* ext2p
 * Block group
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bgdescriptor.h"
#include "dir.h"
#include "disk.h"
#include "fault.h"
#include "inode.h"
#include "superblock.h"

#include "bg.h"

static bool _readBGTable(BlockGroup *bg, const uint32_t BLOCK_SIZE, Disk *disk);

bool bgRead(int num, BlockGroup *bg, Disk *disk) {
	if( !sbRead(&bg->sb, disk) ) {
		return false;
	}

	const uint32_t BLOCK_SIZE = (1024 << bg->sb.logBlockSize);

	if( BLOCK_SIZE != 1024 ) {
		diskSeek(disk, BLOCK_SIZE);
	}

	diskSkip(disk, num * (BLOCK_SIZE / 32));
	if( !_readBGTable(bg, BLOCK_SIZE, disk) ) {
		return false;
	}

	bg->data = diskClone(disk);
	bg->blocksRead = diskGetPos(disk) / BLOCK_SIZE;

	return true;
}

static bool
_readBGTable(BlockGroup *bg, const uint32_t BLOCK_SIZE, Disk *disk) {
	diskCopy(disk, &bg->desc, 32);

	diskSeek(disk, BLOCK_SIZE * bg->desc.blockBitmap);
	bg->blockBitmap = malloc(BLOCK_SIZE);
	diskCopy(disk, bg->blockBitmap, BLOCK_SIZE);

	diskSeek(disk, BLOCK_SIZE * bg->desc.inodeBitmap);
	bg->inodeBitmap = malloc(BLOCK_SIZE);
	diskCopy(disk, bg->inodeBitmap, BLOCK_SIZE);

	diskSeek(disk, BLOCK_SIZE * bg->desc.inodeTable);
	bg->inodes = malloc(sizeof(*bg->inodes) * bg->sb.inodesPerGroup);
	for( uint32_t i = 0; i < bg->sb.inodesPerGroup; ++i ) {
		inodeRead(&bg->inodes[i], disk);
		diskSkip(disk, bg->sb.inodeSize - 128);
	}

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
