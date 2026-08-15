#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// MARK: types

typedef uint64_t grug_id;

typedef grug_id grug_on_fn_id;
#define INVALID_GRUG_ON_FN_ID UINT64_MAX

typedef grug_id grug_file_id;
#define INVALID_GRUG_FILE_ID UINT64_MAX

typedef grug_id grug_entity_id;
#define INVALID_GRUG_ENTITY_ID UINT64_MAX

typedef grug_id grug_object_id;
#define INVALID_GRUG_OBJECT_ID UINT64_MAX

union grug_value {
	double _number;
	bool _bool;
	/// Null terminated, this doesn't use the grug_string type because benchmarks showed adding the extra 8 bytes per value halved argument passing performance even for non-string types.
	char const* _string;
	grug_object_id _id;
};

struct grug_state;

// Information about an entity. 
// These fields should be treated as readonly by the game
// Backends can modify `data` when initialing or deinitializing data
struct grug_entity {
	grug_entity_id id;
	grug_file_id file_id;
	grug_object_id me;
	void* data;
};

typedef union grug_value (*game_fn)(struct grug_state* gst, void* data, const union grug_value[]);

struct grug_error_code {
	// four components make an error code, each one more specific than the last
	// See the macros below for specific errors that can happen
	uint8_t tag[4];
};

#define GRUG_ERROR_CODE_NONE ((struct grug_error_code) {{0, 0, 0, 0}})
#define GRUG_ERROR_CODE_INIT ((struct grug_error_code) {{1, 0, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE ((struct grug_error_code) {{2, 0, 0, 0}})
#define GRUG_ERROR_CODE_RUNTIME ((struct grug_error_code) {{3, 0, 0, 0}})

#define GRUG_ERROR_CODE_INIT_MOD_API ((struct grug_error_code){{1, 1, 0, 0}});
#define GRUG_ERROR_CODE_INIT_FUNCTION_REGISTRATION ((struct grug_error_code){{1, 2, 0, 0}});

#define GRUG_ERROR_CODE_INIT_MOD_API_IO ((struct grug_error_code){{1, 1, 1, 0}});
#define GRUG_ERROR_CODE_INIT_MOD_API_JSON ((struct grug_error_code){{1, 1, 2, 0}});

#define GRUG_ERROR_CODE_COMPILE_IO ((struct grug_error_code) {{2, 1, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE_FILE_NAME ((struct grug_error_code) {{2, 2, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE_UTF8 ((struct grug_error_code) {{2, 3, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE_TOKENIZER ((struct grug_error_code) {{2, 4, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE_PARSER ((struct grug_error_code) {{2, 5, 0, 0}})
#define GRUG_ERROR_CODE_COMPILE_TYPE_CHECKER ((struct grug_error_code) {{2, 6, 0, 0}})

#define GRUG_ERROR_CODE_COMPILE_FILE_NAME_EMPTY_FILE ((struct grug_error_code) {{2, 2, 1, 0}})

struct grug_file_location {
	/// null terminated file name
	char const* file_name;
	grug_file_id file;
	/// the character index into the file where the error occurred.
	size_t offset;
	/// The number of characters to highlight when reporting the error (how many characters to put the squiggly lines under)
	size_t num_characters;
};

enum grug_callstack_entry_type_enum {
	GRUG_CALLSTACK_ENTRY_TYPE_GAME_FN,
	GRUG_CALLSTACK_ENTRY_TYPE_ON_FN,
};

typedef uint32_t grug_callstack_entry_type;

struct grug_callstack_entry {
	grug_callstack_entry_type type;
	char const* fn_name;
};

struct grug_callstack {
	struct grug_callstack_entry* entries;
	size_t num_entries;
};

struct grug_error {
	struct grug_error_code error_type;
	char const* message;
	/// custom implementation-specific message that doesn't necessarily pass the testing suite
	char const* custom_message;
	/// Information for if the error occurred within a grug script
	struct grug_file_location file;
	/// The callstack at the point of the error if it is available
	struct grug_callstack callstack;
	/// An error may own quite a lot of allocations, hence the arena.
	/// To free the error, simply destroy the arena.
	struct grug_arena* arena;
};

struct grug_updates_list {
	size_t count;
	struct grug_file* updates;
};

struct grug_runtime_error_handler {
	void* user_data;
	void (*drop_fn)(void*);
	void (*handler_fn)(
		struct grug_state* gst,
		struct grug_error* error,
		void* user_data
	);
};

