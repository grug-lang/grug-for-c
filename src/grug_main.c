#include <alloca.h>
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "grug_main.h"
#include "grug_arena.h"
#include "grug_options.h"

// MARK: utilities

// TODO(bluesillybeard): add a way to have a user-defined realloc, and fall back to this otherwise
void* grug_realloc(void* ptr, size_t old_len, size_t new_len) {
	if(!ptr) {
		return GRUG_MALLOC(new_len);
	}
	assert(new_len >= old_len);
	if(new_len == old_len) {
		return ptr;
	}
	void* new_ptr = GRUG_MALLOC(new_len);
	if(!new_ptr) {
		return 0;
	}
	memcpy(new_ptr, ptr, old_len);
	GRUG_FREE(ptr, old_len);
	return new_ptr;
}

static char* read_all_contents(char const* file_path, size_t* out_len) {
	struct block {
		char data[1024];
		size_t data_len;
		struct block* pnext;
	};

	struct block* first = 0;
	struct block* last = 0;
	size_t total_size = 0;

	FILE* file = fopen(file_path, "rb");

	if(!file) {
		if(out_len) {
			*out_len = 0;
		}
		return NULL;
	}

	first = GRUG_MALLOC(sizeof(struct block));

	first->data_len = fread(first->data, 1, 1024, file);
	total_size += first->data_len;

	last = first;

	while(!feof(file)) {
		struct block* new = GRUG_MALLOC(sizeof(struct block));
		last->pnext = new;
		last = new;
		last->data_len = fread(last->data, 1, 1024, file);
		total_size += last->data_len;
	}

	if(!fclose(file)) {
		// I'm not sure under what circumstances this can fail but may as well log it somewhere.
		// TODO(bluesillybeard) we really should figure out a proper logging system, even it it's just user-defined print* functions
		printf("GRUG: Failed to close file\n");
	}

	char* data = GRUG_MALLOC(total_size + 1);
	size_t data_written = 0;
	while(first) {
		memcpy(data + data_written, first->data, first->data_len);
		data_written += first->data_len;
		struct block* next = first->pnext;
		GRUG_FREE(first, sizeof(struct block));
		first = next;
	}
	if(out_len) {
		*out_len = total_size;
	}
	return data;
}

#define MIN(x, y) ((x)>(y) ? (y) : (x))

// MARK: private functions

struct grug_state {
	struct grug_arena* update_arena;
	struct grug_error last_error;
};

static void write_error_plain(grug_error_type error_type, char const* message, char const* custom_message, struct grug_file_location file, struct grug_error* out_error) {
	if(out_error) {
		memset(out_error, 0, sizeof(struct grug_error));
		out_error->error_type = error_type;
		if(message) {
			size_t message_len = strlen(message);
			memcpy(out_error->message, message, MIN(message_len+1, sizeof(out_error->message)));
			if(!custom_message) {
				memcpy(out_error->custom_message, message, MIN(message_len, sizeof(out_error->custom_message)));
			}
		}
		if(custom_message) {
			size_t message_len = strlen(custom_message);
			memcpy(out_error->custom_message, custom_message, MIN(message_len+1, sizeof(out_error->custom_message)));
			if(!message) {
				memcpy(out_error->message, custom_message, MIN(message_len+1, sizeof(out_error->message)));
			}
		}
		memcpy(&out_error->file, &file, sizeof(struct grug_file_location));
	}
}

static void write_error(struct grug_state* gst, grug_error_type error_type, char const* message, char const* custom_message, struct grug_file_location file, struct grug_error* out_error) {
	write_error_plain(error_type, message, custom_message, file, out_error);
	if(gst) {
		write_error_plain(error_type, message, custom_message, file, &gst->last_error);
	}
}

// MARK: public functions

struct grug_init_settings grug_default_settings(void) {
	return (struct grug_init_settings) {
		.mod_api_json_source = "",
		.mods_dir_path = "",
		.runtime_error_handler = {0},
		.logger = {0},
		.backend = {0},
	};
}

/// Returns null upon an error and writes to out_error
struct grug_state* grug_init(struct grug_init_settings settings, struct grug_error* out_error) {
	(void)settings;
	struct grug_state* gst = GRUG_MALLOC(sizeof(struct grug_state));
	if(!gst) {
		write_error_plain(GRUG_ERROR_TYPE_INIT, "Failed to create state: malloc() returned null", NULL, (struct grug_file_location){0}, out_error);
		return NULL;
	}
	struct grug_arena* arena = grug_arena_new();
	if(!arena) {
		write_error_plain(GRUG_ERROR_TYPE_INIT, "Failed to create state: grug_arena_new() returned null", NULL, (struct grug_file_location){0}, out_error);
		GRUG_FREE(gst, sizeof(struct grug_state));
		return NULL;
	}
	*gst = (struct grug_state) {
		.update_arena = arena,
	};
	return gst;
}

struct grug_error const* grug_get_error(struct grug_state* gst) {
	return &gst->last_error;
}

struct grug_callstack grug_get_callstack(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
	return (struct grug_callstack){0};
}

bool grug_register_game_fn(struct grug_state* gst, char const* game_fn_name, void* fn_data, game_fn fn_ptr) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)game_fn_name;
	(void)fn_data;
	(void)fn_ptr;
	return false;
}

bool grug_all_game_functions_registered(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
	return false;
}

grug_on_fn_id grug_get_on_fn_id(struct grug_state* gst, const char* entity_type, const char* on_fn_name) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity_type;
	(void)on_fn_name;
	return 0;
}

struct grug_on_fns grug_get_fn_ids(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
	return (struct grug_on_fns){0};
}

