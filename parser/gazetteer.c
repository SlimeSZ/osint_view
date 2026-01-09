#include "gazetteer.h" 
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 48000

static Gazetteer *gazetteer_init(void) {
	Gazetteer *gaz = malloc(sizeof(Gazetteer));
	if (__builtin_expect(!gaz, 0)) {
		fprintf(stderr, "Err: malloc");
		return NULL;
	}
	gaz->entries = malloc(INITIAL_CAPACITY * sizeof(GeoEntry));
	if (!gaz->entries) {
		fprintf(stderr, "Err: malloc");
		free(gaz);
		return NULL;
	}
	gaz->count = 0;
	gaz->capacity = INITIAL_CAPACITY; 
	return gaz;
}

void free_gazetteer(Gazetteer *gaz) {
	if (__builtin_expect(!gaz, 0))
		return;
	for (size_t i = 0; i < gaz->count; i++) {
		GeoEntry *e = &gaz->entries[i];
		free(e->name);
		free(e->iso2);
		free(e->iso3);
		free(e->country);
	}
	free(gaz->entries);
	free(gaz);
}
