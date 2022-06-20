#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "profiler.h"

namespace sk {

///////////////////////////////////////////

void *sk_malloc(size_t bytes) {
	void *result = malloc(bytes);
	if (result == nullptr && bytes > 0) {
		fprintf(stderr, "Memory alloc failed!");
		abort();
	}
	PROFILE_ALLOC(result, bytes);
	return result;
}

///////////////////////////////////////////

void *sk_calloc(size_t bytes) {
	void *result = calloc(bytes, 1);
	if (result == nullptr && bytes > 0) {
		fprintf(stderr, "Memory alloc failed!");
		abort();
	}
	PROFILE_ALLOC(result, bytes);
	return result;
}

///////////////////////////////////////////

void *sk_realloc(void *memory, size_t bytes) {
	PROFILE_FREE(memory);
	void *result = realloc(memory, bytes);
	if (result == nullptr && bytes > 0) {
		fprintf(stderr, "Memory alloc failed!");
		abort();
	}
	PROFILE_ALLOC(result, bytes);
	return result;
}

///////////////////////////////////////////

void _sk_free(void* memory) {
	PROFILE_FREE(memory);
	free(memory);
}

} // namespace sk