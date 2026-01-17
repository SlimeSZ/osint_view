#include "gazetteer.h" 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <uthash.h>

/* Geo Configs */
#define MAX_GEOENTRIES 49000
#define MAX_COUNTRIES 200

/* Parsing Configs */
#define MAX_LINE 1024
#define MAX_LINE_NO 50000

static Gazetteer *gaz_init(void) {
	Gazetteer *gaz = malloc(sizeof(Gazetteer));
	if (!gaz) {
		fprintf(stderr, "Err: malloc");
		return NULL;
	}
	gaz->entries = calloc(MAX_GEOENTRIES, sizeof(GeoEntry));
	if (!gaz->entries) {
		fprintf(stderr, "Err: calloc");
		free(gaz);
		return NULL;
	}
	gaz->geo_capacity = MAX_GEOENTRIES; 
	gaz->countries = calloc(MAX_COUNTRIES, sizeof(CountryIndex));
	if (!gaz->countries) {
		fprintf(stderr, "Err: calloc");
		free(gaz->entries);
		free(gaz);
		return NULL;
	}
	gaz->country_capacity = MAX_COUNTRIES;
	return gaz;
}

void gaz_free(Gazetteer *gaz) {
	if (!gaz) return;
	for (size_t i = 0; i < gaz->geo_count; i++) {
		GeoEntry *ge = &gaz->entries[i];
		free(ge->city_name);
		free(ge->iso2);
		free(ge->iso3);
	}
	for (size_t i = 0; i < gaz->country_count; i++) {
		CountryIndex *ci = &gaz->countries[i];
		free(ci->iso2);
		free(ci->name);
	}
	free(gaz->entries);
	free(gaz->countries);
	free(gaz);
}

