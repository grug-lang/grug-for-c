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
	/// Null terminated, this doesn't use the grug_string type because benchmarks showed adding the extra 8 bytes per value halved argument passing performance even for non-string types.
	char const* _string;
	grug_id _id;
};

/// combines a null terminated C string with a length
struct grug_string {
	char* ptr;
	size_t len;
};

#define GRUG_WRAP_STRING(_str) (struct grug_string){.ptr = (char*)(_str), .len = strlen(_str)};

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
	GRUG_ERROR_TYPE_NONE = 0,
	GRUG_ERROR_TYPE_INIT,
	GRUG_ERROR_TYPE_COMPILE,
	GRUG_ERROR_TYPE_RUNTIME_STACK_OVERFLOW,
	GRUG_ERROR_TYPE_RUNTIME_TIME_LIMIT_EXCEEDED,
	GRUG_ERROR_TYPE_RUNTIME_GAME_FN_ERROR,
};

typedef uint32_t grug_error_type;

struct grug_error {
	grug_error_type error_type;
	/// A message that can be printed instead of parsing the data part of this struct
	struct grug_string message;
	union {
		struct {
			/// The file where the error occurred
			struct grug_string file_name;
			grug_file_id file;
			/// The function where the error occurred
			struct grug_string function_name;
			/// Note: will be set the most recently called on fn if the error occurred in a helper fn
			grug_on_fn_id function;
			/// The line number where the error occurred
			size_t line_number;
			/// The column number where the error occurred
			size_t column_number;
			/// the character index into the file where the error occurred.
			size_t first_character;
			/// The number of characters to highlight when reporting the error (how many characters to put the squiggly lines under)
			size_t num_characters;
		} runtime;
		struct {
			/// The file where the error occurred
			struct grug_string file_name;
			grug_file_id file;
			/// The function where the error occurred
			struct grug_string function_name;
			/// Note: will be set 0 (no id) error occurred in a helper fn
			grug_on_fn_id function;
			/// The line number where the error occurred
			size_t line_number;
			/// The column number where the error occurred
			size_t column_number;
			/// the character index into the file where the error occurred.
			size_t first_character;
			/// The number of characters to highlight when reporting the error (how many characters to put the squiggly lines under)
			size_t num_characters;
			struct grug_string error_code;
		} compiletime;
	} data;
};

struct grug_error_list {
	/// May be null of num_errors is 0.
	struct grug_error* errors;
	size_t num_errors;
};

struct grug_updates_list {
	size_t count;
	struct grug_file** updates;
};

struct grug_runtime_error_handler {
	void* user_data;
	void (*drop_fn)(void*);
	/// The handler function is expected to pull the error from the grug state, since the grug_error struct has grown to be rather large.
	void (*handler_fn)(
		struct grug_state* gst,
		void* user_data
	);
};

struct grug_on_fn_entry {
	struct grug_string entity_name;
	struct grug_string on_fn_name;
	grug_on_fn_id id;
};

struct grug_on_fns {
	struct grug_on_fn_entry* entries;
	size_t count;
};

struct grug_file {
	/// fill name of the mod file (ex: ak47-Gun.grug)
	struct grug_string name;
	/// what entity type this file implements (ex: Gun)
	struct grug_string entity_type;
	/// the name of the entity
	struct grug_string entity_name;

	/// file id
	grug_file_id id;

	/// Null if there is no error in this file
	struct grug_error* error;

	/// PRIVATE, When this file was last modified
	int64_t _mtime;

	/// PRIVATE, when resources seen by this script were last modified
	int64_t _resource_mtimes;
	size_t _resource_mtimes_size;

	bool _seen;
};

struct grug_mod_dir {
	/// Name of this folder
	struct grug_string name;

	struct grug_mod_dir** mods;
	size_t mods_size;

	struct grug_file* files;
	size_t files_size;  

	size_t _mods_capacity;
	size_t _files_capacity;

	bool _seen;
};

enum grug_token_type_enum {
	GRUG_TOKEN_OPEN_PARENTHESIS,
	GRUG_TOKEN_CLOSE_PARENTHESIS,
	GRUG_TOPEN_OPEN_BRACE,
	GRUG_TOKEN_CLOSE_BRACE,
	GRUG_TOKEN_PLUS,
	GRUG_TOKEN_MINUS,
	GRUG_TOKEN_MULTIPLICATION,
	GRUG_TOKEN_DIVISION,
	GRUG_TOKEN_COMMA,
	GRUG_TOKEN_NEWLINE,
	GRUG_TOKEN_EQUALS,
	GRUG_TOKEN_NOT_EQUALS,
	GRUG_TOKEN_ASSIGNMENT,
	GRUG_TOKEN_GREATER_OR_EQUAL,
    GRUG_TOKEN_GREATER,
    GRUG_TOKEN_LESS_OR_EQUAL,
    GRUG_TOKEN_LESS,
    GRUG_TOKEN_AND,
    GRUG_TOKEN_OR,
    GRUG_TOKEN_NOT,
    GRUG_TOKEN_TRUE,
    GRUG_TOKEN_FALSE,
    GRUG_TOKEN_IF,
    GRUG_TOKEN_ELSE,
    GRUG_TOKEN_WHILE,
    GRUG_TOKEN_BREAK,
    GRUG_TOKEN_RETURN,
    GRUG_TOKEN_CONTINUE,
    GRUG_TOKEN_SPACE,
    GRUG_TOKEN_INDENTATION,
    GRUG_TOKEN_STRING,
    GRUG_TOKEN_WORD,
    GRUG_TOKEN_NUMBER,
    GRUG_TOKEN_COMMENT,
};