struct grug_logger {
	void* user_data;
	void (*drop_fn)(void*);
	/// Info logs are meant to be preserved in release builds
	void (*log_info)(struct grug_state* gst, void* user_data, char const* message);
	/// Debug logs are meant to only be enabled with a debug log flag enabled
	void (*log_debug)(struct grug_state* gst, void* user_data, char const* message);
	/// Trace logs are extremely verbose and are disabled by default
	void (*log_trace)(struct grug_state* gst, void* user_data, char const* message);
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
	struct grug_error* error;
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

enum grug_token_type_enum {
	GRUG_TOKEN_TYPE_OPEN_PARENTHESIS,
	GRUG_TOKEN_TYPE_CLOSE_PARENTHESIS,
	GRUG_TOKEN_TYPE_OPEN_BRACE,
	GRUG_TOKEN_TYPE_CLOSE_BRACE,
	GRUG_TOKEN_TYPE_OPEN_BRACKET,
	GRUG_TOKEN_TYPE_CLOSE_BRACKET,
	GRUG_TOKEN_TYPE_PLUS,
	GRUG_TOKEN_TYPE_MINUS,
	GRUG_TOKEN_TYPE_STAR,
	GRUG_TOKEN_TYPE_FORWARD_SLASH,
	GRUG_TOKEN_TYPE_COMMA,
	GRUG_TOKEN_TYPE_COLON,
	GRUG_TOKEN_TYPE_DOT,
	GRUG_TOKEN_TYPE_NEW_LINE,
	GRUG_TOKEN_TYPE_DOUBLE_EQUALS,
	GRUG_TOKEN_TYPE_NOT_EQUALS,
	GRUG_TOKEN_TYPE_EQUAL,
	GRUG_TOKEN_TYPE_GREATER_EQUALS,
	GRUG_TOKEN_TYPE_GREATER,
	GRUG_TOKEN_TYPE_LESS_EQUALS,
	GRUG_TOKEN_TYPE_LESS,
	GRUG_TOKEN_TYPE_AND,
	GRUG_TOKEN_TYPE_OR,
	GRUG_TOKEN_TYPE_NOT,
	GRUG_TOKEN_TYPE_TRUE,
	GRUG_TOKEN_TYPE_FALSE,
	GRUG_TOKEN_TYPE_IF,
	GRUG_TOKEN_TYPE_ELSE,
	GRUG_TOKEN_TYPE_WHILE,
	GRUG_TOKEN_TYPE_BREAK,
	GRUG_TOKEN_TYPE_RETURN,
	GRUG_TOKEN_TYPE_CONTINUE,
	GRUG_TOKEN_TYPE_EXPORT,
	GRUG_TOKEN_TYPE_LOCAL,
	GRUG_TOKEN_TYPE_SPACE,
	GRUG_TOKEN_TYPE_INDENT,
	GRUG_TOKEN_TYPE_STRING,
	GRUG_TOKEN_TYPE_ENTITY,
	GRUG_TOKEN_TYPE_RESOURCE,
	GRUG_TOKEN_TYPE_WORD,
	GRUG_TOKEN_TYPE_NUMBER,
	GRUG_TOKEN_TYPE_COMMENT,
};

typedef uint32_t grug_token_type;

/// opaque struct so grug can internally use any arena impl down the road
struct grug_arena;

#define GRUG_SPACES_PER_INDENT 4

struct grug_token {
	grug_token_type type;
	/// When parsed from 'real' code this will always be set, however when generated from an AST or otherwise this may or may not be set.
	/// Will not be null terminated, as it is simply a window view into the file contents string which is stored elsewhere
	char const* contents;
	/// Number of bytes of the contents
	size_t contents_len;
};

// MARK: AST

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
		/// optionally used if type is GRUG_TYPE_ID
		char const* custom_name;
		/// used if type is GRUG_TYPE_RESOURCE
		char const* resource_type;
		/// optionally used if type is GRUG_TYPE_ENTITYe;
		char const* entity_type;
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

// TODO(bluesillybeard): add location info to expressions
struct grug_expr {
	struct grug_type result_type; /* should be undetermined before type checking and filled in afterwards */

