#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "grug_main.h"
#include "beard_arena.h"
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

/// Returns a null terminated string of the entire contents of a file, or null if the file fails to open.
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
	data[total_size] = 0;
	return data;
}

// MARK: private functions

struct grug_state {
	struct grug_arena* update_arena;
	struct grug_error last_error;
	char const* mod_api_json_source;
	struct grug_logger logger;
	struct grug_backend backend;
	bool fast_mode;
};

static void write_error_plain(struct grug_error_code error_code, char const* message, char const* custom_message, struct grug_file_location file, struct grug_callstack callstack, struct grug_arena* arena_or_none, struct grug_error* out_error) {
	if(out_error) {
		if(!message && custom_message) {
			message = custom_message;
		}
		if(!custom_message && message) {
			custom_message = message;
		}
		struct grug_error err = {
			.error_type = error_code,
			.message = message,
			.custom_message = custom_message,
			.file = file,
			.callstack = callstack,
			.arena = NULL,
		};
		*out_error = grug_copy_error(&err, arena_or_none);
	}
}

static void write_error_plain_basic(struct grug_error_code error_code, char const* message, char const* custom_message, struct grug_arena* arena, struct grug_error* out_error) {
	write_error_plain(error_code, message, custom_message, (struct grug_file_location){0}, (struct grug_callstack){0}, arena, out_error);
}

static void write_error(struct grug_state* gst, struct grug_error_code error_code, char const* message, char const* custom_message, struct grug_file_location file, struct grug_callstack callstack, struct grug_error* out_error) {
	write_error_plain(error_code, message, custom_message, file, callstack, NULL, out_error);
	if(gst) {
		write_error_plain(error_code, message, custom_message, file, callstack, gst->last_error.arena, &gst->last_error);
	}
}

