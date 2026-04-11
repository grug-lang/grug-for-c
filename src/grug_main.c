#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "grug_main.h"
#include "grug_arena.h"
#include "grug_options.h"

// SECTION MARK: mini lib

// TODO: add a way to have a user-defined realloc, and fall back to this otherwise
void* grug_realloc(void* ptr, size_t old_len, size_t new_len) {
	if(!ptr) {

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

	struct block* first;
	struct block* last;
	size_t total_size = 0;

	FILE* f = fopen(file_path, "rb");

	if(!f) {
		if(out_len) {
			*out_len = 0;
		}
		return NULL;
	}

	first = malloc(sizeof(struct block));

	first->data_len = fread(first->data, 1, 1024, f);
	total_size += first->data_len;

	last = first;

	while(!feof(f)) {
		struct block* new = malloc(sizeof(struct block));
		last->pnext = new;
		last = new;
		last->data_len = fread(last->data, 1, 1024, f);
		total_size += last->data_len;
	}

	fclose(f);

	char* data = malloc(total_size + 1);
	size_t data_written = 0;
	while(first) {
		memcpy(data + data_written, first->data, first->data_len);
		data_written += first->data_len;
		struct block* next = first->pnext;
		free(first);
		first = next;
	}
	if(out_len) {
		*out_len = total_size;
	}
	return data;
}

// SECTION MARK: function implementations

struct grug_init_settings grug_default_settings(void) {
	return (struct grug_init_settings){ 
		// TODO: interpreter backend
		.mod_api_path = "",
		.mods_dir_path = "",
		.runtime_error_handler = (struct grug_runtime_error_handler){0},
		.backend = (struct grug_backend){.obj = 0, .vtable = 0},
	};
}

struct grug_state* grug_init(struct grug_init_settings settings) {
	(void) settings;
	// TODO: implement
	return 0;
}

void grug_swap_backend(struct grug_state* gst, struct grug_backend backend) {
	(void)gst;
	(void)backend;
	// TODO: implement
}

void grug_set_fast_mode(struct grug_state* gst, bool fast) {
	(void)gst;
	(void)fast;
	// TODO: implement
}
bool grug_register_game_fn(struct grug_state* gst, char const* game_fn_name, game_fn fn) {
	(void)gst;
	(void)game_fn_name;
	(void)fn;
	// TODO: implement
	return false;
}

struct grug_on_fns grug_get_fn_ids(struct grug_state* gst) {
	(void)gst;
	// TODO: implement
	return (struct grug_on_fns){
		.count = 0,
		.entries = 0,
	};
}

grug_file_id grug_compile_file(struct grug_state* gst, const char* path) {
	size_t file_len;
	char* file = read_all_contents(path, &file_len);
	grug_file_id result = grug_compile_file_from_str(gst, path, file);
	free(file);
	return result;
}

grug_file_id grug_compile_file_from_str(struct grug_state* gst, const char* path, char* file_text) {
	// TODO: implement
	(void)gst;
	(void)path;
	(void)file_text;
	return 0;
}

const struct grug_mod_dir* grug_get_mods(struct grug_state* gst) {
	(void)gst;
	// TODO: implement
	return 0;
}

grug_id grug_create_entity(struct grug_state* gst, grug_file_id script, grug_object_id me_id) {
	(void)gst;
	(void)script;
	(void)me_id;
	// TODO: implement
	return 0;
}

grug_file_id grug_entity_get_file(struct grug_state* gst, grug_id entity) {
	(void)gst;
	(void)entity;
	// TODO: implement
	return 0;
}

struct grug_entity* grug_entity_get_data(struct grug_state* gst, grug_id entity) {
	(void)gst;
	(void)entity;
	// TODO: implement
	return 0;
}

void grug_deinit_entity(struct grug_state* gst, grug_id entity) {
	(void)gst;
	(void)entity;
	// TODO: implement
}

struct grug_updates_list grug_update(struct grug_state* gst) {
	(void)gst;
	// TODO: implement
	return (struct grug_updates_list) {
		.count = 0,
		.updates = 0,
	};
}

void grug_deinit(struct grug_state* gst) {
	(void)gst;
	// TODO: implement
}

bool grug_all_game_functions_registered(struct grug_state* gst) {
	(void) gst;
	// TODO: implement
	return false;
}

bool grug_call_on_function(struct grug_state* gst, grug_id entity, grug_on_fn_id on_fn_id, union grug_value* args, size_t args_len) {
	(void)gst;
	(void)on_fn_id;
	(void)entity;
	(void)args;
	(void)args_len;
	// TODO: implement
	return false;
}

bool grug_call_on_function_raw(struct grug_state* gst, grug_id entity, grug_on_fn_id on_fn_id, union grug_value* args) {
	(void)gst;
	(void)on_fn_id;
	(void)entity;
	(void)args;
	// TODO: implement
	return false;
}

void grug_backend_call(struct grug_state* gst, grug_on_fn_id fn, grug_id entity, const union grug_value args[]) {
	(void)gst;
	(void)fn;
	(void)entity;
	(void)args;
	// TODO: implement
}

struct grug_string grug_alloc_string(size_t len) {
	if(len == 0) {
		return (struct grug_string){
			.ptr = 0,
			.len = 0,
		};
	}
	return (struct grug_string) {
		.ptr = GRUG_MALLOC(len + 1),
		.len = len,
	};
}

struct grug_string grug_copy_string(struct grug_string src) {
	struct grug_string new_str = grug_alloc_string(src.len);
	memcpy(new_str.ptr, src.ptr, src.len);
	new_str.ptr[src.len] = 0;
	return new_str;
}

void grug_free_string(struct grug_string str) {
	if(str.len > 0) {
		GRUG_FREE(str.ptr, str.len + 1);
	}
}

struct grug_error grug_copy_error(struct grug_error src) {
	struct grug_string message = grug_copy_string(src.message);
	struct grug_string custom_message;
	if(src.message.ptr == src.custom_message.ptr) {
		custom_message = message;
	} else {
		custom_message = grug_copy_string(src.custom_message);
	}

	struct grug_file_location file;
	switch (src.error_type) {
		case GRUG_ERROR_TYPE_NONE:
		case GRUG_ERROR_TYPE_INIT: {
			file = (struct grug_file_location){0};
			break;
		}
		case GRUG_ERROR_TYPE_COMPILE:
		case GRUG_ERROR_TYPE_RUNTIME_STACK_OVERFLOW:
		case GRUG_ERROR_TYPE_RUNTIME_TIME_LIMIT_EXCEEDED:
		case GRUG_ERROR_TYPE_RUNTIME_GAME_FN_ERROR: {
			file = (struct grug_file_location){
				.file = src.file.file,
				.file_name = grug_copy_string(src.file.file_name),
				.offset = src.file.offset,
				.num_characters = src.file.num_characters,
			};
			break;
		}
		default: {
			assert(false);
		}
	}


	return (struct grug_error) {
		.error_type = src.error_type,
		.message = message,
		.custom_message = custom_message,
		.file = file,
	};
}

void grug_free_error(struct grug_error src){
	grug_free_string(src.message);
	if(src.message.ptr != src.custom_message.ptr) {
		grug_free_string(src.custom_message);
	}

	switch (src.error_type) {
		case GRUG_ERROR_TYPE_NONE:
		case GRUG_ERROR_TYPE_INIT: {
			// nothing to free
		}
		case GRUG_ERROR_TYPE_COMPILE:
		case GRUG_ERROR_TYPE_RUNTIME_STACK_OVERFLOW:
		case GRUG_ERROR_TYPE_RUNTIME_TIME_LIMIT_EXCEEDED:
		case GRUG_ERROR_TYPE_RUNTIME_GAME_FN_ERROR: {
			grug_free_string(src.file.file_name);
		}
	}
}

void grug_free_tokens(struct grug_tokens tokens) {
	for(size_t token_index = 0; token_index < tokens.tokens_len; token_index += 1) {
		grug_free_string(tokens.tokens[token_index].contents);
	}
	GRUG_FREE(tokens.tokens, tokens.tokens_len * sizeof(struct grug_token));
}

void grug_free_ast(struct grug_ast ast)  {
	grug_arena_deinit(&ast.arena);
}

static void add_token(struct grug_tokens* tokens, size_t* capacity, struct grug_token token) {
	if(*capacity == tokens->tokens_len) {
		size_t new_capacity = *capacity * 3 / 2 + 1;
		struct grug_token* new_tokens = grug_realloc(tokens->tokens, *capacity, new_capacity);
		assert(new_tokens);
		*capacity = new_capacity;
		tokens->tokens = new_tokens;
	}
	tokens->tokens[tokens->tokens_len] = token;
	tokens->tokens_len += 1;
}

static struct grug_error grug_alloc_format_error(grug_error_type error, char const* fmt, ...) {
	va_list v;
	va_start(v, fmt);
	int error_message_len = vsnprintf(NULL, 0, fmt, v) + 1;
	va_end(v);
	va_start(v, fmt);
	char* error_msg;
	if(error_message_len >= 0) {
		error_msg = alloca((unsigned int)error_message_len);
		snprintf(error_msg, (size_t)error_message_len, fmt, v);
	} else {
		error_msg = "Failed to format message";
		error_message_len = (int)strlen(error_msg);
	}
	va_end(v);
	return grug_copy_error((struct grug_error) {
		.error_type = error,
		.message = (struct grug_string){.ptr = error_msg, .len = (size_t)error_message_len},

	});
}

struct grug_tokens grug_to_tokens(struct grug_string grug, struct grug_error* o_error) {
	(void)o_error;
	struct grug_tokens tokens = {
		.tokens = 0,
		.tokens_len = 0,
	};
	size_t tokens_capacity;
	char* src = grug.ptr;
	size_t src_len = grug.len;
	size_t i = 0;
	size_t line_number = 1;
	// TODO: clean up this horrible copy-paste of the grug-for-python code unceremoniously translated line-for-line without splitting things into functions.
	while(i < grug.len) {
		char c = src[i];
		if(c == '(') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_OPEN_PARENTHESIS, {.len = 0}});
			i += 1;
		}
		else if(c == ')') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_CLOSE_PARENTHESIS, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '{') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_OPEN_BRACE, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '}') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_CLOSE_BRACE, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '+') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_PLUS, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '-') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_MINUS, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '*'){
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_MULTIPLICATION, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '/') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_DIVISION, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == ',') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_COMMA, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == ':') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_COLON, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '\n') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_NEWLINE, .contents = {.ptr = &src[i], .len = 1}});
			line_number += 1;
			i += 1;
		}
		else if(c == '=' && i + 1 < src_len && src[i + 1] == '=') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_EQUALS, .contents = GRUG_WRAP_STRING("==")});
			i += 2;
		}
		else if(c == '!' && i + 1 < src_len && src[i + 1] == '=') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_NOT_EQUALS, .contents = GRUG_WRAP_STRING("!=")});
			i += 2;
		}
		else if(c == '=') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_ASSIGNMENT, .contents = {.ptr = &src[i], .len = 1}});
			i += 1;
		}
		else if(c == '>' && i + 1 < src_len && src[i + 1] == '=') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_GREATER_OR_EQUAL, .contents = GRUG_WRAP_STRING(">=")});
			i += 2;
		}
		else if(c == '>') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_GREATER, .contents = GRUG_WRAP_STRING(">")});
			i += 1;
		}
		else if(c == '<' && i + 1 < src_len && src[i + 1] == '=') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_LESS_OR_EQUAL, .contents = GRUG_WRAP_STRING("<=")});
			i += 2;
		}
		else if(c == '<') {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_LESS, .contents = GRUG_WRAP_STRING("<")});
			i += 1;
		}
		else if((i+sizeof("and") < src_len && memcmp(&src[i], "and", sizeof("and")) == 0) && ((i + 3) >= src_len || (!(src[i + 3] >= '0' && src[i + 3] <= '9') || src[i + 3] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_AND, .contents = GRUG_WRAP_STRING("and")});
			i += 3;
		}
		else if((i+sizeof("or") < src_len && memcmp(&src[i], "or", sizeof("or")) == 0) && ((i + 2) >= src_len || (!(src[i + 2] >= '0' && src[i + 2] <= '9') || src[i + 2] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_OR, .contents = GRUG_WRAP_STRING("or")});
			i += 2;
		}
		else if((i+sizeof("not") < src_len && memcmp(&src[i], "not", sizeof("not")) == 0) && ((i + 3) >= src_len || (!(src[i + 3] >= '0' && src[i + 3] <= '9') || src[i + 3] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_NOT, .contents = GRUG_WRAP_STRING("not")});
			i += 3;
		}
		else if((i+sizeof("true") < src_len && memcmp(&src[i], "true", sizeof("true")) == 0) && ((i + 4) >= src_len || (!(src[i + 4] >= '0' && src[i + 4] <= '9') || src[i + 4] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_TRUE, .contents = GRUG_WRAP_STRING("true")});
			i += 4;
		}
		else if((i+sizeof("false") < src_len && memcmp(&src[i], "false", sizeof("false")) == 0) && ((i + 5) >= src_len || (!(src[i + 5] >= '0' && src[i + 5] <= '9') || src[i + 5] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_FALSE, .contents = GRUG_WRAP_STRING("false")});
			i += 5;
		}
		else if((i+sizeof("if") < src_len && memcmp(&src[i], "if", sizeof("if")) == 0) && ((i + 2) >= src_len || (!(src[i + 2] >= '0' && src[i + 2] <= '9') || src[i + 2] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_IF, .contents = GRUG_WRAP_STRING("if")});
			i += 2;
		}
		else if((i+sizeof("else") < src_len && memcmp(&src[i], "else", sizeof("else")) == 0) && ((i + 4) >= src_len || (!(src[i + 4] >= '0' && src[i + 4] <= '9') || src[i + 4] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_ELSE, .contents = GRUG_WRAP_STRING("else")});
			i += 4;
		}
		else if((i+sizeof("while") < src_len && memcmp(&src[i], "while", sizeof("while")) == 0) && ((i + 5) >= src_len || (!(src[i + 5] >= '0' && src[i + 5] <= '9') || src[i + 5] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_WHILE, .contents = GRUG_WRAP_STRING("while")});
			i += 5;
		}
		else if((i+sizeof("break") < src_len && memcmp(&src[i], "break", sizeof("break")) == 0) && ((i + 5) >= src_len || (!(src[i + 5] >= '0' && src[i + 5] <= '9') || src[i + 5] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_BREAK, .contents = GRUG_WRAP_STRING("break")});
			i += 5;
		}
		else if((i+sizeof("return") < src_len && memcmp(&src[i], "return", sizeof("return")) == 0) && ((i + 6) >= src_len || (!(src[i + 6] >= '0' && src[i + 6] <= '9') || src[i + 6] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_RETURN, .contents = GRUG_WRAP_STRING("return")});
			i += 6;
		}
		else if((i+sizeof("continue") < src_len && memcmp(&src[i], "continue", sizeof("continue")) == 0) && ((i + 8) >= src_len || (!(src[i + 8] >= '0' && src[i + 8] <= '9') || src[i + 8] == '_'))) {
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_CONTINUE, .contents = GRUG_WRAP_STRING("continue")});
			i += 8;
		} else if (c == ' ') {
			if(i + 1 >= src_len || src[i + 1] != ' ') {
				add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_SPACE, .contents = GRUG_WRAP_STRING(" ")});
				i += 1;
				continue;
			}
			size_t old_i = i;
			while(i < src_len && src[i] == ' '){
				i += 1;
			}

			size_t spaces = i - old_i;

			if(spaces % GRUG_SPACES_PER_INDENT != 0){
				// TODO: what to do when there are more than 2gib of spaces?
				if(spaces > INT_MAX) {
					spaces = INT_MAX;
				}
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Encountered %d spaces, while indentation expects multiples of %d spaces, on line %d", (int)spaces, GRUG_SPACES_PER_INDENT, line_number);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}
			// TODO: make the number of spaces actually match the spaces per indent
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_INDENTATION, .contents = GRUG_WRAP_STRING("    ")});
		} else if(c == '*') {
			i += 1;
			size_t start = i;
			while(i < src_len && src[i] != '"') {
				if(src[i] == '\0'){
					*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Unexpected null byte on line %d", line_number);
					// Free whatever we had so far
					GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
					return (struct grug_tokens){0};
				} else if(src[i] == '\\' && i + 1 < src_len && (src[i + 1] == '\r' || src[i + 1] == '\n')) {
					*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Unexpected line break in string on line %d", line_number);
					// Free whatever we had so far
					GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
					return (struct grug_tokens){0};
				}
				i += 1;
			}
			if(i >= src_len) {
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Unclosed \" on line %d", line_number);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_STRING, .contents = (struct grug_string){.ptr = &src[start], .len = (i - start)}});
			i += 1;
		} else if(isalpha(c) || c == '_') {
			size_t start = i;
			while(i < src_len && (isalnum(src[i]) || src[i] == '_')) {
				i += 1;
			}
			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_WORD, .contents = (struct grug_string){.ptr = &src[start], .len = i - start}});
		} else if(isdigit(c)) {
			size_t start = i;
			bool seen_period = false;
			i += 1;
			while(i < src_len && (isdigit(src[i]) || src[i] == '.')) {
				if(src[i] == '.') {
					if(seen_period) {
						*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Encountered two '.' periods in a number on line %d", line_number);
						// Free whatever we had so far
						GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
						return (struct grug_tokens){0};
					}
					seen_period = true;
				}
				i += 1;
			}
			if(src[i - 1] == '.') {
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Missing digit after decimal point in %*.s", i - start, &src[start]);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}

			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_NUMBER, .contents = (struct grug_string){.ptr = &src[start], .len = i - start}});
		} else if(c == '#') {
			i += 1;
			if(i >= src_len || src[i] != ' ') {
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Expected a single space after the '#' on line %d", line_number);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}
			i += 1;
			size_t start = i;
			while(i < src_len && src[i] != '\r' && src[i] != '\n'){
				if(src[i] == '\0') {
					*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Unexpected null byte on line %d", line_number);
					// Free whatever we had so far
					GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
					return (struct grug_tokens){0};
				}
				i += 1;
			}

			size_t comment_len = i - start;
			if(comment_len == 0) {
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Expected the comment to contain some text on line %d", line_number);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}

			if(isspace(src[i - 1])) {
				*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "A comment has trailing whitespace on line %d", line_number);
				// Free whatever we had so far
				GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
				return (struct grug_tokens){0};
			}

			add_token(&tokens, &tokens_capacity, (struct grug_token){.type = GRUG_TOKEN_COMMENT, .contents = (struct grug_string){.ptr = &src[start], .len = i - start}});
		} else {
			*o_error = grug_alloc_format_error(GRUG_ERROR_TYPE_COMPILE, "Unrecognized character '%c' on line %d", c, line_number);
			// Free whatever we had so far
			GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
			return (struct grug_tokens){0};
		}
	}

	// Copy all of the tokens
	struct grug_tokens result = (struct grug_tokens) {
		.tokens = GRUG_MALLOC(tokens.tokens_len * sizeof(*tokens.tokens)),
		.tokens_len = tokens.tokens_len,
	};
	for(size_t i = 0; i < tokens.tokens_len; i += 1) {
		result.tokens[i] = (struct grug_token) {
			.type = tokens.tokens[i].type,
			// grug_copy_string does not actually check the null terminator, but it will always include one in the copy.
			// ALL of the strings from the tokenization are either statically allocated, or they are a window into the original format string.
			.contents = grug_copy_string(tokens.tokens[i].contents),
		};
	}
	GRUG_FREE(tokens.tokens, tokens_capacity * sizeof(*tokens.tokens));
	return result;
}

