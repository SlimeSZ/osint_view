#include "gazetteer.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <uthash.h>
#include <stdbool.h>

#define MAX_GEOS 49000
#define MAX_COUNTRIES 250

/* parsing helpers */
static void add_iso_map(Gazetteer *gaz, const char *iso2, uint16_t country_idx) {
    IsoToCountryMap *map = malloc(sizeof(IsoToCountryMap));
    if (!map) return;
    strncpy(map->iso2, iso2, 2);
    map->iso2[2] = '\0';
    map->country_idx = country_idx;
    HASH_ADD_STR(gaz->iso_map, iso2, map);
}
static void add_country_map(Gazetteer *gaz, const char *name, uint16_t country_idx) {
    CountryToCountryMap *map = malloc(sizeof(CountryToCountryMap));
    if (!map) return;
    strncpy(map->name, name, sizeof(map->name) - 1);
    map->name[63] = '\0';
    map->country_idx = country_idx;
    HASH_ADD_STR(gaz->country_map, name, map);
}
static void add_city_map(Gazetteer *gaz, const char *name, size_t geo_idx) {
    CityToGeoMap *map = malloc(sizeof(CityToGeoMap));
    if (!map) return;
    strncpy(map->name, name, sizeof(map->name) - 1);
    map->name[127] = '\0';
    map->geo_idx = geo_idx;
    HASH_ADD_STR(gaz->geo_map, name, map);
}

static Gazetteer *gaz_init(void) {
	Gazetteer *gaz = calloc(1, sizeof(Gazetteer));
	if (!gaz) { fprintf(stderr, "malloc gaz_init()"); return NULL; }

	gaz->entries = calloc(MAX_GEOS, sizeof(GeoEntry));
	if (!gaz->entries) { fprintf(stderr, "calloc gaz_init()"); free(gaz); return NULL; }

	gaz->countries = calloc(MAX_COUNTRIES, sizeof(CountryIndex));
	if (!gaz->countries) { fprintf(stderr, "calloc gaz_init()"); free(gaz->entries); free(gaz); return NULL; }

	gaz->country_capacity = MAX_COUNTRIES;
	gaz->geo_capacity = MAX_GEOS;

	return gaz;
}

static CountryIndex *country_get_init(Gazetteer *gaz, const char *name, const char *iso2) {
	if (!gaz) return NULL;

	IsoToCountryMap *map = NULL;
	HASH_FIND_STR(gaz->iso_map, iso2, map);
	if (map) {
		return &gaz->countries[map->country_idx];
	}

	if (gaz->country_count >= gaz->country_capacity) { 
		fprintf(stderr, "max countries reached country_get_init()");
		return NULL;
	}

	CountryIndex *ci = &gaz->countries[gaz->country_count];
	ci->name = strdup(name);
	ci->geo_count = 0;
	ci->start_idx = 0;

	add_iso_map(gaz, iso2, gaz->country_count);
	add_country_map(gaz, name, gaz->country_count);

	gaz->country_count++;

	return ci;
}

/*
 * Parses a single CSV field from `entry` into `buf` (up to `bufsize-1` chars), 
 * handling quoted values. Terminates the string in `buf` with '\0', 
 * then advances `p` to the next comma (or end of string) and returns that position.
 *
 * Example:
 *   const char *line = "\"Kyiv\",Ukraine,50.45,30.523";
 *   char buf[32];
 *   const char *next = parse_field(line, buf, sizeof(buf));
 *   // After call:
 *   //   buf contains "Kyiv"
 *   //   next points to "Ukraine,50.45,30.523"
 */
static const char *parse_field(const char *entry, char *buf, size_t bufsize) {
	const char *p = entry;
	size_t i = 0;

	// skip whitespace
	while (*p == ' ')
		p++;
	
	// extract actual data enclosed within quotes
	if (*p == '"') {
		p++; // opening quote 
		while (*p && *p != '"' && i < bufsize - 1) {
			buf[i++] = *p++;
		}
		if (*p == '"')
			p++; // closing quote 
	} else {
		// TODO: impl
		printf("Unknown data -- unquoted");
	}

	buf[i] = '\0';

	// skip to next comma 
	while (*p && *p != ',')
		p++;
	if (*p == ',') 
		p++;

	return p;
}

static float conv_latlon(const char *str) {
	char *end_p;
	float val = strtof(str, &end_p);
	if (*end_p != 0 && !isspace(*end_p)) {
		fprintf(stderr, "Invalid lat/lon %s\n", str);
		return -1;
	}
	return val;
}

static int compare_by_iso2(const void *a, const void *b) {
	const GeoEntry *ga = (const GeoEntry*)a;
	const GeoEntry *gb = (const GeoEntry*)b;
	return strcmp(ga->iso2, gb->iso2);
}

