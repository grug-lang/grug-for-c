#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t grug_id;

typedef grug_id grug_on_fn_id;
#define INVALID_GRUG_ON_FN_ID UINT64_MAX

typedef grug_id grug_file_id;
#define INVALID_GRUG_FILE_ID UINT64_MAX

union grug_value {
    double _number;
    bool _bool;
    char const* _string;
    grug_id _id;
};

struct grug_state;

// Information about an entity. 
// These fields should be treated as readonly by the game
// Backends can modify `data` when initialing or deinitializing data
struct grug_entity {
	grug_id id;
	grug_file_id file_id;
	void* data;
};

typedef union grug_value (*game_fn)(struct grug_state* gst, const union grug_value[]);

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

// TODO: use strings or give the user the file_id and on_fn_id?
struct grug_runtime_error_handler {
	void* data;
	void (*drop_fn)(void*);
	void (*handler_fn)(
		void* data,
		uint32_t err_kind,
		char* reason,
		size_t reason_len,
		char* on_fn_name,
		size_t on_fn_name_len,
		char* script_path,
		size_t script_path_len
	);
};

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
	struct grug_type* result_type; /* should be null before type checking and filled in afterwards */

	/* Note: grug_rs puts the following two fields into a separate struct */ 
	/* This may cause a layout mismatch if any fields are added between result_type and type */
	/* If that is an issue, it can be solved by putting `type` and `expr_data` into an anonymous struct */
	grug_expr_type type;
	union {
		char* string;
		char* resource;
		char* entity;
		char* identifier_name;
		struct {
			double value;
			char* string;
		} number;
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
			void* game_fn_ptr;
		} call;
		struct grug_expr* parenthesized;
	} expr_data;
};

struct grug_member_variable {
	char* name;
	struct grug_type type; 
	struct grug_expr assignment_expr; 
};

enum grug_statement_type_enum {
	GRUG_STATEMENT_VARIABLE = 0,
	GRUG_STATEMENT_CALL,
	GRUG_STATEMENT_IF,
	GRUG_STATEMENT_WHILE,
	GRUG_STATEMENT_RETURN,
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
			struct grug_type* type; /* optional */
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
			struct grug_expr* expr; /* Optional */
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

	/* 
	 * Each on function entry may be null which indicates that that on_function
	 * was not present in the script 
	 * */ 
	struct grug_on_function** on_functions; 
	size_t on_functions_count; 

	struct grug_helper_function* helper_function;
	size_t helper_functions_count;
};

/* AST */

// Free all resource owned by the backend
typedef void (*grug_backend_vtable_drop)(void* backend_data);
/// The AST of a typechecked grug file is provided to let the backend do
/// further transforms and lower to bytecode or even machine code
/// `ast` owns allocations that are freed once this function returns. Ensure
/// all resources (including strings) are copied out before it returns;
/// 
/// The script ids are guaranteed to be in contiguous ascending order.
///
/// If the same script id is returned again, then it means the old script
/// associated with the id should be destroyed and replaced with this one. 
///
/// The entity data of all entities created from the old script should be
/// regenerated
typedef void (*grug_backend_vtable_compile_script)(void* backend_data, grug_file_id file_id, struct grug_ast ast);
/// Initialize the member data of the newly created entity. When this
/// function is called, the member field of `entity` points to garbage and
/// must not be deinitialized. The GrugScriptId to be used is obtained from
/// the file_id member of `entity`. 
///
/// `entity` is pinned until it is deinitialized by a call to
/// `destroy_entity_data` or `insert_file` with the same path as its
/// current GrugScriptId. The reference must be stored as a raw pointer
/// within self so that it can be used during `destroy_entity_data` to
/// check for pointer equality. 
/// It is safe to use that pointer as a &GrugEntity in the meantime.
///
/// Returns false if there was a runtime error during execution
typedef bool (*grug_backend_vtable_init_entity)(void* backend_data, struct grug_state* gst, struct grug_entity* entity); 
/// Deinitialize all the data associated with all entities. The pointers
/// stored during `init_entity` must be used to get access to the entity data.
/// The entities can only be accessed as a &GrugEntity even self is available with an exclusive reference
typedef bool (*grug_backend_vtable_clear_entities)(void* backend_data); 
/// Deinitialize the data associated with `entity`. 
typedef void (*grug_backend_vtable_destroy_entity_data)(void* backend_data, struct grug_entity* entity);
/// Run the on function at index `on_fn_index` of the script associated
/// with `entity`.
///
/// # SAFETY: `values` must point to an array of GrugValues of at least as
/// many elements as the number of arguments to the on_ function
///
/// If the number of arguments is 0, then `values` is allowed to be null
typedef bool (*grug_backend_vtable_call_on_function_raw)(void* backend_data, struct grug_state* gst, struct grug_entity* entity, uint64_t on_fn_index, union grug_value* args); 
/// Run the on function at index `on_fn_index` of the script associated
/// with `entity`.
///
/// # Panics: The length of `values` must exactly match the number of
/// expected arguments to the on_ function
typedef bool (*grug_backend_vtable_call_on_function)(void* backend_data, struct grug_state* gst, struct grug_entity* entity, uint64_t on_fn_index, union grug_value* args, size_t args_len); 