static void write_error_basic(struct grug_state* gst, struct grug_error_code error_code, char const* message, char const* custom_message, struct grug_error* out_error) {
	write_error_plain(error_code, message, custom_message, (struct grug_file_location){0}, (struct grug_callstack){0}, NULL, out_error);
	if(gst) {
		write_error_plain(error_code, message, custom_message, (struct grug_file_location){0}, (struct grug_callstack){0}, gst->last_error.arena, &gst->last_error);
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
		write_error_basic(NULL, GRUG_ERROR_CODE_INIT, "Failed to create state: malloc() returned null", NULL, out_error);
		return NULL;
	}
	struct grug_arena* update_arena = grug_arena_new();
	if(!update_arena) {
		write_error_basic(NULL, GRUG_ERROR_CODE_INIT, "Failed to create state: grug_arena_new() returned null", NULL, out_error);
		GRUG_FREE(gst, sizeof(struct grug_state));
		return NULL;
	}
	char* mod_api_json_source = NULL;
	if(!settings.mod_api_json_source) {
		if(!settings.mod_api_json_path) {
			write_error_basic(NULL, GRUG_ERROR_CODE_INIT, "failed to create state: a mod_api.json is required", NULL, out_error);
			GRUG_FREE(gst, sizeof(struct grug_state));
			return NULL;
		}
		size_t file_len = 0;
		mod_api_json_source = read_all_contents(settings.mod_api_json_path, &file_len);
		if(!mod_api_json_source) {
			write_error_basic(NULL, GRUG_ERROR_CODE_INIT, "failed to create state: could not find the mod_api.json file", NULL, out_error);
			GRUG_FREE(gst, sizeof(struct grug_state));
			return NULL;
		}
	} else {
		size_t file_len = strlen(settings.mod_api_json_source);
		mod_api_json_source = GRUG_MALLOC(file_len + 1);
		if(!mod_api_json_source) {
			write_error_basic(NULL, GRUG_ERROR_CODE_INIT, "failed to create state: malloc() returned null", NULL, out_error);
			GRUG_FREE(gst, sizeof(struct grug_state));
			return NULL;
		}
		memcpy(mod_api_json_source, settings.mod_api_json_source, file_len);
		mod_api_json_source[file_len] = 0;
	}
	// Not sure why but GCC doesn't like allowing the initializer for the empty last error to be inside the initializer for the grug_state.
	struct grug_error last_error = {0};
	*gst = (struct grug_state) {
		.last_error = last_error,
		.update_arena = update_arena,
		.mod_api_json_source = mod_api_json_source,
		.logger = settings.logger,
		.backend = settings.backend,
		.fast_mode = false,
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
	// assert(false && "Not Implemented");
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
	gst->fast_mode = fast;
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

struct grug_error grug_copy_error(struct grug_error const* err, struct grug_arena* arena_or_none) {
	if(!err) {
		return (struct grug_error) {0};
	}
	if(err->callstack.num_entries) {
		assert(err->callstack.entries);
	}
	if(err->custom_message || err->message) {
		assert(err->custom_message);
		assert(err->message);
	}
	bool needs_allocation = err->message || err->custom_message || err->file.file_name || err->callstack.num_entries;
	struct grug_arena* arena = NULL;
	if(needs_allocation) {
		arena = arena_or_none;
		if(!arena) {
			arena = grug_arena_new();
			if(!arena) {
				/// If the creation of an arena failed, return some static memory with an error instead
				return (struct grug_error) {
					.error_type = GRUG_ERROR_CODE_INIT,
					.message = "Failed to create error: grug_arena_new() returned null",
					.custom_message = "Failed to create error: grug_arena_new() returned null",
					.file = {0},
					.callstack = {0},
					.arena = NULL,
				};
			}
		}
	}
	// make copies of things - grug_arena_copy_string is null safe and will propagate either a null arena or a null string
	char const* err_message = grug_arena_copy_string(arena, err->message);
	char const* err_custom_message = NULL;
	// No point re-copying the same string
	if(err->custom_message == err->message) {
		err_custom_message = err_message;
	} else {
		err_custom_message = grug_arena_copy_string(arena, err->custom_message);
	}
	char const* err_file_location_file_name = grug_arena_copy_string(arena, err->file.file_name);
	struct grug_callstack_entry* err_callstack_entries = (struct grug_callstack_entry*)grug_arena_copy(arena, (void*)err->callstack.entries, err->callstack.num_entries * sizeof(struct grug_callstack_entry));
	for(size_t entry_index = 0; entry_index < err->callstack.num_entries; entry_index += 1) {
		err_callstack_entries[entry_index].fn_name = grug_arena_copy_string(arena, err_callstack_entries[entry_index].fn_name);
	}
	return (struct grug_error) {
		.error_type = err->error_type,
		.message = err_message,
		.custom_message = err_custom_message,
		.file.file_name = err_file_location_file_name,
		.file.file = err->file.file,
		.file.offset = err->file.offset,
		.file.num_characters = err->file.num_characters,
		.callstack.entries = err_callstack_entries,
		.callstack.num_entries = err->callstack.num_entries,
		.arena = arena,
	};
}

void grug_assign_error(struct grug_error* err, struct grug_error const* new_err, struct grug_arena* arena_or_none) {
	if(err->arena) {
		*err = grug_copy_error(new_err, err->arena);
	} else {
		*err = grug_copy_error(new_err, arena_or_none);
	}
}

struct grug_arena* grug_arena_new(void) {
	struct beard_arena* arena = GRUG_MALLOC(sizeof(struct beard_arena));
	if(!arena) {
		return NULL;
	}

	beard_arena_init(arena, 0, 8192);

	return (struct grug_arena*)arena;
}

void* grug_arena_alloc(struct grug_arena* arena, size_t size) {
	if(arena) {
		return beard_arena_allocate((struct beard_arena*)arena, size);
	}
	return NULL;
}

char* grug_arena_copy(struct grug_arena* arena, char const* data, size_t size) {
	if(arena && data) {
		char* dst = grug_arena_alloc(arena, size);
		memcpy(dst, data, size);
		return dst;
	}
	return NULL;
}

char* grug_arena_copy_string(struct grug_arena* arena, char const* data) {
	if(arena && data) {
		size_t size = strlen(data) + 1;
		char* dst = grug_arena_alloc(arena, size);
		memcpy(dst, data, size);
		return dst;
	}
	return NULL;
}

void* grug_arena_alloc_aligned(struct grug_arena* arena, size_t size, size_t align) {
	if(arena) {
		return beard_arena_allocate_aligned((struct beard_arena*)arena, size, align);
	}
	return NULL;
}

void grug_arena_free(struct grug_arena* arena, void* ptr, size_t size) {
	if(arena) {
		beard_arena_free((struct beard_arena*)arena, ptr, size);
	}
}

void* grug_arena_realloc(struct grug_arena* arena, void* ptr, size_t old_size, size_t new_size) {
	if(arena) {
		return beard_arena_reallocate((struct beard_arena*)arena, ptr, old_size, new_size);
	}
	return NULL;
}

void grug_arena_clear(struct grug_arena* arena, size_t reserve_bytes) {
	if(arena) {
		beard_arena_reset((struct beard_arena*)arena, reserve_bytes);
	}
}

void grug_arena_deinit(struct grug_arena* arena) {
	if(arena) {
		beard_arena_deinit((struct beard_arena*)arena);
		GRUG_FREE(arena, sizeof(struct beard_arena));
	}
}

void grug_free_ast(struct grug_ast ast) {
	assert(false && "Not Implemented");
	(void)ast;
}

static inline void write_char_to_buffer(char* out_buffer, size_t capacity, size_t* inout_index, char character) {
	if(out_buffer) {
		if(*inout_index < capacity) {
			out_buffer[*inout_index] = character;
		}
	}
	*inout_index += 1;
}

static inline void write_stringl_to_buffer(char* out_buffer, size_t capacity, size_t* inout_index, char const* string, size_t string_len) {
	// TODO(bluesillybeard) this is extremely easy to optimize
	for(size_t index = 0; index < string_len; index += 1) {
		write_char_to_buffer(out_buffer, capacity, inout_index, string[index]);
	}
}

static inline void write_string_to_buffer(char* out_buffer, size_t capacity, size_t* inout_index, char const* string) {
	size_t string_len = strlen(string);
	write_stringl_to_buffer(out_buffer, capacity,  inout_index,  string, string_len);
}

size_t grug_tokens_to_grug(struct grug_token const* tokens, size_t num_tokens, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	// I don't quite remember why I made this return an error, but I see no reason to remove it.
	(void)o_error;
	if(out_string_buffer_capacity) {
		assert(out_string_buffer);
	}
	if(num_tokens) {
		assert(tokens);
	}
	size_t write_index = 0;

	for(size_t token_index = 0; token_index < num_tokens; token_index += 1) {
		struct grug_token tok = tokens[token_index];
		switch (tok.type) {
			case GRUG_TOKEN_TYPE_OPEN_PARENTHESIS: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '(');
				break;
			}
			case GRUG_TOKEN_TYPE_CLOSE_PARENTHESIS: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ')');
				break;
			}
			case GRUG_TOKEN_TYPE_OPEN_BRACE: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '{');
				break;
			}
			case GRUG_TOKEN_TYPE_CLOSE_BRACE: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '}');
				break;
			}
			case GRUG_TOKEN_TYPE_OPEN_BRACKET: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '[');
				break;
			}
			case GRUG_TOKEN_TYPE_CLOSE_BRACKET: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ']');
				break;
			}
			case GRUG_TOKEN_TYPE_PLUS: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '+');
				break;
			}
			case GRUG_TOKEN_TYPE_MINUS: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '-');
				break;
			}
			case GRUG_TOKEN_TYPE_STAR: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '*');
				break;
			}
			case GRUG_TOKEN_TYPE_FORWARD_SLASH: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '/');
				break;
			}
			case GRUG_TOKEN_TYPE_COMMA: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ',');
				break;
			}
			case GRUG_TOKEN_TYPE_COLON: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ':');
				break;
			}
			case GRUG_TOKEN_TYPE_DOT: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '.');
				break;
			}
			case GRUG_TOKEN_TYPE_NEW_LINE: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '\n');
				break;
			}
			case GRUG_TOKEN_TYPE_DOUBLE_EQUALS: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "==");
				break;
			}
			case GRUG_TOKEN_TYPE_NOT_EQUALS: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "!=");
				break;
			}
			case GRUG_TOKEN_TYPE_EQUAL: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '=');
				break;
			}
			case GRUG_TOKEN_TYPE_GREATER_EQUALS: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ">=");
				break;
			}
			case GRUG_TOKEN_TYPE_GREATER: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '>');
				break;
			}
			case GRUG_TOKEN_TYPE_LESS_EQUALS: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "<=");
				break;
			}
			case GRUG_TOKEN_TYPE_LESS: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, '<');
				break;
			}
			case GRUG_TOKEN_TYPE_AND: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "and");
				break;
			}
			case GRUG_TOKEN_TYPE_OR: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "or");
				break;
			}
			case GRUG_TOKEN_TYPE_NOT: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "not");
				break;
			}
			case GRUG_TOKEN_TYPE_TRUE: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "true");
				break;
			}
			case GRUG_TOKEN_TYPE_FALSE: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "false");
				break;
			}
			case GRUG_TOKEN_TYPE_IF: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "if");
				break;
			}
			case GRUG_TOKEN_TYPE_ELSE: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "else");
				break;
			}
			case GRUG_TOKEN_TYPE_WHILE: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "while");
				break;
			}
			case GRUG_TOKEN_TYPE_BREAK: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "break");
				break;
			}
			case GRUG_TOKEN_TYPE_RETURN: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "return");
				break;
			}
			case GRUG_TOKEN_TYPE_CONTINUE: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "continue");
				break;
			}
			case GRUG_TOKEN_TYPE_EXPORT: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "export");
				break;
			}
			case GRUG_TOKEN_TYPE_LOCAL: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "local");
				break;
			}
			case GRUG_TOKEN_TYPE_SPACE: {
				write_char_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, ' ');
				break;
			}
			case GRUG_TOKEN_TYPE_INDENT: {
				write_string_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, "    ");
				break;
			}
			case GRUG_TOKEN_TYPE_STRING: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			case GRUG_TOKEN_TYPE_ENTITY: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			case GRUG_TOKEN_TYPE_RESOURCE: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			case GRUG_TOKEN_TYPE_WORD: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			case GRUG_TOKEN_TYPE_NUMBER: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			case GRUG_TOKEN_TYPE_COMMENT: {
				write_stringl_to_buffer(out_string_buffer, out_string_buffer_capacity, &write_index, tok.contents, tok.contents_len);
				break;
			}
			default: {
				assert(false && "Invalid token type");
			}
		}
	}
	return write_index;
}