typedef uint32_t grug_token_type;

struct grug_token {
	grug_token_type type;
	// Only defined for tokens that actually hold a string of contents.
	struct grug_string contents;
};

struct grug_tokens {
	struct grug_token* tokens;
	size_t tokens_len;
};

/* AST */

enum grug_type_type_enum {
	GRUG_TYPE_VOID = 0,
	GRUG_TYPE_BOOL,
	GRUG_TYPE_NUMBER,
	GRUG_TYPE_STRING,
	GRUG_TYPE_ID,
	GRUG_TYPE_RESOURCE,
	GRUG_TYPE_ENTITY,
};
typedef uint32_t grug_type_type;

struct grug_type {
	grug_type_type type;
	union {
		struct grug_string custom_name;   /* optionally used if type is GRUG_TYPE_ID */
		struct grug_string resource_type; /* used if type is GRUG_TYPE_RESOURCE */
		struct grug_string entity_type;   /* optionally used if type is GRUG_TYPE_ENTITY */
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
	GRUG_EXPR_TYPE_NOTHING,
	/* everything above this is a literal expr */
	GRUG_EXPR_TYPE_UNARY,
	GRUG_EXPR_TYPE_BINARY,
	GRUG_EXPR_TYPE_CALL,
	GRUG_EXPR_TYPE_PARENTHESIZED,
};
typedef uint32_t grug_expr_type;

// TODO: add location info to expressions
struct grug_expr {
	struct grug_type result_type; /* should be undetermined before type checking and filled in afterwards */

	/* Note: grug_rs puts the following two fields into a separate struct */ 
	/* This may cause a layout mismatch if any fields are added between result_type and type */
	/* If that is an issue, it can be solved by putting `type` and `expr_data` into an anonymous struct */
	grug_expr_type type;
	union {
		struct grug_string string;
		struct grug_string resource;
		struct grug_string entity;
		struct grug_string identifier_name;
		struct {
			double value;
			struct grug_string string;
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
			struct grug_string function_name;
			struct grug_expr* args;
			size_t args_count;
			void* game_fn_ptr;
		} call;
		struct grug_expr* parenthesized;
	} expr_data;
};

struct grug_member_variable {
	struct grug_string name;
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

// declare struct so the circular references work
/// ast statement
struct grug_statement;

struct grug_block {
	struct grug_statement* statements;
	size_t statements_len;
};

struct grug_if_branch {
	struct grug_expr cond;
	struct grug_block block;
};

typedef uint32_t grug_statement_type;

struct grug_statement {
	grug_statement_type type;
	union {
		struct {
			struct grug_string name;
			struct grug_type type; /* optional */
			struct grug_expr assignment_expr; 
		} variable;
		struct grug_expr call;
		struct {
			struct grug_if_branch branch;
			// Each branch is an if->do
			struct grug_if_branch* additional_branches;
			size_t additional_branches_len;
			struct grug_block else_block;
		} if_stmt;
		struct {
			struct grug_expr condition;
			struct grug_block block;
		} while_stmt;
		struct {
			struct grug_expr expr; /* Optional */
		} return_stmt;
		struct grug_string comment;
	} statement_data;
};

struct grug_argument {
	struct grug_string name; 
	struct grug_type type;
};

struct grug_on_function {
	struct grug_string name;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_block block;
};

struct grug_helper_function {
	struct grug_string name;
	struct grug_type return_type;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_block block;
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
	struct grug_string mod_api_path;
	struct grug_string mods_dir_path;
	struct grug_runtime_error_handler runtime_error_handler;
	struct grug_backend backend;
};

struct grug_init_settings grug_default_settings(void);

// Returns a non-null but "empty" state upon an error
struct grug_state* grug_init(struct grug_init_settings settings);

/// Returns a list of errors that have occurred. This list is cleared (and the memory returned here is invalidated) at the start of grug_update().
struct grug_error_list grug_get_errors(struct grug_state* gst);

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

// The game may call this at any point, even within an on_fn. However, a backend is entirely free to ignore this call if it happens within an on fn, so beware.
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

// This is basically a wrapper of malloc, but it's here to allow for a sensible alloc -> free lifetime with a pair of functions
// it does also allocate space for a null terminator
struct grug_string grug_alloc_string(size_t len);
void grug_free_string(struct grug_string str);

void grug_free_tokens(struct grug_tokens tokens);

void grug_free_ast(struct grug_ast ast);

struct grug_tokens grug_to_tokens(struct grug_string grug);
struct grug_ast tokens_to_ast(struct grug_tokens tokens);
struct grug_tokens ast_to_tokens(struct grug_ast ast);
struct grug_string tokens_to_grug(struct grug_tokens tokens);
struct grug_ast json_to_ast(struct grug_string json);
struct grug_string ast_to_json(struct grug_ast ast);

struct grug_ast grug_to_ast(struct grug_string grug);
struct grug_string ast_to_grug(struct grug_ast ast);
struct grug_string grug_to_json(struct grug_string grug);
struct grug_string json_to_grug(struct grug_string json);

#ifdef __cplusplus
}
#endif