	/* Note: grug_rs puts the following two fields into a separate struct */ 
	/* This may cause a layout mismatch if any fields are added between result_type and type */
	/* If that is an issue, it can be solved by putting `type` and `expr_data` into an anonymous struct */
	grug_expr_type type;
	union {
		char const* string;
		char const* resource;
		char const* entity;
		char const* identifier_name;
		struct {
			double value;
			char const* string;
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
			char const* function_name;
			struct grug_expr* args;
			size_t args_count;
			void* game_fn_ptr;
		} call;
		struct grug_expr* parenthesized;
	} expr_data;
};

struct grug_member_variable {
	char const* name;
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
			char const* name;
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
		char const* comment;
	} statement_data;
};

struct grug_argument {
	char const* name; 
	struct grug_type type;
};

struct grug_on_function {
	char const* name;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_block block;
};

struct grug_helper_function {
	char const* name;
	struct grug_type return_type;
	struct grug_argument* arguments;
	size_t arguments_len;
	struct grug_block block;
};

struct grug_ast {
	struct grug_member_variable* members;
	size_t members_count;
	
	struct grug_on_function* on_functions; 
	size_t on_functions_count; 

	struct grug_helper_function* helper_function;
	size_t helper_functions_count;
	struct grug_arena* _arena;
};

// MARK: backend

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

struct grug_init_settings {
	/// The raw text of the mod API
	/// May be NULL if the file path is defined instead.
	char const* mod_api_json_source;
	/// The file path. Can be an absolute path or relative to CWD. If relative to CWD, grug will remember what it was at init so changing the CWD at runtime has no ill effect on grug.
	/// May be NULL if the file source is defined instead.
	char const* mod_api_json_path;
	/// Can be an absolute path or relative to CWD. If relative to CWD, grug will remember what it was at init so changing the CWD at runtime has no ill effect on grug.
	char const* mods_dir_path;
	struct grug_runtime_error_handler runtime_error_handler;
	struct grug_logger logger;
	struct grug_backend backend;
};

// MARK: API

struct grug_init_settings grug_default_settings(void);

/// Returns null upon an error and writes to out_error
struct grug_state* grug_init(struct grug_init_settings settings, struct grug_error* out_error);

struct grug_error const* grug_get_error(struct grug_state* gst);

struct grug_callstack grug_get_callstack(struct grug_state* gst);

// returns true if registration is successful
// returns false if not.
//
// Reasons for failure include but are not limited to 
// 	- function was not defined in `mod_api.json`. 
// 	- function has already been registered
bool grug_register_game_fn(struct grug_state* gst, char const* game_fn_name, void* fn_data, game_fn fn_ptr);

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
grug_file_id grug_compile_file_from_str(struct grug_state* gst, const char* path, char const* file_text);

// Compiles and inserts all grug files in the mods directory
const struct grug_mod_dir* grug_get_mods(struct grug_state* gst);

// Instantiate an entity from a script
grug_entity_id grug_create_entity(struct grug_state* gst, grug_file_id script, grug_object_id me_id);

// Gets the file id of an entity, or 0 (null id) if the ID given isn't an entity or doesn't exist.
grug_file_id grug_entity_get_file_id(struct grug_state* gst, grug_entity_id entity);

// Gets the entity data of an entity, or NULL if the ID given isn't an entity or doesn't exist.
struct grug_entity* grug_entity_get_data(struct grug_state* gst, grug_entity_id entity);

// Destroy the data associated with an entity. Does nothing if called on a non-existent entity. TODO(bluesillybeard): should this have an error?
void grug_deinit_entity(struct grug_state* gst, grug_entity_id entity);

/// The values returned are entirely allocated temporarily and are 'freed' when grug_update is called again.
struct grug_updates_list grug_update(struct grug_state* gst);

// Destroy a grug state and free all its resources
void grug_deinit(struct grug_state* gst);

void grug_swap_backend(struct grug_state* gst, struct grug_backend backend);

// The game may call this at any point, even within an on_fn. However, a backend is entirely free to ignore this call if it happens within an on fn, so beware.
void grug_set_fast_mode(struct grug_state* gst, bool fast);

// returns false if on function could not be executed, if the id given isn't an entity, or if there was a runtime error
// `args` can be NULL if there are no arguments
bool grug_call_on_function_raw(struct grug_state* gst, grug_entity_id entity, grug_on_fn_id on_fn_id, union grug_value* args);
bool grug_call_on_function(struct grug_state* gst, grug_entity_id entity, grug_on_fn_id on_fn_id, union grug_value* args, size_t args_len);

