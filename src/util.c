/* elfp
 * Utilities
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fault.h"

#include "util.h"

static int _levenshtein(const char *A, size_t asz, const char *B, size_t bsz);

bool utilReadFile(const char *FILEPATH, FP *fp) {
	FILE *file = fopen(FILEPATH, "rb");
	if( file == NULL ) {
		ERR("couldn't open the file at '%s'\n", FILEPATH);
		return false;
	}

	fseek(file, 0L, SEEK_END);
	fp->size = ftell(file);
	rewind(file);

	fp->_start = malloc(fp->size + 1);
	if( fp->_start == NULL ) {
		ERR("failed to allocate buffer for file at '%s'\n", FILEPATH);
		return false;
	}

	const size_t BYTES_READ = fread(fp->_start, sizeof(char), fp->size, file);
	if( BYTES_READ < fp->size ) {
		ERR("couldn't read the file at '%s'\n", FILEPATH);
		return false;
	}

	fp->_start[BYTES_READ] = '\0';
	fp->data = fp->_start;

	fclose(file);

	return true;
}

void utilCloneFile(FP *fp, FP *dest) {
	dest->_start = fp->data;
	dest->data = fp->data;
	dest->size = fp->size - (fp->data - fp->_start);
}

void utilFreeFile(FP *fp) {
	free(fp->_start);
	fp->data = NULL;
}

uint8_t utilRead8(FP *fp) {
	return *fp->data++;
}

uint16_t utilRead16(bool le, FP *fp) {
	uint16_t lo, hi;

	if( le ) {
		hi = utilRead8(fp);
		lo = utilRead8(fp);
	} else {
		lo = utilRead8(fp);
		hi = utilRead8(fp);
	}

	return (lo << 8) | hi;
}

uint32_t utilRead32(bool le, FP *fp) {
	uint32_t a, b, c, d;

	if( le ) {
		d = utilRead8(fp);
		c = utilRead8(fp);
		b = utilRead8(fp);
		a = utilRead8(fp);
	} else {
		a = utilRead8(fp);
		b = utilRead8(fp);
		c = utilRead8(fp);
		d = utilRead8(fp);
	}

	return UTIL_SHF(a, 24) | UTIL_SHF(b, 16) | UTIL_SHF(c, 8) | (d & 0xFF);
}

uint64_t utilRead64(bool le, FP *fp) {
	uint64_t a, b, c, d, e, f, g, h;

	if( le ) {
		h = utilRead8(fp);
		g = utilRead8(fp);
		f = utilRead8(fp);
		e = utilRead8(fp);
		d = utilRead8(fp);
		c = utilRead8(fp);
		b = utilRead8(fp);
		a = utilRead8(fp);
	} else {
		a = utilRead8(fp);
		b = utilRead8(fp);
		c = utilRead8(fp);
		d = utilRead8(fp);
		e = utilRead8(fp);
		f = utilRead8(fp);
		g = utilRead8(fp);
		h = utilRead8(fp);
	}

	return UTIL_SHF(a, 56) | UTIL_SHF(b, 48) | UTIL_SHF(c, 40) | UTIL_SHF(d, 32)
		| UTIL_SHF(e, 24) | UTIL_SHF(f, 16) | UTIL_SHF(g, 8) | (h & 0xFF);
}

void utilWrite8(FILE *file, uint8_t data) {
	fputc(data, file);
}

void utilWrite16(FILE *file, uint16_t data) {
	fputc(UTIL_SHFR(data, 0), file);
	fputc(UTIL_SHFR(data, 8), file);
}

void utilWrite32(FILE *file, uint32_t data) {
	fputc(UTIL_SHFR(data, 0), file);
	fputc(UTIL_SHFR(data, 8), file);
	fputc(UTIL_SHFR(data, 16), file);
}

void utilWrite64(FILE *file, uint64_t data) {
	fputc(UTIL_SHFR(data, 0), file);
	fputc(UTIL_SHFR(data, 8), file);
	fputc(UTIL_SHFR(data, 16), file);
	fputc(UTIL_SHFR(data, 24), file);
}

size_t utilFmtTime(time_t time, fmttime_t ftime) {
	const struct tm *TIME_TM = localtime(&time);
	return strftime(ftime, BUFSIZ, "%a, %d %b %Y %T %z", TIME_TM);
}

int utilLevenshtein(const char *A, const char *B) {
	size_t asz = strlen(A);
	size_t bsz = strlen(B);

	return _levenshtein(A, asz, B, bsz);
}

static int _levenshtein(const char *A, size_t asz, const char *B, size_t bsz) {
	if( strcmp(A, B) == 0 ) {
		return 0;
	}

	if( asz == 0 ) {
		return bsz;
	}

	if( bsz == 0 ) {
		return asz;
	}

	if( A[asz - 1] == B[bsz - 1] ) {
		return _levenshtein(A, asz - 1, B, bsz - 1);
	}

	int a = _levenshtein(A, asz - 1, B, bsz);
	int b = _levenshtein(A, asz, B, bsz - 1);
	int c = _levenshtein(A, asz - 1, B, bsz - 1);

	if( a > b ) {
		a = b;
	}

	if( a > c ) {
		a = c;
	}

	return a + 1;
}