size_t grug_ast_to_grug(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	size_t num_tokens_required = grug_ast_to_tokens(ast, NULL, 0, o_error);
	if(o_error->error_type.tag[0]) {
		return 0;
	}
	struct grug_token* tokens = GRUG_MALLOC(num_tokens_required * sizeof(struct grug_token));
	if(!tokens) {
		struct grug_error err = {
			// TODO(bluesillybeard): add specific error codes for failed allocations
			.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
			.message = "Failed to convert AST to tokens: malloc() returned null",
		};
		grug_assign_error(o_error, &err, NULL);
		return 0;
	}
	size_t num_tokens = grug_ast_to_tokens(ast, tokens, num_tokens_required, o_error);
	assert(num_tokens == num_tokens_required);
	size_t chars_required = grug_tokens_to_grug(tokens, num_tokens, out_string_buffer, out_string_buffer_capacity, o_error);
	GRUG_FREE(tokens, num_tokens_required * sizeof(struct grug_token));
	return chars_required;
}

size_t grug_json_to_grug(char const* json, size_t json_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	struct grug_ast ast = grug_json_to_ast(json, json_len, NULL, o_error);
	if(o_error->error_type.tag[0]) {
		return 0;
	}
	size_t return_value = grug_ast_to_grug(ast, out_string_buffer, out_string_buffer_capacity, o_error);
	grug_free_ast(ast);
	return return_value;
}

