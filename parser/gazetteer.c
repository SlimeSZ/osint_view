#include "gazetteer.h" 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define INITIAL_CAPACITY 48000
#define MAX_LINE 1024
#define MAX_LINE_NO 50000
#define MAX_COUNTRIES 300

static Gazetteer *gazetteer_init(void) {
	Gazetteer *gaz = malloc(sizeof(Gazetteer));
	if (__builtin_expect(!gaz, 0)) {
		fprintf(stderr, "Err: malloc");
		return NULL;
	}
	gaz->entries = calloc(INITIAL_CAPACITY, sizeof(GeoEntry));
	if (!gaz->entries) {
		fprintf(stderr, "Err: calloc");
		free(gaz);
		return NULL;
	}
	gaz->countries = calloc(MAX_COUNTRIES, sizeof(CountryIndex));
	if (!gaz->countries) {
		fprintf(stderr, "Err: calloc");
		free(gaz);
		free(gaz->entries);
		return NULL;
	}

	gaz->count = 0;
	gaz->capacity = INITIAL_CAPACITY; 
	gaz->country_count = 0;
	return gaz;
}

void free_gazetteer(Gazetteer *gaz) {
	if (__builtin_expect(!gaz, 0))
		return;
	for (size_t i = 0; i < gaz->count; i++) {
		GeoEntry *e = &gaz->entries[i];
		free(e->city_name);
		free(e->iso2);
		free(e->iso3);
	}
	for (size_t i = 0; i < gaz->country_count; i++) {
		CountryIndex *c = &gaz->countries[i];
		free(c->iso2);
		free(c->name);
	}
	free(gaz->countries);
	free(gaz->entries);
	free(gaz);
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

static CountryIndex *country_idx_get_init(Gazetteer *gaz, const char *iso2, const char *name) {
	// find existing
	for (size_t i = 0; i < gaz->country_count; i++) {
		if (strcmp(gaz->countries[i].iso2, iso2) == 0)
			return &gaz->countries[i];
	}
	if (gaz->country_count >= MAX_COUNTRIES) {
		fprintf(stderr, "Err: exceeeded MAX COUNTRIES\n");
		return NULL;
	}
	// create new country idx
	CountryIndex *country = &gaz->countries[gaz->country_count];
	country->iso2 = strdup(iso2);
	country->name = strdup(name);
	country->start_idx = gaz->count;
	country->city_count = 0;
	gaz->country_count++;
	return country;
}
	
Gazetteer *load_gazetteer(const char *filepath) {
	FILE *fp = fopen(filepath, "r");
	if (!fp) {
		fprintf(stderr, "Err: cannot fopen: %s\n", filepath);
		return NULL;
	}
	Gazetteer *gaz = gazetteer_init();
	if (!gaz) {
		fclose(fp);
		return NULL;
	}

	char line[MAX_LINE];
	size_t line_no = 0;
	size_t lines_left = MAX_LINE_NO;

	while (fgets(line, sizeof(line), fp)) {
		line_no++;
		lines_left--;
		line[strcspn(line, "\r\n")] = '\0';
		if (strlen(line) == 0)
			continue;
		
		const char *p = line;
		char city[256];
		char city_ascii[256];
		char lat_str[32];
		char lon_str[32];
		char country[256];
		char iso2[8];
		char iso3[8];

		// parse each field 
		// cols: 
		// "city","city_ascii","lat","lng","country","iso2","iso3","admin_name","capital","population","id"
		p = parse_field(p, city, sizeof(city));
		p = parse_field(p, city_ascii, sizeof(city_ascii));
		p = parse_field(p, lat_str, sizeof(lat_str));
		p = parse_field(p, lon_str, sizeof(lon_str));
		p = parse_field(p, country, sizeof(country));
		p = parse_field(p, iso2, sizeof(iso2));
		p = parse_field(p, iso3, sizeof(iso3));
		
		// validate lat/lon's (optional)
		// handles "50.45 N" or "50.45 "
		char *end_p;
		float lat = strtof(lat_str, &end_p);
		if (*end_p != '\0' && !isspace(*end_p)) {
			fprintf(stderr, "Invalid lat on line %zu\n", line_no);
			continue;
		}
		float lon = strtof(lon_str, &end_p);
		if (*end_p != '\0' && !isspace(*end_p)) {
			fprintf(stderr, "Warning: invalid lon on line %zu, skipping\n", line_no);
			continue;
		}

		// realloc space if need be
		if (gaz->count >= gaz->capacity) {
			size_t old_cap = gaz->capacity;
			size_t new_cap = gaz->capacity + lines_left;
			GeoEntry *tmp = realloc(gaz->entries, new_cap * sizeof(GeoEntry));
			if (!tmp) {
				fprintf(stderr, "Err: realloc failed\n");
				return NULL;
			}
			gaz->entries = tmp;
			gaz->capacity = new_cap;
			memset(&gaz->entries[old_cap], 0, 
				(new_cap - old_cap) * sizeof(GeoEntry)
			);
		}

		CountryIndex *c_idx = country_idx_get_init(gaz, iso2, country);
		if (!c_idx) continue;
		GeoEntry *entry = &gaz->entries[gaz->count];
		entry->city_name = strdup(entry->city_name);
		entry->iso2 = strdup(iso2);
		entry->iso3 = strdup(iso3);
		entry->lat = lat;
		entry->lon = lon;
		entry->country = c_idx;
		c_idx->city_count++;
		gaz->count++; 	
	}
	
	fclose(fp);
	printf("Loaded %zu cities' data\n", gaz->count);
	return gaz;
}

int main(void) {
	Gazetteer *gaz = load_gazetteer("data/worldcities.csv");
	if (!gaz) {
		fprintf(stderr, "Err (main): failed to load gaz");
		return 1;
	}
	printf("\nFirst 10 GeoEntries:\n");
	for (size_t i = 0; i < gaz->capacity && i < gaz->count; i++) {
		GeoEntry *ge = &gaz->entries[i];

	}
	free_gazetteer(gaz);
	return 0;
}
