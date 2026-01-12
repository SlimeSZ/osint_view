#ifndef SCHEMA_PARSER_H
#define SCHEMA_PARSER_H 

#include <cstddef>
#include <cstdint>
#include <stdint.h>

/* Per JsonFieldDesc validators -- not mutually exclusive (e.g. FIELD_REQUIRED | FIELD_NULLABLE) */
#define FIELD_REQUIRED (1u << 0) // Anything else is implicitly 'field optional' 
#define FIELD_NULLABLE (1u << 1) 
#define FIELD_IGNORE (1u << 2)

/* Json Types -- represented as 2^3 mutually exclusive states */
typedef enum {
	JSON_INVALID,
	JSON_NULL,
	JSON_BOOL,
	JSON_I64,  
	JSON_F64,   
	JSON_STRING,
	JSON_ARRAY,
	JSON_OBJECT,
} JsonType;
_Static_assert(JSON_OBJECT < 8, "JsonType must fit in 3 bits");

/* Storage Mapping (parsed val -> struct field) -- represented as 9 mutually exclusive states */
typedef enum {
	STORE_I32,
	STORE_I64,
	STORE_F64,
	STORE_BOOL,
	STORE_STR_STATIC,
	STORE_STR_DYN, 		// for later str extension
	STORE_NESTED, 
	STORE_ARRAY_STATIC,
	STORE_ARRAY_DYN,
} StoreType;
_Static_assert(STORE_ARRAY_DYN < 16, "StoreType must fit in 4 bits");

/* Schema descriptor router for known JSON structures */
typedef struct JsonFieldDesc JsonFieldDesc;
struct JsonFieldDesc {
	// 32-bytes
	const char *key; 	  
	uint32_t key_hash; 	
	uint16_t key_len;	
	uint16_t offset;  	
	
	uint8_t json_type : 3;	// 2^3 = 8 JSON_TYPE states, 3 bits pack perfectly
	uint8_t store_type : 4; // 9 STORE_TYPE states, need at least 2^4 bits to pack 
	uint8_t _pad_1 : 1;
	uint8_t field_flags;     
	uint8_t _pad_2[14];
	
	// 32-bytes
	union {
		// string 
		struct {
			uint16_t max_len; // prevent buffer overflow
		} str;
		// object 
		struct {
			const JsonFieldDesc *nested_fields; // classic approach for nested schema 
			uint16_t nested_count;
		} object;
		
		// array -- I have split primitive and object arrays for separation of concerns  
		union {
			struct {
				uint16_t max_elem;
				uint16_t elem_size; 
			} prim_array;
			struct {
				uint16_t max_elem;
				const JsonFieldDesc *elem_desc;
				// no eleme_size as it's deterministic for non-primitive arrays
				uint16_t elem_count; 
			} obj_array;
		} array_meta;

		uint64_t _pad_3[4];
	} meta;
} __attribute__((packed, aligned(64)));
_Static_assert(sizeof(JsonFieldDesc) == 64, "JsonFieldDesc struct must be 64 bytes");
_Static_assert(offsetof(JsonFieldDesc, key_hash) == 8, "key_hash must be ast byte 8");

/* Descriptor array + metadata, core blueprint for JSON string -> Struct mapping  */
typedef struct {
	const JsonFieldDesc *fields;		// array of members of C struct 
	uint16_t field_count;			// total no. fields in C struct
	uint16_t sizeof_struct;			// C struct's size  
} Schema;

#define MAX_NESTING_DEPTH 16

/* A frame is simply the data contained within "{ ..." or "[ ... " in JSON */
typedef enum { CONTAINER_OBJECT, CONTAINER_ARRAY, } Container;
typedef struct {
	Container type;				 
	uint16_t parsed_count; 			// elements parsed thus far, 0 for objects since indexing is irrelevant  
	const JsonFieldDesc *curr_field; 	// NULL if array, parsing {K, V} fields requires additional metadata tracking

	/*	These might not even be right, they are simply here for my sake
	 * root entity: simply the target struct 
	 * nested field: struct + offset
	 * array element: array_base + (parsed_count * sizeof(array[parsed_count])) 	(roughly)
	*/
	void *base_ptr;

	uint64_t fields_seen;			// Bitset: i'th field parsed if set 
	uint64_t fields_null;			// Bitset: i'th field null if set 
} ParseFrame;

/* Persistent & Iterative Parser Context */
typedef struct {
	const char *curr;			// curr pos. in JSON string 
	const char *json_end;   		// end of JSON buffer 
	void *target;				// target struct to be filled 
	const Schema *schema;  			// target struct schema 

	ParseFrame *curr_frame;		
	ParseFrame frames[MAX_NESTING_DEPTH]; 
	uint8_t depth; 				// curr nested depth, tracks next frame to parse	

	uint32_t err_code;			// 0 = success, 1 - 4 = err 
	uint32_t err_offset; 			// byte offset for key that caused err 
	const char *err_key;			// key at err_offset
} parser_ctx_t;

/* Parser Context error codes */
#define PARSE_OK 		0 
#define PARSE_ERR_MALFORMED 	1
#define PARSE_ERR_OVERFLOW 	2 
#define PARSE_ERR_TYPE 		3 
#define PARSE_ERR_MISSING 	4

/* API (not final), just throwing out what comes to mind */


#endif 