// TODO(bluesillybeard): this was supposed to be better but perhaps this is worse than the else-if chain or w/e else
struct grug_token_info {
	grug_token_type type;
	// May be NULL
	char const* expected;
	// If this is a string, this is the character that indicates the end of the string
	char const* string_end;
	// If set, expected is instead treated as the start of a string (", r", e", etc)
	bool is_string;
	// whether this token requires proceeding non-ascii or whitespace character
	bool proceed_with_special;
	// whether this token expects to only show up at the start of a new line
	bool expected_at_line_start;
	
};

/// These are in the order they are searched for
/// If you can figure out some way to make this not suck but still pass GCCs -pedantic, be my guest.
/// However it will likely be rewritten anyways.
static struct grug_token_info token_infos[GRUG_TOKEN_TYPE_NUM] = {0}; //NOLINT: can't be const because C
static bool token_infos_initialized = false;//NOLINT: can't be const because C

static void init_token_infos(void) {
	if(!token_infos_initialized) {
		token_infos_initialized = true;
		struct grug_token_info token_infos_local[GRUG_TOKEN_TYPE_NUM] = {
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_OPEN_PARENTHESIS, .expected = "(", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_CLOSE_PARENTHESIS, .expected = ")", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_OPEN_BRACE, .expected = "{", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_CLOSE_BRACE, .expected = "}", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_OPEN_BRACKET, .expected = "[", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_CLOSE_BRACKET, .expected = "]", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_PLUS, .expected = "+", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_MINUS, .expected = "-", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_STAR, .expected = "*", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_FORWARD_SLASH, .expected = "/", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_COMMA, .expected = ",", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_COLON, .expected = ":", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_DOT, .expected = ".", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_NEW_LINE, .expected = NULL, .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_DOUBLE_EQUALS, .expected = "==", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_NOT_EQUALS, .expected = "!=", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_EQUAL, .expected = "=", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_GREATER_EQUALS, .expected = ">=", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_GREATER, .expected = ">", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_LESS_EQUALS, .expected = "<=", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_LESS, .expected = "<", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_AND, .expected = "and", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_OR, .expected = "or", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_NOT, .expected = "not", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_TRUE, .expected = "true", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_FALSE, .expected = "false", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_IF, .expected = "if", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_ELSE, .expected = "else", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_WHILE, .expected = "while", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_BREAK, .expected = "break", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_RETURN, .expected = "return", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_CONTINUE, .expected = "continue", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_EXPORT, .expected = "export", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = true},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_LOCAL, .expected = "local", .string_end = NULL, .is_string = false, .proceed_with_special = true, .expected_at_line_start = true},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_INDENT, .expected = "    ", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = true},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_SPACE, .expected = " ", .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_STRING, .expected = "\"", .string_end = "\"", .is_string = true, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_ENTITY, .expected = "e\"", .string_end = "\"", .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_RESOURCE, .expected = "r\"", .string_end = "\"", .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_COMMENT, .expected = "#", .string_end = "\n", .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_WORD, .expected = NULL, .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_NUMBER, .expected = NULL, .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
			(struct grug_token_info) {.type = GRUG_TOKEN_TYPE_NONE, .expected = NULL, .string_end = NULL, .is_string = false, .proceed_with_special = false, .expected_at_line_start = false},
		};
		memcpy(token_infos, token_infos_local, sizeof(token_infos));
	}
}

