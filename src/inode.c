/* ext2p
 * Inode
 */

#include <stdbool.h>
#include <stdint.h>

#include "disk.h"

#include "inode.h"

bool inodeRead(Inode *inode, Disk *disk) {
	diskCopy(disk, inode, 128);

	return true;
}