struct grug_backend_vtable {
    grug_backend_vtable_compile_script compile_script;
	grug_backend_vtable_init_entity init_entity;
	grug_backend_vtable_clear_entities clear_entities;
	grug_backend_vtable_destroy_entity_data entity_data;
	grug_backend_vtable_call_on_function_raw call_on_function_raw;
	grug_backend_vtable_call_on_function call_on_function;
    grug_backend_vtable_drop drop;
};

struct grug_backend {
    void* obj;
    struct grug_backend_vtable* vtable;
};

// TODO: This should prolly be implementation specific
struct grug_init_settings {
	// TODO: We probably want a way to define the mod_api as a string (at least for prototyping)
	char* mod_api_path;
	size_t mod_api_path_len;
	char* mods_dir_path;
	size_t mods_dir_path_len;
	struct grug_runtime_error_handler runtime_error_handler;
	struct grug_backend backend;
};

struct grug_init_settings grug_default_settings(void);

struct grug_state* grug_init(struct grug_init_settings settings);

// returns true if registration is successful
// returns false if not.
//
// Reasons for failure include but are not limited to 
// 	- function was not defined in `mod_api.json`. 
// 	- function has already been registered
bool grug_register_game_fn(struct grug_state* gst, char const* game_fn_name, game_fn fn);

// Returns true if all game functions defined in mod_api.json are registered
bool grug_all_game_functions_registered(struct grug_state* gst);

// Get the on_fn_id for a particular on_ function for a particular entity
grug_on_fn_id grug_get_on_fn_id(struct grug_state* gst, const char* entity_type, const char* on_fn_name);

// Returns a list of all the fn ids for the mod_api.json
struct grug_on_fns grug_get_fn_ids(struct grug_state* gst);

// Compiles a single file from the mods directory
grug_file_id grug_compile_file(struct grug_state* gst, const char* path);
// Compile a file from a string. Useful for prototypeing or for built in scripts
// If it overlaps with a path on the actual filesystem, it is given the same id as that path
grug_file_id grug_compile_file_from_str(struct grug_state* gst, const char* path, char* file_text);

// Compiles and inserts all grug files in the mods directory
const struct grug_mod_dir* grug_get_mods(struct grug_state* gst);

// TODO: would it make any sense to be able to attach that grug-side entity data to an object after the fact instead of at creation?
// TODO: alternatively, does it make sense to be able to strip the entity part of an ID?
// Instantiate an entity from a script
grug_id grug_create_entity(struct grug_state* gst, grug_file_id script);

// Gets the file id of an entity, or 0 (null id) if the ID given isn't an entity or doesn't exist.
grug_file_id grug_entity_get_file_id(struct grug_state* gst, grug_id entity);

// Gets the entity data of an entity, or NULL if the ID given isn't an entity or doesn't exist.
struct grug_entity* grug_entity_get_data(struct grug_state* gst, grug_id entity);

// Create an object ID.
grug_id grug_create_object(struct grug_state* gst);

// Destroy the data associated with an entity. Does nothing If called on a non-entity object or non-existent id. TODO: should this have an error?
void grug_deinit_entity(struct grug_state* gst, grug_id entity);

/// The values returned are entirely allocated temporarily and are 'freed' when grug_update is called again.
struct grug_updates_list grug_update(struct grug_state* gst);

// Destroy a grug state and free all its resources
void grug_deinit(struct grug_state* gst);

void grug_swap_backend(struct grug_state* gst, struct grug_backend backend);
// TODO: Should this be done per script? or maybe per function call?
void grug_set_fast_mode(struct grug_state* gst, bool fast);

// returns false if on function could not be executed, if the id given isn't an entity, or if there was a runtime error
// `args` can be NULL if there are no arguments
bool grug_call_on_function_raw(struct grug_state* gst, grug_id entity, grug_on_fn_id on_fn_id, union grug_value* args);
bool grug_call_on_function(struct grug_state* gst, grug_id entity, grug_on_fn_id on_fn_id, union grug_value* args, size_t args_len);

#define GRUG_CALL_ARGLESS(_state, _entity, _on_fn_id) \
        grug_call_on_function(_state, _entity, _on_fn_id, NULL, 0); \

#define GRUG_CALL(_state, _entity, _on_fn_id, _args_count, ...) \
        grug_call_on_function(_state, _entity, _on_fn_id, (union grug_value[]) {__VA_ARGS__}, _args_count); \

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline union grug_value GRUG_ARG_NUMBER(double v)      {union grug_value r; r._number = v; return r;}
static inline union grug_value GRUG_ARG_BOOL(bool v)          {union grug_value r; r._bool = v  ; return r;}
static inline union grug_value GRUG_ARG_STRING(char const* v) {union grug_value r; r._string = v; return r;}
static inline union grug_value GRUG_ARG_ID(grug_id v)         {union grug_value r; r._id = v    ; return r;}
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
