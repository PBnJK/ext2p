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

#include "ext2.h"

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

Dir *ext2GetDir(Ext2 *ext2, uint32_t inodenum) {
	uint32_t bg = (inodenum - 1) / ext2->bgs->sb.inodesPerGroup;
	return bgGetDir(&ext2->bgs[bg], inodenum);
}
