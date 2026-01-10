#ifndef GAZETTEER_H
#define GAZETTEER_H

#include <stddef.h>
#define MAX_ACTORS 4
#define MAX_LOCATIONS 8

typedef struct {
	char *name;
	char *iso2;
	size_t start_idx; // first city in Gazetteer.entries array 
	size_t city_count;
} CountryIndex;

typedef struct {
	char *city_name; // maps via hashmap 
	float lat;
	float lon;
	char *iso2;
	char *iso3;
	CountryIndex *country;
} GeoEntry;

typedef struct {
	GeoEntry *entries; // independent, not indexed by country 
	size_t count;
	size_t capacity;

	CountryIndex *countries; // maps via hashmap
	size_t country_count;
} Gazetteer;

Gazetteer *load_gazetteer(const char *filepath);
void free_gazetteer(Gazetteer *gaz);










#endif
