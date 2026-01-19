#ifndef DEMONYMIZER_H
#define DEMONYMIZER_H

#include "gazetteer.h"
#include <uthash.h>

typedef struct Demonymizer Demonymizer;
typedef struct DemonymMap DemonymMap; 
typedef struct IsoToCountryMap IsoToCountryMap;

struct Demonymizer {
	IsoToCountryMap *iso_map; 
	DemonymMap *demonym_map;
};

struct DemonymMap {
	char demonym[64];
	char country[64];
	UT_hash_handle hh;
};

static inline char *iso_to_countrychar(Gazetteer *gaz, const char *iso2);
static inline char *adj_to_demonymchar(Demonymizer *dem, const char *token);
Demonymizer *load_dem(const Gazetteer *gaz, const char *filepath);
void free_dem(Demonymizer *dem);
const char *resolve_dem(Demonymizer *dem, const char *token);


#endif 
