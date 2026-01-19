#include "demonymizer.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "gazetteer.h"


static void add_demonym_map(Demonymizer *dem, const char *demonym, const char *country) {
	size_t dem_len = strnlen(demonym, sizeof(((DemonymMap *)0)->demonym));
	size_t country_len = strnlen(country, sizeof(((DemonymMap *)0)->country));
	
	if (dem_len == 0 || country_len == 0)
		return;

	DemonymMap *map = malloc(sizeof(DemonymMap));
	if (!map) {
		perror("malloc");
		return;
	}
	memset(map, 0, sizeof(DemonymMap));


	strncpy(map->demonym, demonym, sizeof(map->demonym) - 1);
	map->demonym[sizeof(map->demonym) - 1] = '\0';

	strncpy(map->country, country, sizeof(map->country) - 1);
	map->country[sizeof(map->country) - 1] = '\0';

	DemonymMap *existing = NULL;
	HASH_FIND_STR(dem->demonym_map, map->demonym, existing);
	if (existing) {
		HASH_DEL(dem->demonym_map, existing);
		free(existing);
	}

	HASH_ADD_STR(dem->demonym_map, demonym, map);
}

static Demonymizer *dem_init(void) {
	Demonymizer *dem = calloc(1, sizeof(Demonymizer));
	if (!dem) { perror("calloc"); return NULL; }
	return dem;
}

void free_dem(Demonymizer *dem) {
	if (!dem) return;

	DemonymMap *dem_cur, *dem_tmp;
	HASH_ITER(hh, dem->demonym_map, dem_cur, dem_tmp) {
		HASH_DEL(dem->demonym_map, dem_cur);
		free(dem_cur);
	}
	free(dem);
}

static inline char *iso_to_countrychar(Gazetteer *gaz, const char *iso2) {
	CountryIndex *ci = iso_to_country(gaz, iso2);		
	if (!ci) return NULL;
	return ci->name;
}

static inline char *adj_to_demonymchar(Demonymizer *dem, const char *token) {
	
	return NULL;
}

static const char *parse_field(const char *entry, char *buf, size_t bufsize) {
	const char *p = entry;
	size_t i = 0;


	return NULL;
}

Demonymizer *load_dem(const Gazetteer *gaz, const char *filepath) {
	if (!gaz || !filepath) 
		return NULL; 
	
	FILE *fp = fopen(filepath, "r");
	Demonymizer *dem = dem_init();
	if (!dem) { fclose(fp); return NULL; }

	char line[1024];
	size_t line_no = 0;
	size_t empty_lines = 0;

	if (fgets(line, sizeof(line), fp))
		line_no++;

	while (fgets(line, sizeof(line), fp)) {
		line_no++;
		line[strcspn(line, "\r\n")] = '\0';
		if (strlen(line) == 0) {
			empty_lines++;
			continue;
		}

		const char *p = line;
		char demonym[64];
		char country[64];
		p = parse_field(p, demonym, sizeof(demonym));
		p = parse_field(p, country, sizeof(country));
	}

	fclose(fp);
	printf(" \n");
	printf("No. empty lines: %zu\n", empty_lines);

	
	return dem;
}


const char *resolve_dem(Demonymizer *dem, const char *token);

int main(void) {

}
