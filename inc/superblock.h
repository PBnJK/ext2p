#ifndef GUARD_EXT2P_SUPERBLOCK_H_
#define GUARD_EXT2P_SUPERBLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#include "disk.h"

/* Magic found at offset 56 of the superblock */
#define EXT2_SUPER_MAGIC 0xEF53

/* Revision 0 fixed values */
#define EXT2_REV0_FIRST_INODE 11
#define EXT2_REV0_INODE_SIZE 128

/* Compatible features bitmasks
 *
 * A filesystem implementation that does not handle these features is still able
 * to use this filesystem without risk of damaging metadata
 */
#define SB_FC_DIR_PREALLOC 0x1
#define SB_FC_IMAGIC_INODES 0x2
#define SB_FC_HAS_JOURNAL 0x4
#define SB_FC_EXT_ATTR 0x8
#define SB_FC_RESIZED_INODE 0x10
#define SB_FC_DIR_INDEXING 0x20

/* Incompatible features bitmasks
 *
 * A filesystem implementation that does not handle these features is not able
 * to use this filesystem without possible damage to its data, and should refuse
 * to mount it
 */
#define SB_FI_COMPRESSION 0x1
#define SB_FI_FILETYPE 0x2
#define SB_FI_RECOVER 0x4
#define SB_FI_JOURNAL_DEV 0x8
#define SB_FI_META_BG 0x10

/* "Read-only" features bitmasks
 *
 * A filesystem implementation that does not handle these features is still able
 * to use this filesystem, but should mount it in read-only mode if that is the
 * case
 */
#define SB_FRO_SPARSE_SB 0x1
#define SB_FRO_LARGE_FILE 0x2
#define SB_FRO_BTREE_DIR 0x4

/* Compression algorithms bitmasks
 *
 * Used to determine which compression algorithm(s) were used, if any, to
 * compress this filesystem's data */
#define SB_COMP_LZV1 0x1
#define SB_COMP_LZRW3A 0x2
#define SB_COMP_GZIP 0x4
#define SB_COMP_BZIP2 0x8
#define SB_COMP_LZO 0x10

/* Enum containing all valid values of s_state
 * Represents the current state of the filesystem
 */
typedef enum _SB_State {
	SB_ST_VALID = 1,
	SB_ST_ERROR = 2,
} SB_State;

/* Enum containing all valid values of s_errors
 * Represents the action to be taken when an error is detected
 */
typedef enum _SB_Errors {
	SB_ERR_CONTINUE = 1, /* Ignore error */
	SB_ERR_READONLY = 2, /* Remount in read-only mode */
	SB_ERR_PANIC = 3, /* Cause a kernel panic (!) */
} SB_Errors;

/* Enum containing all valid values of s_creator_os
 * Represents the OS that created the filesystem
 */
typedef enum _SB_OS {
	SB_OS_LINUX = 0, /* Linux */
	SB_OS_HURD = 1, /* GNU HURD */
	SB_OS_MASIX = 2, /* Masix */
	SB_OS_FREEBSD = 3, /* FreeBSD */
	SB_OS_LITES = 4, /* Lites */
} SB_OS;

/* Enum containing all valid values of s_rev_level
 * Represents the revision level
 */
typedef enum _SB_RevLevel {
	SB_REV_OLD = 0, /* Revision 0 */
	SB_REV_DYNAMIC = 1, /* Revision 1 (variable inode sizes, etc.) */
} SB_RevLevel;

/* Structure representing an ext2 Superblock */
typedef struct _Superblock {
	uint32_t inodeCount; /* Inodes in the filesystem */
	uint32_t blockCount; /* Blocks in the filesystem */

	uint32_t reservedBlocksCount; /* Blocks reserved for the super user */

	uint32_t freeBlocksCount; /* Free blocks in the filesystem */
	uint32_t freeInodesCount; /* Free inodes in the filesystem */

	uint32_t firstDataBlock; /* ID of the Superblock */

	uint32_t logBlockSize; /* Block size */
	int32_t logFragSize; /* Fragment size */

	uint32_t blocksPerGroup; /* Blocks per block group */
	uint32_t fragsPerGroup; /* Fragments per block group */
	uint32_t inodesPerGroup; /* Inodes per block group */

	int32_t mountTime; /* UNIX timestamp of last filesystem mount */
	int32_t writeTime; /* UNIX timestamp of last filesystem write */

	int16_t mountCount; /* Times the filesystem was mounted since last check */
	int16_t maxMountCount; /* Max times the filesystem can be mounted before a
							   full check is performed */

	uint16_t magic; /* *.~ Magic ~.* */

	uint16_t state; /* Current state of the filesystem */
	uint16_t errors; /* Action to be taken when an error is detected */

	uint16_t minorRevLevel; /* Minor revision level */

	int32_t lastCheck; /* UNIX timestamp of last filesystem check */
	int32_t checkInterval; /* Max UNIX time between filesystem checks */

	uint32_t creatorOS; /* OS that created the filesystem */

	uint32_t revLevel; /* Major revision level */

	uint16_t defaultUserID; /* Default user ID for reserved blocks */
	uint16_t defaultGroupID; /* Default group ID for reserved blocks */

	uint32_t firstInode; /* Index of the first usable inode */
	uint16_t inodeSize; /* Size of an inode */

	uint16_t sbBlockGroup; /* Block group containing this Superblock */

	uint32_t featuresCompat; /* Compatible features (see SB_FC_*) */
	uint32_t featuresIncompat; /* Incompatible features (see SB_FI_*) */
	uint32_t featuresReadOnly; /* "Read-only" features (see SB_FRO_*) */

	uint8_t uuid[16]; /* Volume ID */
	char volumeName[16]; /* Volume name (ISO-Latin-1 only, null-terminated) */
	char mountPath[64]; /* Path to where the filesystem was last mounted */

	uint32_t compressionBitmap; /* Compression algo(s) used (See SB_COMP_*) */

	/* Performance related */
	uint8_t preAllocBlocksF; /* Blocks to prealloc when creating a new file */
	uint8_t preAllocBlocksD; /* Blocks to prealloc when creating a new dir */
	uint16_t reservedGDT; /* Reserved GDT entries for future expansion */

	/* Journaling (ext3) */
	uint8_t journalUUID[16]; /* Journal Superblock UUID */
	uint32_t journalInode; /* Inode number of the journal file */
	uint32_t journalDevice; /* Device number of the journal file */
	uint32_t lastOrphan; /* First inode in the list of inodes to delete */

	/* SB_FC_DIR_INDEXING specific */
	uint32_t dirHashSeeds[4]; /* Seeds used for dir indexing */
	uint8_t defaultHashVersion; /* Default hash version used for dir indexing */
	uint8_t padding[3]; /* Padding (reserved for future versions) */

	/* Other options */
	uint32_t defaultMountOptions; /* TODO */
	uint32_t firstMetaBlockGroup; /* TODO */
} Superblock;

bool sbRead(Superblock *sb, Disk *disk);

#endif // !GUARD_EXT2P_SUPERBLOCK_H_
