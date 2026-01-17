#ifndef GAZETTEER_H
#define GAZETTEER_H

#include <stdint.h>
#include <stddef.h>
#define MAX_ACTORS 4
#define MAX_LOCATIONS 8
#include <uthash.h>

typedef struct {
	char *name;
	char *iso2;
	size_t start_idx; // first city in Gazetteer.entries array 
	size_t city_count;
} CountryIndex;

typedef struct {
	char *city_name; 
	float lat;
	float lon;
	char *iso2; // hash key field
	char *iso3;
} GeoEntry;

typedef struct {
	char iso2[3];
	uint16_t country_idx;
	UT_hash_handle hh;
} IsoToCountryMap;

typedef struct {
	GeoEntry *entries;  
	size_t geo_count;
	size_t geo_capacity;

	CountryIndex *countries; 
	size_t country_count;
	size_t country_capacity;
	/* hashmaps: entries[i].iso2 -> uint16_t country_idx */	
	IsoToCountryMap *country_hashmap;
} Gazetteer;

Gazetteer *load_gaz(const char *filepath);
void gaz_free(Gazetteer *gaz);










#endif
