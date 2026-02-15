#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t grug_id;

typedef grug_id grug_on_fn_id;

typedef grug_id grug_file_id;

typedef grug_id grug_entity_id;

union grug_value {
    double _number;
    bool _bool;
    char const* _string;
    grug_id _id;
};

struct grug_state;

typedef void (*game_fn_void)(struct grug_state* gst, grug_id me, const union grug_value[]);
typedef void (*game_fn_void_argless)(struct grug_state* gst, grug_id me);
typedef union grug_value (*game_fn_value)(struct grug_state* gst, grug_id me, const union grug_value[]);
typedef union grug_value (*game_fn_value_argless)(struct grug_state* gst, grug_id me);

enum grug_error_type_enum {
    grug_error_type_stack_overflow = 0,
    grug_error_type_time_limit_exceeded,
    grug_error_type_game_fn_error,
};

typedef uint32_t grug_error_type;

struct grug_updates_list {
    size_t count;
    struct grug_file** updates;
};

// TODO: use strings or give the user the actual ids to the script + function?
typedef void (*runtime_error_handler)(char const* reason, grug_error_type type, char const* on_fn_name, char const* on_fn_path);

struct grug_on_fn_entry {
    char const* entity_name;
    char const* on_fn_name;
    grug_on_fn_id id;
};

struct grug_on_fns {
    struct grug_on_fn_entry* entries;
    size_t count;
};

struct grug_file {
    /// fill name of the mod file (ex: ak47-Gun.grug)
    char const* name;
    /// what entity type this file implements (ex: Gun)
    char const* entity_type;
    /// the name of the entity
    char const* entity_name;

    /// file id
    grug_file_id id;

    /// Null if there is no error in this file
    char* error_msg;
    size_t error_line_number;

    /// PRIVATE, When this file was last modified
    int64_t _mtime;

    /// PRIVATE, when resources seen by this script were last modified
    int64_t _resource_mtimes;
    size_t _resource_mtimes_size;

    bool _seen;
};

struct grug_mod_dir {
    /// Name of this folder
    char const* name;

    struct grug_mod_dir** mods;
    size_t mods_size;
    
    struct grug_file* files;
    size_t files_size;  
    
    size_t _mods_capacity;
    size_t _files_capacity;

    bool _seen;
};

/* AST */

enum grug_type_enum {
	GRUG_TYPE_VOID = 0,
	GRUG_TYPE_BOOL,
	GRUG_TYPE_NUMBER,
	GRUG_TYPE_STRING,
	GRUG_TYPE_ID,
	GRUG_TYPE_RESOURCE,
	GRUG_TYPE_ENTITY,
};
typedef uint32_t grug_type_enum_type;

struct grug_type {
	grug_type_enum_type type;
	union {
		char* custom_name;   /* optionally used if type is GRUG_TYPE_ID */
		char* resource_type; /* used if type is GRUG_TYPE_RESOURCE */
		char* entity_type;   /* optionally used if type is GRUG_TYPE_ENTITY */
	} extra_data;
};

enum grug_unary_operator_enum {
	GRUG_UNARY_NOT   = 0,
	GRUG_UNARY_MINUS,
};
typedef uint32_t grug_unary_operator;

enum grug_binary_operator_enum {
	GRUG_BINARY_OR = 0,
	GRUG_BINARY_AND,
	GRUG_BINARY_DOUBLEEQUALS,
	GRUG_BINARY_NOTEQUALS,
	GRUG_BINARY_GREATER,
	GRUG_BINARY_GREATEREQUALS,
	GRUG_BINARY_LESS,
	GRUG_BINARY_LESSEQUALS,
	GRUG_BINARY_PLUS,
	GRUG_BINARY_MINUS,
	GRUG_BINARY_MULTIPLY,
	GRUG_BINARY_DIVISION,
	GRUG_BINARY_REMAINDER,
};
typedef uint32_t grug_binary_operator;

enum grug_expr_type_enum {
	GRUG_EXPR_TYPE_TRUE = 0,
	GRUG_EXPR_TYPE_FALSE,
	GRUG_EXPR_TYPE_STRING,
	GRUG_EXPR_TYPE_RESOURCE,
	GRUG_EXPR_TYPE_ENTITY,
	GRUG_EXPR_TYPE_IDENTIFIER,
	GRUG_EXPR_TYPE_NUMBER,
	/* everything above this is a literal expr */
	GRUG_EXPR_TYPE_UNARY,
	GRUG_EXPR_TYPE_BINARY,
	GRUG_EXPR_TYPE_CALL,
	GRUG_EXPR_TYPE_PARENTHESIZED,
};
typedef uint32_t grug_expr_type;

// TODO: add location info to expressions
struct grug_expr {
	bool result_filled;
	struct grug_type result_type;
	grug_expr_type type;
	union {
		char* string;
		char* resource;
		char* entity;
		char* identifier_name;
		double number;
		struct {
			grug_unary_operator op;
			struct grug_expr* inner;
		} unary;
		struct {
			grug_binary_operator op;
			struct grug_expr* left;
			struct grug_expr* right;
		} binary;
		struct {
			char* function_name;
			struct grug_expr* args;
			size_t args_count;
		} call;
		struct grug_expr* parenthesized;
	} expr_data;
};

struct grug_member_variable {
	char* name;
	struct grug_type type;
	struct grug_expr assignment_expr; /* not a pointer */
};

