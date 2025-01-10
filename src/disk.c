/* ext2p
 * Disk emulator
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fault.h"
#include "util.h"

#include "disk.h"

Disk *diskOpen(const char *FILEPATH) {
	Disk *disk = malloc(sizeof(*disk));
	if( disk == NULL ) {
		FATAL("couldn't allocate memory for disk");
	}

	utilReadFile(FILEPATH, &disk->fp);
	return disk;
}

Disk *diskClone(Disk *disk) {
	Disk *clone = malloc(sizeof(*clone));
	if( clone == NULL ) {
		FATAL("couldn't allocate memory for disk");
	}

	utilCloneFile(&disk->fp, &clone->fp);
	return clone;
}

void diskClose(Disk *disk) {
	utilFreeFile(&disk->fp);
	free(disk);
}

bool diskCheckBounds(Disk *disk, size_t size) {
	const size_t END_POS = disk->fp.data - disk->fp._start + size;
	return END_POS < disk->fp.size;
}

uint8_t diskRead8(Disk *disk) {
	if( !diskCheckBounds(disk, 8) ) {
		FATAL("tried to read past readable area\n");
	}

	return utilRead8(&disk->fp);
}

uint16_t diskRead16(Disk *disk) {
	if( !diskCheckBounds(disk, 16) ) {
		FATAL("tried to read past readable area\n");
	}

	return utilRead16(true, &disk->fp);
}

uint32_t diskRead32(Disk *disk) {
	if( !diskCheckBounds(disk, 32) ) {
		FATAL("tried to read past readable area\n");
	}

	return utilRead32(true, &disk->fp);
}

uint64_t diskRead64(Disk *disk) {
	if( !diskCheckBounds(disk, 64) ) {
		FATAL("tried to read past readable area\n");
	}

	return utilRead64(true, &disk->fp);
}

void diskSkip(Disk *disk, size_t skip) {
	if( !diskCheckBounds(disk, skip) ) {
		FATAL("tried to read past readable area\n");
	}

	disk->fp.data += skip;
}

void diskCopy(Disk *disk, void *dest, size_t size) {
	if( !diskCheckBounds(disk, size) ) {
		FATAL("tried to read past readable area\n");
	}

	memcpy(dest, disk->fp.data, size);
	diskSkip(disk, size);
}

void diskRewind(Disk *disk, size_t pos) {
	if( (disk->fp.data - pos) < disk->fp._start ) {
		FATAL("tried to rewind to before start of file\n");
	}

	disk->fp.data -= pos;
}

void diskSeek(Disk *disk, size_t pos) {
	disk->fp.data = disk->fp._start;
	diskSkip(disk, pos);
}

void diskSeekStart(Disk *disk) {
	disk->fp.data = disk->fp._start;
}

size_t diskGetPos(Disk *disk) {
	return disk->fp.data - disk->fp._start;
}
