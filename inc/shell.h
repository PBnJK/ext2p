#ifndef GUARD_EXT2P_SHELL_H_
#define GUARD_EXT2P_SHELL_H_

#include "ext2.h"
#include <stdint.h>

typedef struct _Shell {
	uint32_t cd; /* Current directory */

	int pathLevel;
	char path[255][128];

	Ext2 *fs;
} Shell;

Shell *shellOpen(const char *IMGPATH);
void shellFree(Shell *shell);

bool shellRun(Shell *shell);

#endif // GUARD_EXT2P_SHELL_H_