enum grug_statement_type_enum {
	GRUG_STATEMENT_VARIABLE = 0,
	GRUG_STATEMENT_CALL,
	GRUG_STATEMENT_IF,
	GRUG_STATEMENT_WHILE,
	GRUG_STATEMENT_COMMENT,
	GRUG_STATEMENT_BREAK,
	GRUG_STATEMENT_CONTINUE,
	GRUG_STATEMENT_EMPTY,
};
typedef uint32_t grug_statement_type;

struct grug_statement {
	grug_statement_type type;
	union {
		struct {
			char* name;
			bool has_type;
			struct grug_type type; /* optional */
			struct grug_expr assignment_expr; 
		} variable;
		struct grug_expr call;
		struct {
			struct grug_expr condition;
			bool chained;
			struct grug_statement* if_block;
			size_t if_block_len;
			struct grug_statement* else_block;
			size_t else_block_len;
		} if_stmt;
		struct {
			struct grug_expr condition;
			struct grug_statement* block;
			size_t block_len;
		} while_stmt;
		struct {
			bool has_value;
			struct grug_expr expr;
		} return_stmt;
		char* comment;
	} statement_data;
};

struct grug_argument {
	char* name; 
	struct grug_type type;
};

struct grug_on_function {
	char* name;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_statement* body_statements;
	size_t body_statements_len;
};

struct grug_helper_function {
	char* name;
	struct grug_type return_type;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_statement* body_statements;
	size_t body_statements_len;
};

struct grug_ast {
    struct grug_member_variable* members;
	size_t members_count;

	struct grug_on_function* on_functions;
	size_t on_functions_count;

	struct grug_helper_function* helper_function;
	size_t helper_functions_count;
};

/* AST */

typedef void (*grug_backend_vtable_drop)(void* obj);
typedef grug_file_id (*grug_backend_vtable_compile_script)(void* obj, struct grug_ast* ast);

struct grug_backend_vtable {
    grug_backend_vtable_drop drop;
    grug_backend_vtable_compile_script compile_script;
    // TODO: finish backend vtable
};

struct grug_backend {
    void* obj;
    struct grug_backend_vtable* vtable;
};

struct grug_init_settings {
    void* user_alloc_obj;
    runtime_error_handler runtime_error_handler;

    // When null, grug assumes "[cwd]/mods"
    char const* mods_folder;

    struct grug_backend backend;
};

struct grug_init_settings grug_default_settings(void);

struct grug_state* grug_init(struct grug_init_settings settings);

void grug_swap_backend(struct grug_state* gst, struct grug_backend backend);
void grug_set_fast_mode(struct grug_state* gst, bool fast);

void grug_register_game_fn_void_argless(struct grug_state* gst, char const* game_fn_name, game_fn_void_argless fn);
void grug_register_game_fn_value_argless(struct grug_state* gst, char const* game_fn_name, game_fn_value_argless fn);
void grug_register_game_fn_void(struct grug_state* gst, char const* game_fn_name, game_fn_void fn);
void grug_register_game_fn_value(struct grug_state* gst, char const* game_fn_name, game_fn_value fn);

/// Returns a list of all the fn ids for the mod_api.json
struct grug_on_fns grug_get_fn_ids(struct grug_state* gst);

const struct grug_mod_dir* grug_get_mods(struct grug_state* gst);

grug_entity_id grug_create_entity(struct grug_state* gst, grug_file_id script, grug_id id);

grug_file_id grug_entity_get_file(struct grug_state* gst, grug_entity_id entity);

// me_id
grug_id grug_entity_get_id(struct grug_state* gst, grug_entity_id entity);

void grug_deinit_entity(struct grug_state* gst, grug_entity_id entity);

struct grug_updates_list grug_update(struct grug_state* gst);

void grug_deinit(struct grug_state* gst);

void grug_backend_call_argless(struct grug_state* gst, grug_on_fn_id fn, grug_entity_id entity);
void grug_backend_call(struct grug_state* gst, grug_on_fn_id fn, grug_entity_id entity, const union grug_value args[]);

#define GRUG_CALL_ARGLESS(_state, _on_fn, _entity) grug_backend_call_argless(_state, _on_fn, _entity)

#define GRUG_CALL(_state, _on_fn, _entity, ...) \
    do { \
        const union grug_value _grug_args[] = {__VA_ARGS__}; \
        grug_backend_call(_state, _on_fn, _entity, _grug_args); \
    } while(0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline union grug_value GRUG_ARG_NUMBER(double v) { union grug_value r; r._number = v; return r; }
static inline union grug_value GRUG_ARG_BOOL(bool v) { union grug_value r; r._bool = v; return r; }
static inline union grug_value GRUG_ARG_STRING(char const* v) { union grug_value r; r._string = v; return r; }
static inline union grug_value GRUG_ARG_ID(grug_id v) { union grug_value r; r._id = v; return r; }
#pragma GCC diagnostic pop

// TODO: use a temporary allocator instead probably, might require a state though?
struct grug_ast* grug_to_ast(char const* file_str, size_t file_len);
struct grug_ast* json_to_ast(char const* file_str, size_t file_len);
void grug_free_ast(struct grug_ast* ast);
char const* ast_to_json(struct grug_ast* ast, size_t* out_len);
char const* ast_to_grug(struct grug_ast* ast, size_t* out_len);
void grug_free_string(char const* str, size_t len);

#ifdef __cplusplus
}
#endif
