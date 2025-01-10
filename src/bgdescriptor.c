/* ext2p
 * Block group descriptor table
 */

#include <stdint.h>

#include "disk.h"

#include "bgdescriptor.h"

bool bgtableRead(uint32_t count, BlockGroupDescriptor *bgtable, Disk *disk) {
	for( uint32_t i = 0; i < count; ++i ) {
		diskCopy(disk, &bgtable[i], 32);
	}

	return true;
}
