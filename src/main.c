/* ext2p
 * Entry point
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fault.h"
#include "shell.h"

static void _usage(void);

int main(int argc, char *argv[]) {
	if( argc < 2 ) {
		_usage();
		exit(EXIT_FAILURE);
	}

	char *img = argv[1];
	if( img == NULL ) {
		ERR("must provide an input image\n\n");
		_usage();
		return EXIT_FAILURE;
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
