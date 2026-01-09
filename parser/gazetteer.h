#ifndef GAZETTEER_H
#define GAZETTEER_H

#include <stddef.h>
#define MAX_ACTORS 4
#define MAX_LOCATIONS 8

typedef struct {
	char *name;
	float lat;
	float lon;
	char *iso2;
	char *iso3;
	char *country;
} GeoEntry;

typedef struct {
	GeoEntry *entries;
	size_t count;
	size_t capacity;
} Gazetteer;

Gazetteer *load_gazetteer(const char *filepath);
void free_gazetteer(Gazetteer *gaz);










#endif
