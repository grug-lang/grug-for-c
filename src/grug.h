#ifndef GRUG_HEADER_H
#define GRUG_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
// SECTION: game-defined c-compile-time types 

// Here are your options as someone compiling grug:
// - GRUG_NUMBER: the type to use for numbers, double if undefined
// - GRUG_NO_NUMBER: define this if you don't want / can't have numbers
// - GRUG_BOOL: the type to use for booleans, bool if undefined
// - GRUG_NO_BOOL: define this if you don't want / can't have bools
// - GRUG_STRING: the type to use for strings, char const* if undefined
// - GRUG_NO_STRING: define this if you don't want / can't have strings
// - GRUG_ID: the type to use for ids, uint64_t if undefined
// - GRUG_NO_ID: define this if you don't want / can't have ids, note that GRUG_ID still applies for instances of scripts
// - GRUG_ON_FN_ID: the type to use for on_fn ids, uint64_t if undefined
// - GRUG_NO_CHECKS: when defined, grug is put into no-check mode which does no type checking on arguments


#ifndef GRUG_NUMBER
    #define GRUG_NUMBER double
#endif

#ifndef GRUG_BOOL
    #define GRUG_BOOL bool
#endif

#ifndef GRUG_STRING
    #define GRUG_STRING char const*
#endif

#ifndef GRUG_ID
    #define GRUG_ID uint64_t
#endif

#ifndef GRUG_ON_FN_ID
    #define GRUG_ON_FN_ID uint64_t
#endif

// TODO: maybe devs want file id to be different from the regular id?
// File id points to a specific script file, not an entity or object
typedef GRUG_ID GRUG_FILE_ID;

// TODO: maybe devs want entity id to be different from the regular id?
// Entity id points to a specific entity, not an object
typedef GRUG_ID GRUG_ENTITY_ID;

union grug_value {
    #ifndef GRUG_NO_NUMBER
        GRUG_NUMBER _number;
    #endif
    #ifndef GRUG_NO_BOOL
        GRUG_BOOL _bool;
    #endif
    #ifndef GRUG_NO_STRING
        GRUG_STRING _string;
    #endif
    #ifndef GRUG_NO_ID
        GRUG_ID _id;
    #endif
};

enum grug_type_enum {
    grug_type_number,
    grug_type_bool,
    grug_type_string,
    grug_type_id,
};

typedef uint32_t grug_type;

struct grug_value_typed {
    grug_type type;
    union grug_value value;
};

struct grug_state;

typedef void (*game_fn_void)(struct grug_state* gst, GRUG_ID me, const union grug_value[]);
typedef void (*game_fn_void_argless)(struct grug_state* gst, GRUG_ID me);
typedef union grug_value (*game_fn_value)(struct grug_state* gst, GRUG_ID me, const union grug_value[]);
typedef union grug_value (*game_fn_value_argless)(struct grug_state* gst, GRUG_ID me);

enum grug_error_type_enum {
    grug_error_type_stack_overflow = 0,
    grug_error_type_time_limit_exceeded,
    grug_error_type_game_fn_error,
};

// Enums are a allegedly wild west of compiler-specific abi and syntax so just say it's 32 bits
typedef uint32_t grug_error_type;

struct grug_init_settings {
    void* user_alloc_obj;
    // TODO: should probably use typedefs instead of inline function pointer types
    // When null, grug simply calls malloc() / free() instead.
    void* (*grug_user_alloc_fn)(void* me, size_t size);
    void (*grug_user_free_fn)(void* me, void* ptr, size_t size);

    // When null, grug assumes "[cwd]/mods"
    char const* mods_folder;

    // TODO: probably typedef instead of inline type
    // TODO: use strings or give the user the actual ids to the script + function?
    void (*runtime_error_handler)(char const* reason, grug_error_type type, char const* on_fn_name, char const* on_fn_path);
};

// SECTION: functions

struct grug_init_settings grug_default_settings(void);

struct grug_state* grug_init(struct grug_init_settings settings);

void grug_register_game_fn_void_argless(struct grug_state* gst, char const* game_fn_name, game_fn_void_argless fn);
void grug_register_game_fn_value_argless(struct grug_state* gst, char const* game_fn_name, game_fn_value_argless fn);
void grug_register_game_fn_void(struct grug_state* gst, char const* game_fn_name, game_fn_void fn);
void grug_register_game_fn_value(struct grug_state* gst, char const* game_fn_name, game_fn_value fn);

