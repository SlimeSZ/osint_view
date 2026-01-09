#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#define RESP_BUF 1024

struct resp_buf {
	char *data;
	size_t len;
	size_t capacity;
};

size_t write_callback(
		void *data,
		size_t size,
		size_t nmemb,
		void *userp
	) {
	size_t n = size * nmemb;
	struct resp_buf *resp = (struct resp_buf*)userp;
	
	if (resp->len + n + 1 > resp->capacity) {
		size_t new_capacity = resp->capacity * 2;
		while (new_capacity < resp->len + n + 1) 
			new_capacity *= 2;
		char *temp = realloc(resp->data, new_capacity);
		if (!temp) {
			fprintf(stderr, "Err: Failed to realloc\n");
			return 0;
		}
		resp->data = temp;
		resp->capacity *= 2;
	}
	
	memcpy(resp->data + resp->len, data, n);
	resp->len += n;
	resp->data[resp->len] = '\0';
	
	return n;
}

int main(void) {
	char *url;

	CURL *curl;
	CURLcode rcode;
	struct curl_slist *headers = NULL;
	struct resp_buf resp;
	resp.data = malloc(RESP_BUF);
	if (!resp.data) {
		fprintf(stderr, "Err: malloc failed");
		return 1;
	}
	resp.len = 0;
	resp.capacity = RESP_BUF;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	
	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "ERR: Failed to create curl handle");
		curl_global_cleanup();
		return 1;
	}

	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	// URL + GET 
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	// resp. body handling 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);

	rcode = curl_easy_perform(curl);
	if (rcode != CURLE_OK) {
		fprintf(stderr, "Err: bad rcode: %s\n", curl_easy_strerror(rcode));
	}

	printf("Data (%zu bytes) Received: %s\n", resp.len, resp.data);

	free(resp.data);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}
