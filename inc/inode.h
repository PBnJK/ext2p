#ifndef GUARD_EXT2_INODE_H_
#define GUARD_EXT2_INODE_H_

#include <stdbool.h>
#include <stdint.h>

#include "disk.h"

/* Reserved inodes */
#define INODE_RES_BAD_BLOCKS 1
#define INODE_RES_ROOT_DIR 2
#define INODE_RES_ACL_IDX 3
#define INODE_RES_ACL_DATA 4
#define INODE_RES_BOOT_LOADER 5
#define INODE_RES_UNRM_DIR 6

/* Inode mode */
#define INODE_FM_SOCK 0xC000
#define INODE_FM_SYMB 0xA000
#define INODE_FM_FILE 0x8000
#define INODE_FM_BLOCK 0x6000
#define INODE_FM_DIR 0x4000
#define INODE_FM_CHAR 0x2000
#define INODE_FM_FIFO 0x1000

#define INODE_FM_SET_UID 0x0800
#define INODE_FM_SET_GID 0x0400
#define INODE_FM_STICKY 0x0200

#define INODE_FM_USER_R 0x0100
#define INODE_FM_USER_W 0x0080
#define INODE_FM_USER_X 0x0040

#define INODE_FM_GROUP_R 0x0020
#define INODE_FM_GROUP_W 0x0010
#define INODE_FM_GROUP_X 0x0008

#define INODE_FM_OTHER_R 0x0004
#define INODE_FM_OTHER_W 0x0002
#define INODE_FM_OTHER_X 0x0001

/* Inode flags */
#define INODE_FL_SECURE_RM 0x00000001
#define INODE_FL_RECORD_UNRM 0x00000002
#define INODE_FL_COMPRESS 0x00000004
#define INODE_FL_SYNC 0x00000008
#define INODE_FL_IMMUTABLE 0x00000010
#define INODE_FL_APPEND 0x00000020
#define INODE_FL_NO_DUMP 0x00000040
#define INODE_FL_NO_ATIME 0x00000080

#define INODE_FL_DIRTY 0x00000100
#define INODE_FL_COMPBLKS 0x00000200
#define INODE_FL_RAWDATA 0x00000400
#define INODE_FL_COMPERR 0x00000800

#define INODE_FL_BTREE_DIR 0x00001000
#define INODE_FL_INDEX_DIR (INODE_FL_BTREE_DIR)
#define INODE_FL_IMAGIC_DIR 0x00002000
#define INODE_FL_JOURNAL_DATA 0x00004000
#define INODE_FL_RESERVED 0x80000000

typedef struct _Inode {
	uint16_t mode; /* Format and access rights of the described file */
	uint16_t uid; /* Unique ID of the file */

	int32_t size_lo; /* Size of the file (low-bytes) */

	int32_t accessTime; /* UNIX time of when the file was last accessed */
	int32_t createTime; /* UNIX time of when the file was created */
	int32_t modifyTime; /* UNIX time of when the file was last modified */
	int32_t deleteTime; /* UNIX time of when the file was deleted */

	uint16_t gid; /* ID of the group with access to the file */
	uint16_t linkCount; /* How many times this inode is linked to */

	uint32_t blocks; /* 512-byte blocks reserved to contain the inode's data */

	uint32_t flags; /* File flags */
	uint32_t osd1; /* OS-dependent value 1 */

	uint32_t block[15]; /* Data blocks */
	uint32_t generation; /* File version (NTFS) */

	uint32_t fileACL; /* ACL stuff */
	uint32_t size_hi; /* Size of the file (high-bytes) */

	uint32_t faddr; /* File fragment location */
	uint8_t osd2[12]; /* OS-dependent value 2 */
} Inode;

bool inodeRead(Inode *inode, Disk *disk);

#endif // !GUARD_EXT2_INODE_H_