static size_t check_token(char const* grug_src, size_t grug_len, uint32_t token_index, bool line_start, struct grug_error* o_error) {
	struct grug_token_info info = token_infos[token_index];
	if(info.expected) {
		size_t expected_len = strlen(info.expected);
		if(grug_len < expected_len) {
			return 0;
		}
		if(info.is_string) {
			size_t expected_end_len = strlen(info.string_end);
			size_t char_index = expected_len;
			size_t max_iterations = 100000000;
			while(max_iterations--) {
				if(strncmp(grug_src+char_index, info.string_end, expected_len) == 0) {
					return char_index+1;
				}
				if(grug_src[char_index] == '\n') {
					struct grug_error err = {
						.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
						// Needs to be brought in line with what the test suite expects
						.message = "Expected end quote but found end of line",
					};
					grug_assign_error(o_error, &err, NULL);
					return 0;
				}
				char_index += 1;
				if(char_index+expected_end_len >= grug_len) {
					struct grug_error err = {
						.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
						// Needs to be brought in line with what the test suite expects
						.message = "Expected end quote but found end of file",
					};
					grug_assign_error(o_error, &err, NULL);
					return 0;
				}
			}
			assert(false);
		}
		if(strncmp(grug_src, info.expected, expected_len) == 0) {
			if(info.expected_at_line_start && !line_start) {
				struct grug_error err = {
					.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
					// Needs to be brought in line with what the test suite expects
					.message = "Expected token to only appear on a new line",
				};
				grug_assign_error(o_error, &err, NULL);
				return 0;
			}
			if(info.proceed_with_special) {
				if(grug_len == expected_len) {
					struct grug_error err = {
						.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
						// Needs to be brought in line with what the test suite expects
						.message = "Expected token to not appear on the end of a file",
					};
					grug_assign_error(o_error, &err, NULL);
					return 0;
				}
				assert(grug_len > expected_len);
				if(isascii(grug_src[expected_len+1]) || isalnum(grug_src[expected_len+1])) {
					// means this is probably one of the special cases
					return 0;
				}
			}
			return expected_len;
		}
		return 0;
	}
	return 0;
}

