/* ext2p
 * Entry point
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

static void _usage(void);

int main(int argc, char *argv[]) {
	char *img = NULL;
	if( argc > 2 ) {
		_usage();
		exit(EXIT_FAILURE);
	}

	if( argc == 2 ) {
		img = argv[1];
	}

	Shell *shell = shellOpen(img);
	if( shell == NULL ) {
		return EXIT_FAILURE;
	}

	return shellRun(shell) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void _usage(void) {
	printf("usage: ext2p [IMAGE]\n");
}