/* API */
void gaz_free(Gazetteer *gaz) {
	if (!gaz) return;
	
	for (size_t i = 0; i < gaz->geo_count; i++) {
		GeoEntry *ge = &gaz->entries[i];
		free(ge->name);
		free(ge->iso2);
		free(ge->iso3);
	}
	
	for (size_t i = 0; i < gaz->country_count; i++) {
		CountryIndex *ci = &gaz->countries[i];
		free(ci->name);
	}

	IsoToCountryMap *iso_cur, *iso_tmp;
	HASH_ITER(hh, gaz->iso_map, iso_cur, iso_tmp) {
		HASH_DEL(gaz->iso_map, iso_cur);
		free(iso_cur);
	}

	CountryToCountryMap *country_cur, *country_tmp;
	HASH_ITER(hh, gaz->country_map, country_cur, country_tmp) {
		HASH_DEL(gaz->country_map, country_cur);
		free(country_cur);
	}
	
	CityToGeoMap *city_cur, *city_tmp;
	HASH_ITER(hh, gaz->geo_map, city_cur, city_tmp) {
		HASH_DEL(gaz->geo_map, city_cur);
		free(city_cur);
	}

	free(gaz->entries);
	free(gaz->countries);
	free(gaz);
}

Gazetteer *load_gaz(const char *filepath) {
	FILE *fp = fopen(filepath, "r");
	if (!fp) { fprintf(stderr, "fopen in load_gaz() %s\n", filepath); return NULL; }
	
	Gazetteer *gaz = gaz_init();
	if (!gaz) { fclose(fp); return NULL; }

	char line[1024];
	size_t line_no = 0;
	size_t empty_lines = 0;
	
	if (fgets(line, sizeof(line), fp)) line_no++; // skip 1st line (CSV headers)
	
	while (fgets(line, sizeof(line), fp)) {
		line_no++;
		line[strcspn(line, "\r\n")] = '\0';
		if (strlen(line) == 0) { empty_lines++; continue; }

		const char *p = line;
		char city[256];
		char city_ascii[256];
		char lat_str[32];
		char lon_str[32];
		char country[256];
		char iso2[8];
		char iso3[8];

		p = parse_field(p, city, sizeof(city));
		p = parse_field(p, city_ascii, sizeof(city_ascii));
		p = parse_field(p, lat_str, sizeof(lat_str));
		p = parse_field(p, lon_str, sizeof(lon_str));
		p = parse_field(p, country, sizeof(country));
		p = parse_field(p, iso2, sizeof(iso2));
		p = parse_field(p, iso3, sizeof(iso3));

		float lat = conv_latlon(lat_str);
		float lon = conv_latlon(lon_str);
		if (lat == -1 || lon == -1) {
			fprintf(stderr, "unable to validate_latlon() on line %zu\n", line_no);
			continue;
		}

		CountryIndex *ci = country_get_init(gaz, country, iso2); 
		if (!ci) continue;

		GeoEntry *ge = &gaz->entries[gaz->geo_count];
		ge->name = strdup(city);
		ge->iso2 = strdup(iso2);
		ge->iso3 = strdup(iso3);
		ge->lat = lat;
		ge->lon = lon;
		
		gaz->geo_count++;
	}
	
	fclose(fp);
	printf("Loaded %zu cities data -- %zu countries\n", gaz->geo_count, gaz->country_count);
	printf("No. empty lines: %zu\n", empty_lines);

	qsort(gaz->entries, gaz->geo_count, sizeof(GeoEntry), compare_by_iso2);
	
	for (size_t i = 0; i < gaz->country_count; i++) {
		gaz->countries[i].geo_count = 0;
		gaz->countries[i].start_idx = 0;
	}
	
	for (size_t i = 0; i < gaz->geo_count; i++) {
		IsoToCountryMap *map = NULL;
		HASH_FIND_STR(gaz->iso_map, gaz->entries[i].iso2, map);
		if (!map) continue;
		
		CountryIndex *ci = &gaz->countries[map->country_idx];
		if (ci->geo_count == 0) {
			ci->start_idx = i;
		}
		ci->geo_count++;
		
		add_city_map(gaz, gaz->entries[i].name, i);
	}

	return gaz;
}

CountryIndex *name_to_country(Gazetteer *gaz, const char *country_name) {
	CountryToCountryMap *map = NULL;
	HASH_FIND_STR(gaz->country_map, country_name, map);
	return map ? &gaz->countries[map->country_idx] : NULL;
}

CountryIndex *iso_to_country(Gazetteer *gaz, const char *iso2) {
	IsoToCountryMap *map = NULL;
	HASH_FIND_STR(gaz->iso_map, iso2, map);
	return map ? &gaz->countries[map->country_idx] : NULL;
}

GeoEntry *name_to_geo(Gazetteer *gaz, const char *geo_name) {
	CityToGeoMap *map = NULL;
	HASH_FIND_STR(gaz->geo_map, geo_name, map);
	return map ? &gaz->entries[map->geo_idx] : NULL;
}

void show_geos(Gazetteer *gaz, const char *country_name, FILE *out) {
	if (!gaz || !country_name || !out) return;
	CountryIndex *ci = name_to_country(gaz, country_name);
	if (!ci) { fprintf(out, "Country %s not found\n", country_name); return; }

	fprintf(out, "%s [%zu geo-entries]: \n", ci->name, ci->geo_count);
	for (size_t i = 0; i < ci->geo_count; i++) {
		GeoEntry *ge = &gaz->entries[ci->start_idx + i];
		fprintf(out, " %s | %.3f | %.3f | %s\n", 
			ge->name, ge->lat, ge->lon, ge->iso2
		);
	}
}

int main(void) {
	const char*country = "Ukraine";
	Gazetteer *gaz = load_gaz("data/worldcities.csv");
	if (!gaz) return 1;
	show_geos(gaz, country, stdout);
	gaz_free(gaz);
	return 0;
}