static struct grug_token pull_token(char const* grug_src, size_t grug_len, bool line_start, struct grug_error* o_error) {
	for(uint32_t check_me = 0; check_me < GRUG_TOKEN_TYPE_NUM; check_me += 1) {
		size_t token_size = check_token(grug_src, grug_len, check_me, line_start, o_error);
		if(o_error->error_type.tag[0]) {
			return (struct grug_token) {0};
		}
		if(token_size) {
			return (struct grug_token) {
				.contents = grug_src,
				.contents_len = token_size,
				.type = token_infos[check_me].type,
			};
		}
	}
	// None of them matched, time to handle the special-cases:
	// GRUG_TOKEN_TYPE_WORD
	// GRUG_TOKEN_TYPE_NUMBER
	if(isascii(grug_src[0])) {
		size_t max_iterations = 100000000;
		size_t read_index = 0;
		while(max_iterations--) {
			if(read_index > grug_len) {
				struct grug_error err = {
					.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
					// Needs to be brought in line with what the test suite expects
					.message = "Im not really sure what to write here yet",
				};
				grug_assign_error(o_error, &err, NULL);
				return (struct grug_token) {0};
			}
			if(!isalnum(grug_src[read_index])) {
				return (struct grug_token) {
					.contents = grug_src,
					.contents_len = read_index,
					.type = GRUG_TOKEN_TYPE_WORD,
				};
			}
			read_index += 1;
		}
		assert(false);
	} else if (isalnum(grug_src[0])) {
		size_t max_iterations = 100000000;
		size_t read_index = 0;
		while(max_iterations--) {
			if(read_index > grug_len) {
				struct grug_error err = {
					.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
					// Needs to be brought in line with what the test suite expects
					.message = "Im not really sure what to write here yet",
				};
				grug_assign_error(o_error, &err, NULL);
				return (struct grug_token) {0};
			}
			if(!isalnum(grug_src[read_index]) || isascii(grug_src[read_index])) {
				return (struct grug_token) {
					.contents = grug_src,
					.contents_len = read_index,
					.type = GRUG_TOKEN_TYPE_WORD,
				};
			}
			read_index += 1;
		}
		assert(false);
	} else {
		struct grug_error err = {
			.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
			// Needs to be brought in line with what the test suite expects
			.message = "Im not really sure what to write here yet",
		};
		grug_assign_error(o_error, &err, NULL);
		return (struct grug_token) {0};
	}

}

size_t grug_grug_to_tokens(char const* grug, size_t grug_len, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error) {
	init_token_infos();
	size_t read_index = 0;
	size_t token_index = 0;
	bool new_line = true;
	while(true) {
		assert(read_index <= grug_len);
		struct grug_token tok = pull_token(grug+read_index, grug_len-read_index, new_line, o_error);
		if(o_error->error_type.tag[0]) {
			return 0;
		}
		if(tok.type == GRUG_TOKEN_TYPE_NONE) {
			return token_index;
		}
		if(out_tokens && out_tokens_capacity >= token_index) {
			out_tokens[token_index] = tok;
		}
		token_index += 1;
		new_line = tok.type == GRUG_TOKEN_TYPE_NEW_LINE;
		
	}
}

