#ifndef GAZETTEER_H
#define GAZETTEER_H

#include <stdint.h>
#include <stddef.h>
#include <uthash.h>

typedef struct GeoEntry GeoEntry;
typedef struct CountryIndex CountryIndex;
typedef struct IsoToCountryMap IsoToCountryMap; /* char iso2 -> CountryIndex */
typedef struct CountryToCountryMap CountryToCountryMap; /* char name -> CountryIndex */
typedef struct CityToGeoMap CityToGeoMap; /* char name -> GeoEntry */

typedef struct {
	GeoEntry *entries;
	size_t geo_count;
	size_t geo_capacity;

	CountryIndex *countries;
	size_t country_count;
	size_t country_capacity;
	
	CountryToCountryMap *country_map;
	CityToGeoMap *geo_map;
	IsoToCountryMap *iso_map;
} Gazetteer;

struct CountryIndex{
	char *name;
	size_t start_idx;
	size_t geo_count;
};

struct GeoEntry {
	char *name;
	float lat;
	float lon;
	char *iso2;
	char *iso3;
};

struct IsoToCountryMap {
	char iso2[3];
	uint16_t country_idx;
	UT_hash_handle hh;
};

struct CountryToCountryMap {
	char name[64];
	uint16_t country_idx;
	UT_hash_handle hh;
};

struct CityToGeoMap {
	char name[128];
	size_t geo_idx;
	UT_hash_handle hh;
};

#endif