struct grug_ast tokens_to_ast(struct grug_tokens tokens, struct grug_error* o_error) {
	(void)tokens;
	(void)o_error;
	// TODO
	return (struct grug_ast){.helper_functions_count = 0, .on_functions_count = 0, .members_count = 0};
}

struct grug_tokens ast_to_tokens(struct grug_ast ast, struct grug_error* o_error) {
	(void)ast;
	(void)o_error;
	// TODO
	return (struct grug_tokens){.tokens_len = 0};
}

struct grug_string tokens_to_grug(struct grug_tokens tokens, struct grug_error* o_error) {
	(void)tokens;
	(void)o_error;
	// TODO
	return (struct grug_string){.len = 0};
}

struct grug_ast json_to_ast(struct grug_string json, struct grug_error* o_error) {
	(void)json;
	(void)o_error;
	// TODO
	return (struct grug_ast){.helper_functions_count = 0, .on_functions_count = 0, .members_count = 0};
}

struct grug_string ast_to_json(struct grug_ast ast, struct grug_error* o_error) {
	(void)ast;
	(void)o_error;
	// TODO
	return (struct grug_string){.len = 0};
}

struct grug_ast grug_to_ast(struct grug_string grug, struct grug_error* o_error) {
	(void)grug;
	(void)o_error;
	// TODO
	return (struct grug_ast){.helper_functions_count = 0, .on_functions_count = 0, .members_count = 0};
}

struct grug_string ast_to_grug(struct grug_ast ast, struct grug_error* o_error) {
	(void)ast;
	(void)o_error;
	// TODO
	return (struct grug_string){.len = 0};
}

struct grug_string grug_to_json(struct grug_string grug, struct grug_error* o_error) {
	(void)grug;
	(void)o_error;
	// TODO
	return (struct grug_string){.len = 0};
}

struct grug_string json_to_grug(struct grug_string json, struct grug_error* o_error) { 
	(void)json;
	(void)o_error;
	// TODO
	return (struct grug_string){.len = 0};
}