GRUG_ON_FN_ID grug_get_fn_id(struct grug_state* gst, char const* type, char const* on_fn_name);

void just_making_sure_tests_can_call_this(void);

GRUG_FILE_ID grug_get_script(struct grug_state* gst, char const* script_name);

GRUG_ENTITY_ID grug_create_entity(struct grug_state* gst, GRUG_FILE_ID script, GRUG_ID id);

void grug_deinit_entity(struct grug_state* gst, GRUG_ENTITY_ID entity);

void grug_update(struct grug_state* gst);

size_t grug_num_updates(struct grug_state* gst);

GRUG_FILE_ID grug_update_file(struct grug_state* gst, size_t update_index);

void grug_deinit(struct grug_state* gst);

void backend_call_argless(struct grug_state* gst, GRUG_ON_FN_ID fn, GRUG_ENTITY_ID entity);
void backend_call(struct grug_state* gst, GRUG_ON_FN_ID fn, GRUG_ENTITY_ID entity, const union grug_value args[]);
void backend_call_typed(struct grug_state* gst, GRUG_ON_FN_ID fn, GRUG_ENTITY_ID entity, const struct grug_value_typed args[]);

void grug_check_type(struct grug_state* gst, char const* game_fn, size_t index, grug_type type);

#define GRUG_GET_STRING(_state, _args, _index) GRUG_GET_ARG(_state, _args, _index, string)
#define GRUG_GET_BOOL(_state, _args, _index) GRUG_GET_ARG(_state, _args, _index, bool)
#define GRUG_GET_NUMBER(_state, _args, _index) GRUG_GET_ARG(_state, _args, _index, number)
#define GRUG_GET_ID(_state, _args, _index) GRUG_GET_ARG(_state, _args, _index, id)

// Can already check args since it just has to assert the function takes no args
#define GRUG_CALL_ARGLESS(_state, _on_fn, _entity) backend_call_argless(_state, _on_fn, _entity)
#ifdef GRUG_NO_CHECKS

#define GRUG_GET_ARG(_args, _index, _type) _args[_index]._##_type
#define GRUG_CALL(_state, _on_fn, _entity, ...) \
    do { \
        const union grug_value _grug_args[] = {__VA_ARGS__}; \
        grug_backend_call(_state, _on_fn, _entity, _grug_args); \
    } while(0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline union grug_value GRUG_ARG_NUMBER(GRUG_NUMBER v) { union grug_value r; r._number = v; return r; }
static inline union grug_value GRUG_ARG_BOOL(GRUG_BOOL v) { union grug_value r; r._bool = v; return r; }
static inline union grug_value GRUG_ARG_STRING(GRUG_STRING v) { union grug_value r; r._string = v; return r; }
static inline union grug_value GRUG_ARG_ID(GRUG_ID v) { union grug_value r; r._id = v; return r; }
#pragma GCC diagnostic pop

#else

#define GRUG_GET_ARG(_state, _args, _index, _type) (grug_check_type(_state, __func__, _index, grug_type_##_type), _args[_index]._##_type)

#define GRUG_CALL(_state, _on_fn, _entity, ...) \
    do { \
        const struct grug_value_typed _grug_args[] = {__VA_ARGS__}; \
        backend_call_typed(_state, _on_fn, _entity, _grug_args); \
    } while(0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline struct grug_value_typed GRUG_ARG_NUMBER(GRUG_NUMBER v) { struct grug_value_typed r; r.value._number = v; r.type = grug_type_number; return r; }
static inline struct grug_value_typed GRUG_ARG_BOOL(GRUG_BOOL v) { struct grug_value_typed r; r.value._bool = v; r.type = grug_type_bool; return r; }
static inline struct grug_value_typed GRUG_ARG_STRING(GRUG_STRING v) { struct grug_value_typed r; r.value._string = v; r.type = grug_type_string; return r; }
static inline struct grug_value_typed GRUG_ARG_ID(GRUG_ID v) { struct grug_value_typed r; r.value._id = v; r.type = grug_type_id; return r; }
#pragma GCC diagnostic pop

#endif

#ifdef __cplusplus
}
#endif

#endif //GRUG_HEADER_H
