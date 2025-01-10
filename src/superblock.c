/* ext2p
 * Superblock parse
 */

#include <inttypes.h>
#include <stdint.h>

#include "disk.h"
#include "fault.h"

#include "superblock.h"

bool sbRead(Superblock *sb, Disk *disk) {
	diskCopy(disk, sb, 264);

	if( sb->magic != EXT2_SUPER_MAGIC ) {
		ERR("Superblock has bad magic %04X (should be EF53)", sb->magic);
		return false;
	}

	if( sb->state == 0 || sb->state > SB_ST_ERROR ) {
		WARN("Superblock has bad state '%" PRIu16 "', ignoring", sb->state);
	}

	if( sb->errors == 0 || sb->errors > SB_ERR_PANIC ) {
		WARN("Superblock has bad errors '%" PRIu16 "', ignoring", sb->errors);
	}

	if( sb->creatorOS > SB_OS_LITES ) {
		WARN("Superblock has bad OS '%" PRIu32 "', ignoring", sb->creatorOS);
	}

	if( sb->revLevel > SB_REV_DYNAMIC ) {
		WARN("Superblock has bad rev. '%" PRIu32 "', ignoring", sb->revLevel);
	}

	diskSkip(disk, 760);
	return true;
}