void grug_game_fn_runtime_error(struct grug_state* gst, char const* message);

#define GRUG_CALL_ARGLESS(_state, _entity, _on_fn_id) \
		grug_call_on_function(_state, _entity, _on_fn_id, NULL, 0); \

#define GRUG_CALL(_state, _entity, _on_fn_id, _args_count, ...) \
		grug_call_on_function(_state, _entity, _on_fn_id, (union grug_value[]) {__VA_ARGS__}, _args_count); \

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline union grug_value GRUG_ARG_NUMBER(double value)      {union grug_value grug_value; grug_value._number = value; return grug_value;}
static inline union grug_value GRUG_ARG_BOOL(bool value)          {union grug_value grug_value; grug_value._bool = value  ; return grug_value;}
static inline union grug_value GRUG_ARG_STRING(char const* value) {union grug_value grug_value; grug_value._string = value; return grug_value;}
static inline union grug_value GRUG_ARG_ID(grug_object_id value)  {union grug_value grug_value; grug_value._id = value    ; return grug_value;}
#pragma GCC diagnostic pop

static inline bool grug_error_code_matches(struct grug_error_code left, struct grug_error_code right) {
	size_t i = 0; 
	while (i < sizeof(struct grug_error_code) / sizeof(uint8_t)) {
		if (left.tag[i] == 0 || right.tag[i] == 0) {
			return true;
		} else if (left.tag[i] != right.tag[i]) {
			return false;
		}
		i += 1;
	}
	return true;
}

struct grug_arena* grug_arena_new(void);
void* grug_arena_alloc(struct grug_arena* arena, size_t size);
char* grug_arena_copy(struct grug_arena* arena, char const* data, size_t size);
/// grug_arena_copy_string is null safe and will propagate either a null arena or a null string
char* grug_arena_copy_string(struct grug_arena* arena, char const* data);
void* grug_arena_alloc_aligned(struct grug_arena* arena, size_t size, size_t align);
/// Size is required so the arena may check if the allocation is the most recent one and can pull it off the stack
void grug_arena_free(struct grug_arena* arena, void* ptr, size_t size);
/// Size is required so the arena may check if the allocation is the most recent one and can simply extend it
void* grug_arena_realloc(struct grug_arena* arena, void* ptr, size_t old_size, size_t new_size);
/// clears out the memory allocated in an arena, optionally keeping a reserve capacity
void grug_arena_clear(struct grug_arena* arena, size_t reserve_bytes);
/// completely destroys an arena and all associated memory.
void grug_arena_deinit(struct grug_arena* arena);

struct grug_error grug_copy_error(struct grug_error const* err, struct grug_arena* arena_or_none);

/// replaces an existing error object with a new one, ideally without re-allocating all of the memory
void grug_assign_error(struct grug_error* err, struct grug_error const* new_err, struct grug_arena* arena_or_none);

static inline void grug_free_error(struct grug_error* err) {
	grug_arena_deinit(err->arena);
	*err = (struct grug_error) {0};
}

void grug_free_ast(struct grug_ast ast);

size_t grug_tokens_to_grug(struct grug_token const* tokens, size_t num_tokens, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

size_t grug_ast_to_grug(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

size_t grug_json_to_grug(char const* json, size_t json_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

size_t grug_grug_to_tokens(char const* grug, size_t grug_len, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error);

size_t grug_ast_to_tokens(struct grug_ast ast, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error);

size_t grug_json_to_tokens(char const* json, size_t json_len, struct grug_token* out_tokens, size_t out_tokens_capacity, struct grug_error* o_error);

struct grug_ast grug_grug_to_ast(char const* grug, size_t grug_len, struct grug_arena* arena_or_none, struct grug_error* o_error);

struct grug_ast grug_tokens_to_ast(struct grug_token const* tokens, size_t num_tokens, struct grug_arena* arena_or_none, struct grug_error* o_error);

struct grug_ast grug_json_to_ast(char const* json, size_t json_len, struct grug_arena* arena_or_none, struct grug_error* o_error);

size_t grug_grug_to_json(char const* grug, size_t grug_len, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

size_t grug_tokens_to_json(struct grug_token const* tokens, size_t num_tokens, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

size_t grug_ast_to_json(struct grug_ast ast, char* out_string_buffer, size_t out_string_buffer_capacity, struct grug_error* o_error);

#ifdef __cplusplus
}
#endif
