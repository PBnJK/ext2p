#ifndef GUARD_EXT2P_SHELL_H_
#define GUARD_EXT2P_SHELL_H_

#include <stdbool.h>
#include <stdint.h>

#include "ext2.h"

typedef struct _Shell {
	uint32_t cd; /* Current directory */

	int pathLevel;
	struct {
		char name[255];
		uint32_t inode;
	} path[128];

	bool run;
	int err;

	Ext2 *fs;
} Shell;

Shell *shellOpen(const char *IMGPATH);
void shellFree(Shell *shell);

bool shellRun(Shell *shell);

#endif // GUARD_EXT2P_SHELL_H_