size_t grug_ast_to_tokens(struct grug_ast ast, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)ast;
	(void)out_tokens;
	(void)out_tokens_capacity;
	(void)o_error;
	return 0;
}

size_t grug_json_to_tokens(char const* json, size_t json_len, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error) {
	struct grug_ast ast = grug_json_to_ast(json, json_len, NULL, o_error);
	if(o_error->error_type.tag[0]) {
		return 0;
	}
	size_t return_value = grug_ast_to_tokens(ast, out_tokens, out_tokens_capacity, o_error);
	grug_free_ast(ast);
	return return_value;
}

struct grug_ast grug_grug_to_ast(char const* grug, size_t grug_len, struct grug_arena* arena_or_none, struct grug_error* o_error) {
	size_t num_tokens_required = grug_grug_to_tokens(grug, grug_len, NULL, 0, o_error);
	if(o_error->error_type.tag[0]) {
		return (struct grug_ast){0};
	}
	struct grug_token* tokens = GRUG_MALLOC(num_tokens_required * sizeof(struct grug_token));
	if(!tokens) {
		struct grug_error err = {
			// TODO(bluesillybeard): add specific error codes for failed allocations
			.error_type = GRUG_ERROR_CODE_COMPILE_TOKENIZER,
			.message = "Failed to convert grug to tokens: malloc() returned null",
		};
		grug_assign_error(o_error, &err, NULL);
		return (struct grug_ast){0};
	}
	size_t num_tokens = grug_grug_to_tokens(grug, grug_len, tokens, num_tokens_required, o_error);
	if(o_error->error_type.tag[0]) {
		GRUG_FREE(tokens, num_tokens_required * sizeof(struct grug_token));
		return (struct grug_ast){0};
	}
	assert(num_tokens_required == num_tokens);

	struct grug_ast ast = grug_tokens_to_ast(tokens, num_tokens, arena_or_none, o_error);
	// ast keeps local copies of everything so we can free the tokens immediately
	GRUG_FREE(tokens, num_tokens_required * sizeof(struct grug_token));
	return ast;
}

struct grug_ast grug_tokens_to_ast(struct grug_token const* tokens, size_t num_tokens, struct grug_arena* arena_or_none, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)tokens;
	(void)num_tokens;
	(void)arena_or_none;
	(void)o_error;
	return (struct grug_ast){0};
}

struct grug_ast grug_json_to_ast(char const* json, size_t json_len, struct grug_arena* arena_or_none, struct grug_error* o_error) {
	assert(false && "Not Implemented");
	(void)json;
	(void)json_len;
	(void)arena_or_none;
	(void)o_error;
	return (struct grug_ast){0};
}

size_t grug_grug_to_json(char const* grug, size_t grug_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	struct grug_ast ast = grug_grug_to_ast(grug, grug_len, NULL, o_error);
	if(o_error->error_type.tag[0]) {
		return 0;
	}
	size_t return_value = grug_ast_to_json(ast, out_string_buffer, out_string_buffer_capacity, o_error);
	grug_free_ast(ast);
	return return_value;
	
}

size_t grug_tokens_to_json(struct grug_token const* tokens, size_t num_tokens, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) {
	struct grug_ast ast = grug_tokens_to_ast(tokens, num_tokens, NULL, o_error);
	if(o_error->error_type.tag[0]) {
		return 0;
	}
	size_t return_value = grug_ast_to_json(ast, out_string_buffer, out_string_buffer_capacity, o_error);
	grug_free_ast(ast);
	return return_value;
}

size_t grug_ast_to_json(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error) { // NOLINT: what part of casting to void do you not understand clang-tidy?
	assert(false && "Not Implemented");
	(void)ast;
	(void)out_string_buffer;
	(void)out_string_buffer_capacity;
	(void)o_error;
	return 0;
}