/* Called whilst parsing CSV to help build country indices */
static CountryIndex *country_get_init(Gazetteer *gaz, const char *iso2, const char *country_name) {
	if (!gaz) return NULL;
	// look up via hashmap (iso2 -> CountryIndex) in case country already exists & return it 
	IsoToCountryMap *map = NULL;
	HASH_FIND_STR(gaz->country_hashmap, iso2, map);
	if (map) {
		return &gaz->countries[map->country_idx];
	}
	if (gaz->country_count >= gaz->country_capacity) {
		fprintf(stderr, "country count exceeded MAX_COUNTRIES");
		return NULL;
	}
	// return newly created
	CountryIndex *ci = &gaz->countries[gaz->country_count];
	ci->iso2 = strdup(iso2);
	ci->name = strdup(country_name);
	ci->city_count = 0;
	/* Works ONLY if CSV is grouped by country which it is for data we use here but something to keep in mind */
	// ci->start_idx = gaz->geo_count;

	IsoToCountryMap *new_map = malloc(sizeof(IsoToCountryMap));
	if (!new_map) { perror("malloc"); return NULL; }
	strncpy(new_map->iso2, iso2, sizeof(new_map->iso2) - 1);
	new_map->iso2[2] = '\0';
	new_map->country_idx = gaz->country_count;
	HASH_ADD_STR(gaz->country_hashmap, iso2, new_map);

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

static int compare_by_iso2(const void *a, const void *b) {
	const GeoEntry *ga = (const GeoEntry*)a;
	const GeoEntry *gb = (const GeoEntry*)b;
	return strcmp(ga->iso2, gb->iso2);
}

Gazetteer *load_gaz(const char *filepath) {
	FILE *fp = fopen(filepath, "r");
	if (!fp) {
		fprintf(stderr, "Err: cannot open via fopen: %s\n", filepath);
		return NULL;
	}

	Gazetteer *gaz = gaz_init();
	if (!gaz) {
		fclose(fp);
		return NULL;
	}

	char line[MAX_LINE];
	size_t line_no = 0;

	// debugging 
	size_t empty_lines = 0;	

	// skip first line
	if (fgets(line, sizeof(line), fp)) {
		line_no++;
	}

	while (fgets(line, sizeof(line), fp)) {
		line_no++;
		line[strcspn(line, "\r\n")] = '\0';
		if (strlen(line) == 0) {
			empty_lines++;
			continue;
		}
		const char *p = line;
		char city[256];
		char city_ascii[256];
		char lat_str[32];
		char lon_str[32];
		char country[256];
		char iso2[8];
		char iso3[8];
		
		/* parse each field */ 
		// cols: 
		// "city","city_ascii","lat","lng","country","iso2","iso3","admin_name","capital","population","id"
		p = parse_field(p, city, sizeof(city));
		p = parse_field(p, city_ascii, sizeof(city_ascii));
		p = parse_field(p, lat_str, sizeof(lat_str));
		p = parse_field(p, lon_str, sizeof(lon_str));
		p = parse_field(p, country, sizeof(country));
		p = parse_field(p, iso2, sizeof(iso2));
		p = parse_field(p, iso3, sizeof(iso3));
		
		/* validate lat/lon's (optional) -- handles "50.45 N" or "50.45 */
		char *end_p;
		float lat = strtof(lat_str, &end_p);
		if (*end_p != '\0' && !isspace(*end_p)) {
			fprintf(stderr, "Invalid lat on line %zu, skipping\n", line_no);
			continue;
		}
		float lon = strtof(lon_str, &end_p); 
			if (*end_p != '\0' && !isspace(*end_p)) {
				fprintf(stderr, "Invalid lon on line %zu, skipping\n", line_no);
				continue;
		}

		CountryIndex *ci = country_get_init(gaz, iso2, country);
		if (!ci) 
			continue;
		
		GeoEntry *ge = &gaz->entries[gaz->geo_count];
		ge->city_name = strdup(city);
		ge->iso2 = strdup(iso2);
		ge->iso3 = strdup(iso3);
		ge->lat = lat;
		ge->lon = lon;
		gaz->geo_count++;
	}

	fclose(fp);
	printf("Loaded %zu cities data -- %zu countries\n", gaz->geo_count, gaz->country_count);
	printf("No. empty lines: %zu\n", empty_lines);

	/* Rebuild country indices to group by country  */
	qsort(gaz->entries, gaz->geo_count, sizeof(GeoEntry), compare_by_iso2);
	for (size_t i = 0; i < gaz->country_count; i++) {
		gaz->countries[i].city_count = 0;
		gaz->countries[i].start_idx = 0;
	}
	for (size_t i = 0; i <gaz->geo_count; i++) {
		IsoToCountryMap *map = NULL;
		HASH_FIND_STR(gaz->country_hashmap, gaz->entries[i].iso2, map);
		if (!map)
			continue;
		CountryIndex *ci = &gaz->countries[map->country_idx];
		if (ci->city_count == 0) {
			ci->start_idx = i;
		}
		ci->city_count++;
	}

	return gaz;
}

int main(void) {
	Gazetteer *gaz = load_gaz("data/worldcities.csv");
	if (!gaz) {
		fprintf(stderr, "Err (main): failed to load gaz\n");
		return 1;
	}

	printf("\nCountries & Cities data:\n");

	size_t printed_countries = 0;
	for (size_t c = 0; c < gaz->country_count && printed_countries < 100000; c++) {
		CountryIndex *country = &gaz->countries[c];
		printf("Country: %s (%s) | cities: %zu | start_idx: %zu\n",
				country->name, country->iso2, country->city_count, country->start_idx);
		for (size_t i = 0; i < country->city_count; i++) {
			size_t geo_idx = country->start_idx + i;
			if (geo_idx >= gaz->geo_count) break;

			GeoEntry *ge = &gaz->entries[geo_idx];
			printf("  %s | %s | %s | %.3f | %.3f\n",
			       ge->city_name, ge->iso2, ge->iso3, ge->lat, ge->lon);
		}

		printed_countries++;
	}

	gaz_free(gaz);
	return 0;
}





