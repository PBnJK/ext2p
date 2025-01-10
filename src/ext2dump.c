/* ext2p
 * ext2 filesystem dumper
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bgdescriptor.h"
#include "ext2.h"
#include "inode.h"
#include "superblock.h"
#include "util.h"

#include "ext2dump.h"

#define YESNO(C) ((C) ? "yes" : "no")

#define COMPAT(C) (YESNO(sb->featuresCompat & (C)))
#define INCOMPAT(C) (YESNO(sb->featuresIncompat & (C)))
#define COMPATRO(C) (YESNO(sb->featuresReadOnly & (C)))

#define COMPRESSED(C) (YESNO(sb->compressionBitmap & (C)))

#define INOFL(C) (YESNO(ino->flags & (C)))

static void _sbDump(Superblock *sb);

static char *_sbOSDump(uint32_t os);
static char *_sbRevDump(uint32_t rev);
static char *_sbStateDump(uint32_t state);
static char *_sbErrorDump(uint32_t errors);
static void _sbMountCheckDump(uint16_t mount, uint16_t max, char out[256]);
static void _sbTimeCheckDump(int32_t check, int32_t interval, char out[256]);
static void _sbUUIDDump(uint8_t uuid[16], char out[33]);

static void _bgDump(BlockGroupDescriptor *bgdesc, int i);

static void _inoDump(Inode *ino, uint32_t rev, int i);
static void _inoUserDump(uint16_t mode, char *perms);
static void _inoGroupDump(uint16_t mode, char *perms);
static void _inoOtherDump(uint16_t mode, char *perms);

void ext2Dump(Ext2 *ext2, int flags) {
	if( flags & DUMP_SUPERBLOCK ) {
		_sbDump(&ext2->bgs->sb);
		putchar('\n');
	}

	if( flags & DUMP_BGDESCRIPTOR ) {
		for( int i = 0; i < 32; ++i ) {
			_bgDump(&ext2->bgs[i].desc, i);
			putchar('\n');
		}
	}

	if( flags & DUMP_ALL_BGDESCRIPTOR ) {
		uint32_t blockCount = (1024 << ext2->bgs->sb.logBlockSize) / 32;
		for( uint32_t i = 0; i < blockCount; ++i ) {
			_bgDump(&ext2->bgs[i].desc, i);
			putchar('\n');
		}
	}

	if( flags & DUMP_INODE ) {
		for( int i = 0; i < 32; ++i ) {
			_inoDump(&ext2->bgs->inodes[i], ext2->bgs->sb.revLevel, i);
			putchar('\n');
		}
	}

	if( flags & DUMP_INODE_ALL ) {
		for( uint32_t i = 0; i < ext2->bgs->sb.inodesPerGroup; ++i ) {
			_inoDump(&ext2->bgs->inodes[i], ext2->bgs->sb.revLevel, i);
			putchar('\n');
		}
	}

	if( flags & DUMP_INODE_ROOT ) {
		Inode *root = &ext2->bgs->inodes[INODE_RES_ROOT_DIR];
		_inoDump(root, ext2->bgs->sb.revLevel, INODE_RES_ROOT_DIR);
	}
}

static void _sbDump(Superblock *sb) {
	char uuid[33];
	char needsCheck[256];

	printf("* Superblock ");
	printf("(block #%" PRIu32 ", ", sb->firstDataBlock);
	printf("group #%" PRIu16 ")\n", sb->sbBlockGroup);
	printf("│\n");

	printf("├─ Created by (OS)... %s\n", _sbOSDump(sb->creatorOS));
	printf("├─ Revision.......... %s\n", _sbRevDump(sb->revLevel));
	printf("├─ Minor revision.... %" PRIu16 "\n", sb->minorRevLevel);
	printf("│\n");

	printf("├─ FS state....... %s\n", _sbStateDump(sb->state));
	printf("├─ Error policy... %s\n", _sbErrorDump(sb->errors));
	printf("│\n");

	printf("├─ Times mounted........ %" PRIu16 "\n", sb->mountCount);
	_sbMountCheckDump(sb->mountCount, sb->maxMountCount, needsCheck);
	printf("├─ Warrants fs check?... %s\n", needsCheck);
	printf("│\n");

	fmttime_t mountTime;
	utilFmtTime(sb->mountTime, mountTime);
	printf("├─ Last mount time... %s\n", mountTime);

	fmttime_t writeTime;
	utilFmtTime(sb->writeTime, writeTime);
	printf("├─ Last write time... %s\n", writeTime);
	printf("│\n");

	fmttime_t lastCheck;
	utilFmtTime(sb->lastCheck, lastCheck);
	printf("├─ Last fs check........ %s\n", lastCheck);

	_sbTimeCheckDump(sb->lastCheck, sb->checkInterval, needsCheck);
	printf("├─ Warrants fs check?... %s\n", needsCheck);
	printf("│\n");

	printf("├─┬─ Block groups:\n");
	printf("│ ├─── Inodes per group...... %" PRIu32 "\n", sb->inodesPerGroup);
	printf("│ ├─── Blocks per group...... %" PRIu32 "\n", sb->blocksPerGroup);
	printf("│ ├─── Fragments per group... %" PRIu32 "\n", sb->fragsPerGroup);
	printf("│ └─── Reserved GDT blocks... %" PRIu16 "\n", sb->reservedGDT);
	printf("│\n");

	printf("├─┬─ Inodes:\n");
	printf("│ ├─── Count........... %" PRIu32, sb->inodeCount);
	printf(" (%" PRIu32 " free)\n", sb->freeInodesCount);
	printf("│ ├─── Size............ %" PRIu32 " bytes\n", sb->inodeSize);
	printf("│ └─── First usable.... #%" PRIu32 "\n", sb->firstInode);
	printf("│\n");

	printf("├─┬─ Blocks:\n");
	printf("│ ├─── Count........ %" PRIu32, sb->blockCount);
	printf(" (%" PRIu32 " free, ", sb->freeBlocksCount);
	printf("%" PRIu32 " reserved for super user)\n", sb->reservedBlocksCount);

	const uint32_t BLOCK_SIZE = 1024 << sb->logBlockSize;
	printf("│ ├─── Size......... %" PRIu32 "Kib ", BLOCK_SIZE / 1024);
	printf("(%" PRIu32 " bytes)\n", BLOCK_SIZE);

	const uint32_t FRAG_SIZE = (sb->logBlockSize > 0)
		? (1024 << sb->logBlockSize)
		: (1024 >> -sb->logBlockSize);
	printf("│ ├─── Frag size.... %" PRIu32 "Kib ", FRAG_SIZE / 1024);
	printf("(%" PRIu32 " bytes)\n", FRAG_SIZE);
	printf("│ ├─── User ID...... %" PRIu16 "\n", sb->defaultUserID);
	printf("│ └─── Group ID..... %" PRIu16 "\n", sb->defaultGroupID);
	printf("│\n");

	printf("├─┬─ Features:\n");
	printf("│ ├─┬─ Compatible:\n");
	printf("│ │ ├─── Prealloc dirs?... %s\n", COMPAT(SB_FC_DIR_PREALLOC));
	printf("│ │ ├─── Imagic inodes?... %s\n", COMPAT(SB_FC_IMAGIC_INODES));
	printf("│ │ ├─── Has journal?..... %s\n", COMPAT(SB_FC_HAS_JOURNAL));
	printf("│ │ ├─── Extended attr?... %s\n", COMPAT(SB_FC_HAS_JOURNAL));
	printf("│ │ ├─── Resized inode?... %s\n", COMPAT(SB_FC_RESIZED_INODE));
	printf("│ │ └─── Dir indexing?.... %s\n", COMPAT(SB_FC_DIR_INDEXING));
	printf("│ │\n");
	printf("│ ├─┬─ Incompatible:\n");
	printf("│ │ ├─── Compressed?......... %s\n", INCOMPAT(SB_FI_COMPRESSION));
	printf("│ │ ├─── Filetype in dirs?... %s\n", INCOMPAT(SB_FI_FILETYPE));
	printf("│ │ ├─── Needs recovery?..... %s\n", INCOMPAT(SB_FI_RECOVER));
	printf("│ │ ├─── Journal device?..... %s\n", INCOMPAT(SB_FI_JOURNAL_DEV));
	printf("│ │ └─── Meta block group?... %s\n", INCOMPAT(SB_FI_META_BG));
	printf("│ │\n");
	printf("│ └─┬─ Compatible (read-only):\n");
	printf("│   ├─── Sparse Superblock?... %s\n", COMPATRO(SB_FRO_SPARSE_SB));
	printf("│   ├─── Large files?......... %s\n", COMPATRO(SB_FRO_LARGE_FILE));
	printf("│   └─── BTree sorted dirs?... %s\n", COMPATRO(SB_FRO_BTREE_DIR));
	printf("│\n");

	printf("├─┬─ Volume:\n");

	_sbUUIDDump(sb->uuid, uuid);
	printf("│ ├─── UUID...... 0x%s\n", uuid);
	printf("│ ├─── Name...... %s\n", *sb->volumeName ? sb->volumeName : "N/A");
	printf("│ └─── Mounted... %s\n", *sb->mountPath ? sb->mountPath : "N/A");
	printf("│\n");

	printf("├─┬─ Compression:\n");
	printf("│ ├─── LZV1 used?...... %s\n", COMPRESSED(SB_COMP_LZV1));
	printf("│ ├─── LZRW3-A used?... %s\n", COMPRESSED(SB_COMP_LZRW3A));
	printf("│ ├─── gzip used?...... %s\n", COMPRESSED(SB_COMP_GZIP));
	printf("│ ├─── bzip2 used?..... %s\n", COMPRESSED(SB_COMP_BZIP2));
	printf("│ └─── LZO used?....... %s\n", COMPRESSED(SB_COMP_LZO));
	printf("│\n");

	printf("├─┬─ Performance:\n");
	printf("│ ├─── Prealloc (files)... %" PRIu8 "\n", sb->preAllocBlocksF);
	printf("│ └─── Prealloc (dirs).... %" PRIu8 "\n", sb->preAllocBlocksD);
	printf("│\n");

	if( sb->featuresCompat & SB_FC_HAS_JOURNAL ) {
		printf("├─┬─ Journaling:\n");

		_sbUUIDDump(sb->journalUUID, uuid);
		printf("│ ├─── Journal UUID..... 0x%s\n", uuid);
		printf("│ ├─── Journal inode.... %" PRIu32 "\n", sb->journalInode);
		printf("│ ├─── Journal device... %" PRIu32 "\n", sb->journalDevice);
		printf("│ └─── Last orphan...... %" PRIu32 "\n", sb->lastOrphan);
	} else {
		printf("├─ Journaling: not present\n");
	}
	printf("│\n");

	if( sb->featuresCompat & SB_FC_DIR_INDEXING ) {
		printf("├─┬─ Dir indexing:\n");
		printf("│ ├─── Seeds.......... 0x%" PRIx32 ", ", sb->dirHashSeeds[0]);
		printf("0x%" PRIx32 ", ", sb->dirHashSeeds[1]);
		printf("0x%" PRIx32 ", ", sb->dirHashSeeds[2]);
		printf("0x%" PRIx32 "\n", sb->dirHashSeeds[3]);
		printf("│ └─── Hash version... %" PRIu8 "\n", sb->defaultHashVersion);
	} else {
		printf("├─ Dir indexing: not present\n");
	}
	printf("│\n");

	printf("└─┬─ Other options:\n");
	printf("  ├─── Mount options.. 0x%02" PRIx32 "\n", sb->defaultMountOptions);
	printf("  └─── 1st meta BG.... %" PRIu32 "\n", sb->firstMetaBlockGroup);
}

static char *_sbOSDump(uint32_t os) {
	switch( os ) {
	case SB_OS_LINUX:
		return "Linux";
	case SB_OS_HURD:
		return "GNU Hurd";
	case SB_OS_MASIX:
		return "Masix";
	case SB_OS_FREEBSD:
		return "FreeBSD";
	case SB_OS_LITES:
		return "Lites";
	}

	return "unknown";
}

static char *_sbRevDump(uint32_t rev) {
	switch( rev ) {
	case SB_REV_OLD:
		return "0 (old)";
	case SB_REV_DYNAMIC:
		return "1 (dynamic)";
	}

	return "unknown";
}

static char *_sbStateDump(uint32_t state) {
	switch( state ) {
	case SB_ST_VALID:
		return "OK (unmounted cleanly)";
	case SB_ST_ERROR:
		return "error(s) occurred";
	}

	return "unknown";
}

static char *_sbErrorDump(uint32_t errors) {
	switch( errors ) {
	case SB_ERR_CONTINUE:
		return "ignore errors";
	case SB_ERR_READONLY:
		return "remount read-only";
	case SB_ERR_PANIC:
		return "cause kernel panic";
	}

	return "unknown";
}

static void _sbMountCheckDump(uint16_t count, uint16_t max, char out[256]) {
	if( count == max ) {
		out = "yes (max mount count reached)";
	} else if( count > max ) {
		snprintf(out, 256, "yes (%" PRIu16 " more than max)", count - max);
	} else {
		snprintf(out, 256, "no (%" PRIu16 " less than max)", max - count);
	}
}

static void _sbTimeCheckDump(int32_t check, int32_t interval, char out[256]) {
	if( interval == 0 ) {
		snprintf(out, 256, "yes (set to check at boot)");
		return;
	}

	const int32_t DIFF = (int32_t)difftime(check, time(NULL));
	const char *CHECK = YESNO(DIFF >= interval);

	snprintf(out, 256, "%s (check every %" PRIi32 " seconds)", CHECK, DIFF);
}

static void _sbUUIDDump(uint8_t uuid[16], char out[33]) {
	for( int i = 0; i < 32; i += 2 ) {
		sprintf(out + i, "%02x", uuid[i >> 1]);
	}

	out[32] = '\0';
}

static void _bgDump(BlockGroupDescriptor *bgdesc, int i) {
	printf("* Block Group Descriptor (#%d)\n", i);
	printf("│\n");

	printf("├─ Block bitmap... %" PRIu32 "\n", bgdesc->blockBitmap);
	printf("├─ Inode bitmap... %" PRIu32 "\n", bgdesc->inodeBitmap);
	printf("│\n");
	printf("├─ Inode table.... %" PRIu32 "\n", bgdesc->inodeTable);
	printf("│\n");
	printf("├─ Free blocks.... %" PRIu16 "\n", bgdesc->freeBlocks);
	printf("├─ Free inodes.... %" PRIu16 "\n", bgdesc->freeInodes);
	printf("│\n");
	printf("└─ Dir inodes..... %" PRIu16 "\n", bgdesc->dirInodes);
}

static void _inoDump(Inode *ino, uint32_t revision, int i) {
	char perms[4];
	int64_t size;
	fmttime_t date;

	printf("* Inode (#%d)\n", i);
	printf("│\n");

	printf("├─┬─ Mode:\n");

	_inoUserDump(ino->mode, perms);
	printf("│ ├─── User......... %s\n", perms);

	_inoGroupDump(ino->mode, perms);
	printf("│ ├─── Group........ %s\n", perms);

	_inoOtherDump(ino->mode, perms);
	printf("│ ├─── Others....... %s\n", perms);
	printf("│ │\n");

	printf("│ ├─── Set UID...... %s\n", YESNO(ino->mode & INODE_FM_SET_UID));
	printf("│ ├─── Set GID...... %s\n", YESNO(ino->mode & INODE_FM_SET_GID));
	printf("│ ├─── Sticky bit... %s\n", YESNO(ino->mode & INODE_FM_STICKY));
	printf("│ │\n");

	printf("│ └─┬─ Format:\n");
	printf("│   ├─── Socket?......... %s\n", YESNO(ino->mode & INODE_FM_SOCK));
	printf("│   ├─── Symlink?........ %s\n", YESNO(ino->mode & INODE_FM_SYMB));
	printf("│   ├─── Regular file?... %s\n", YESNO(ino->mode & INODE_FM_FILE));
	printf("│   ├─── Block device?... %s\n", YESNO(ino->mode & INODE_FM_BLOCK));
	printf("│   ├─── Directory?...... %s\n", YESNO(ino->mode & INODE_FM_DIR));
	printf("│   ├─── Char. device?... %s\n", YESNO(ino->mode & INODE_FM_CHAR));
	printf("│   └─── FIFO?........... %s\n", YESNO(ino->mode & INODE_FM_FIFO));
	printf("│\n");

	if( revision == SB_REV_DYNAMIC ) {
		size = (((int64_t)ino->size_hi) << 32) | ino->size_lo;
	} else {
		size = ino->size_lo;
	}

	printf("├─ UID.... %" PRIu16 "\n", ino->uid);
	printf("├─ Size... %" PRIi64 " bytes\n", size);
	printf("│\n");

	utilFmtTime(ino->accessTime, date);
	printf("├─ Access time......... %s\n", date);

	utilFmtTime(ino->createTime, date);
	printf("├─ Creation time....... %s\n", date);

	utilFmtTime(ino->modifyTime, date);
	printf("├─ Modification time... %s\n", date);

	utilFmtTime(ino->deleteTime, date);
	printf("├─ Deletion time....... %s\n", date);
	printf("│\n");

	printf("├─ Group with access... %" PRIu16 "\n", ino->gid);
	printf("├─ Linked to........... %" PRIu16 " times\n", ino->linkCount);
	printf("│\n");

	printf("├─┬─ Flags:\n");
	printf("│ ├─── Secure rm?..... %s\n", INOFL(INODE_FL_SECURE_RM));
	printf("│ ├─── Record unrm?... %s\n", INOFL(INODE_FL_RECORD_UNRM));
	printf("│ ├─── Compress?...... %s\n", INOFL(INODE_FL_COMPRESS));
	printf("│ ├─── Sync?.......... %s\n", INOFL(INODE_FL_SYNC));
	printf("│ ├─── Immutable?..... %s\n", INOFL(INODE_FL_IMMUTABLE));
	printf("│ ├─── Append?........ %s\n", INOFL(INODE_FL_APPEND));
	printf("│ ├─── No dump?....... %s\n", INOFL(INODE_FL_NO_DUMP));
	printf("│ ├─── No atime?...... %s\n", INOFL(INODE_FL_NO_ATIME));
	printf("│ │\n");

	if( ino->flags & INODE_FL_COMPRESS ) {
		printf("│ ├─┬─ Compression:\n");
		printf("│ │ ├─── Dirty?............. %s\n", INOFL(INODE_FL_DIRTY));
		printf("│ │ ├─── Compress blocks?... %s\n", INOFL(INODE_FL_COMPBLKS));
		printf("│ │ ├─── Access raw data?... %s\n", INOFL(INODE_FL_RAWDATA));
		printf("│ │ └─── Compress error?.... %s\n", INOFL(INODE_FL_COMPERR));
	} else {
		printf("│ ├─── Compression: not used\n");
	}
	printf("│ │\n");

	printf("│ ├─── BTree/hash dir?... %s\n", INOFL(INODE_FL_BTREE_DIR));
	printf("│ ├─── Imagic?........... %s\n", INOFL(INODE_FL_IMAGIC_DIR));
	printf("│ ├─── Journal data?..... %s\n", INOFL(INODE_FL_JOURNAL_DATA));
	printf("│ └─── Reserved?......... %s\n", INOFL(INODE_FL_RESERVED));
	printf("│\n");

	printf("├─┬─ Blocks:\n");
	printf("│ ├─── 512-byte blocks count... %" PRIu32 "\n", ino->blocks);

	printf("│ ├─┬─ Direct blocks:\n");
	for( int i = 0; i < 11; ++i ) {
		printf("│ │ ├─── Block #%" PRIu32 "\n", ino->block[i]);
	}
	printf("│ │ └─── Block #%" PRIu32 "\n", ino->block[11]);
	printf("│ │\n");

	printf("│ ├─── Indirect block.......... #%" PRIu32 "\n", ino->block[12]);
	printf("│ ├─── Doubly-indirect block... #%" PRIu32 "\n", ino->block[13]);
	printf("│ └─── Trebly-indirect block... #%" PRIu32 "\n", ino->block[14]);
}

static void _inoUserDump(uint16_t mode, char *perms) {
	perms[0] = (mode & INODE_FM_USER_R) ? 'R' : '-';
	perms[1] = (mode & INODE_FM_USER_W) ? 'W' : '-';
	perms[2] = (mode & INODE_FM_USER_X) ? 'X' : '-';
	perms[3] = '\0';
}

static void _inoGroupDump(uint16_t mode, char *perms) {
	perms[0] = (mode & INODE_FM_GROUP_R) ? 'R' : '-';
	perms[1] = (mode & INODE_FM_GROUP_W) ? 'W' : '-';
	perms[2] = (mode & INODE_FM_GROUP_X) ? 'X' : '-';
	perms[3] = '\0';
}

static void _inoOtherDump(uint16_t mode, char *perms) {
	perms[0] = (mode & INODE_FM_OTHER_R) ? 'R' : '-';
	perms[1] = (mode & INODE_FM_OTHER_W) ? 'W' : '-';
	perms[2] = (mode & INODE_FM_OTHER_X) ? 'X' : '-';
	perms[3] = '\0';
}
