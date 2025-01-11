#ifndef GUARD_EXT2P_DISK_H_
#define GUARD_EXT2P_DISK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "util.h"

typedef struct _Disk {
	char *filepath;
	FP fp;
} Disk;

#define READ8() diskRead8(disk);
#define READ16() diskRead16(disk);
#define READ32() diskRead32(disk);
#define READ64() diskRead64(disk);

Disk *diskOpen(const char *FILEPATH);
Disk *diskClone(Disk *disk);

void diskClose(Disk *disk);

bool diskCheckBounds(Disk *disk, size_t size);

uint8_t diskRead8(Disk *disk);
uint16_t diskRead16(Disk *disk);
uint32_t diskRead32(Disk *disk);
uint64_t diskRead64(Disk *disk);

void diskWrite8(Disk *disk, uint8_t data);
void diskWrite16(Disk *disk, uint16_t data);
void diskWrite32(Disk *disk, uint32_t data);
void diskWrite64(Disk *disk, uint64_t data);

void diskSkip(Disk *disk, size_t skip);
void diskCopy(Disk *disk, void *dest, size_t size);

void diskRewind(Disk *disk, size_t pos);

void diskSeek(Disk *disk, size_t pos);
void diskSeekStart(Disk *disk);

size_t diskGetPos(Disk *disk);

void diskSave(Disk *disk, const char *FILEPATH);

#endif // !GUARD_EXT2P_DISK_H_
