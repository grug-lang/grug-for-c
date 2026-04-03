#include <assert.h>
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

// SECTION MARK: function implementations

struct grug_init_settings grug_default_settings(void) {
	return (struct grug_init_settings){ 
		// TODO: interpreter backend
		.mod_api_path = (struct grug_string){.len = 0},
		.mods_dir_path = (struct grug_string){.len = 0},
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

const struct grug_mod_dir* grug_get_mods(struct grug_state* gst) {
	(void)gst;
	// TODO: implement
	return 0;
}

grug_id grug_create_entity(struct grug_state* gst, grug_file_id script) {
	(void)gst;
	(void)script;
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

void grug_free_string(struct grug_string str) {
	if(str.len > 0) {
		GRUG_FREE(str.ptr, str.len + 1);
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

struct grug_tokens grug_to_tokens(struct grug_string grug, struct grug_error* o_error) {
	(void)o_error;

	do {
		char c = grug.ptr[0];
		struct grug_token token = {0};
		if(c == '(') {
			token.type = GRUG_TOKEN_OPEN_PARENTHESIS;
		} else if(c == ')') {
			token.type = GRUG_TOKEN_CLOSE_PARENTHESIS;
		} else if(c == '{') {
			token.type = GRUG_TOKEN_OPEN_BRACE;
		} else if(c == '}') {
			token.type = GRUG_TOKEN_CLOSE_BRACE;
		} else if(c == '+') {
			token.type = GRUG_TOKEN_PLUS;
		} else if(c == '-') {
			token.type = GRUG_TOKEN_MINUS;
		} else if(c == '*') {
			token.type = GRUG_TOKEN_MULTIPLICATION;
		} else if(c == '/') {
			token.type = GRUG_TOKEN_DIVISION;
		} else if(c == ',') {
			token.type = GRUG_TOKEN_COMMA;
		} else if(c == '\n') {
			// TODO: handle /r/n (windows style) and /r (mac style) newlines
			token.type = GRUG_TOKEN_NEWLINE;
		} else if(c == '=') {
			token.type = GRUG_TOKEN_EQUALS;
		} else if(c == '!' && grug.ptr[1] == '=') {
			token.type = GRUG_TOKEN_NOT_EQUALS;
		}
		else {
			assert(false);
		}
	} while(grug.len > 0);
	// GRUG_TOKEN_NOT_EQUALS
	// GRUG_TOKEN_ASSIGNMENT
	// GRUG_TOKEN_GREATER_OR_EQUAL
    // GRUG_TOKEN_GREATER
    // GRUG_TOKEN_LESS_OR_EQUAL
    // GRUG_TOKEN_LESS
    // GRUG_TOKEN_AND
    // GRUG_TOKEN_OR
    // GRUG_TOKEN_NOT
    // GRUG_TOKEN_TRUE
    // GRUG_TOKEN_FALSE
    // GRUG_TOKEN_IF
    // GRUG_TOKEN_ELSE
    // GRUG_TOKEN_WHILE
    // GRUG_TOKEN_BREAK
    // GRUG_TOKEN_RETURN
    // GRUG_TOKEN_CONTINUE
    // GRUG_TOKEN_SPACE
    // GRUG_TOKEN_INDENTATION
    // GRUG_TOKEN_STRING
    // GRUG_TOKEN_WORD
    // GRUG_TOKEN_NUMBER
    // GRUG_TOKEN_COMMENT
	return (struct grug_tokens){0};
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