grug_file_id grug_compile_file(struct grug_state* gst, const char* path) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)path;
	return 0;
}

grug_file_id grug_compile_file_from_str(struct grug_state* gst, const char* path, char const* file_text) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)path;
	(void)file_text;
	return 0;
}

const struct grug_mod_dir* grug_get_mods(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
	return NULL;
}

grug_entity_id grug_create_entity(struct grug_state* gst, grug_file_id script, grug_object_id me_id) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)script;
	(void)me_id;
	return 0;
}

grug_file_id grug_entity_get_file_id(struct grug_state* gst, grug_entity_id entity) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity;
	return 0;
}

struct grug_entity* grug_entity_get_data(struct grug_state* gst, grug_entity_id entity) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity;
	return NULL;
}

void grug_deinit_entity(struct grug_state* gst, grug_entity_id entity) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity;
}

struct grug_updates_list grug_update(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
	return (struct grug_updates_list){0};
}

void grug_deinit(struct grug_state* gst) {
	assert(false && "Not Implemented");
	(void)gst;
}

void grug_swap_backend(struct grug_state* gst, struct grug_backend backend) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)backend;
}

void grug_set_fast_mode(struct grug_state* gst, bool fast) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)fast;
}

bool grug_call_on_function_raw(struct grug_state* gst, grug_entity_id entity, grug_on_fn_id on_fn_id, union grug_value* args) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity;
	(void)on_fn_id;
	(void)args;
	return false;
}

bool grug_call_on_function(struct grug_state* gst, grug_entity_id entity, grug_on_fn_id on_fn_id, union grug_value* args, size_t args_len) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)entity;
	(void)on_fn_id;
	(void)args;
	(void)args_len;
	return false;
}

void grug_game_fn_runtime_error(struct grug_state* gst, char const* message) {
	assert(false && "Not Implemented");
	(void)gst;
	(void)message;
}

struct grug_arena* grug_arena_new(void) {
	struct grug_internal_arena* arena = GRUG_MALLOC(sizeof(struct grug_internal_arena));
	if(!arena) {
		return NULL;
	}

	grug_internal_arena_init(arena, 0, 8192);

	return (struct grug_arena*)arena;
}

void* grug_arena_alloc(struct grug_arena* arena, size_t size) {
	if(arena) {
		return grug_internal_arena_allocate((struct grug_internal_arena*)arena, size);
	}
	return NULL;
}

void* grug_arena_alloc_aligned(struct grug_arena* arena, size_t size, size_t align) {
	if(arena) {
		return grug_internal_arena_allocate_aligned((struct grug_internal_arena*)arena, size, align);
	}
	return NULL;
}

void grug_arena_free(struct grug_arena* arena, void* ptr, size_t size) {
	if(arena) {
		grug_internal_arena_free((struct grug_internal_arena*)arena, ptr, size);
	}
}

void* grug_arena_realloc(struct grug_arena* arena, void* ptr, size_t old_size, size_t new_size) {
	if(arena) {
		return grug_internal_arena_reallocate((struct grug_internal_arena*)arena, ptr, old_size, new_size);
	}
	return NULL;
}

void grug_arena_clear(struct grug_arena* arena, size_t reserve_bytes) {
	if(arena) {
		grug_internal_arena_reset((struct grug_internal_arena*)arena, reserve_bytes);
	}
}

void grug_arena_deinit(struct grug_arena* arena) {
	if(arena) {
		grug_internal_arena_deinit((struct grug_internal_arena*)arena);
		GRUG_FREE(arena, sizeof(struct grug_internal_arena));
	}
}

void grug_free_ast(struct grug_ast ast) {
	assert(false && "Not Implemented");
	(void)ast;
}

size_t grug_to_tokens(char const* grug, size_t grug_len, struct grug_tokens* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)grug;
	(void)grug_len;
	(void)out_tokens;
	(void)out_tokens_capacity;
	(void)o_error;
	return 0;
}

size_t ast_to_tokens(struct grug_ast ast, struct grug_tokens* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)ast;
	(void)out_tokens;
	(void)out_tokens_capacity;
	(void)o_error;
	return 0;
}

size_t json_to_grug(char const* json, size_t json_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)json;
	(void)json_len;
	(void)out_string_buffer;
	out_string_buffer[0] = 0; // To silence a clang-tidy complaint
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}

size_t tokens_to_grug(struct grug_tokens tokens, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)tokens;
	(void)out_string_buffer;
	out_string_buffer[0] = 0; // To silence a clang-tidy complaint
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}

size_t ast_to_grug(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)ast;
	(void)out_string_buffer;
	out_string_buffer[0] = 0; // To silence a clang-tidy complaint
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}

struct grug_ast tokens_to_ast(struct grug_tokens tokens, struct grug_arena* arena, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)tokens;
	(void)arena;
	(void)o_error;
	return (struct grug_ast){0};
}

struct grug_ast json_to_ast(char const* json, size_t json_len, struct grug_arena* arena, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)json;
	(void)json_len;
	(void)arena;
	(void)o_error;
	return (struct grug_ast){0};
}

struct grug_ast grug_to_ast(char const* grug, size_t grug_len, struct grug_arena* arena, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)grug;
	(void)grug_len;
	(void)arena;
	(void)o_error;
	return (struct grug_ast){0};
}

size_t ast_to_json(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)ast;
	(void)out_string_buffer;
	out_string_buffer[0] = 0; // To silence a clang-tidy complaint
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}

size_t grug_to_json(char const* grug, size_t grug_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)grug;
	(void)grug_len;
	(void)out_string_buffer;
	out_string_buffer[0] = 0; // To silence a clang-tidy complaint
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}
