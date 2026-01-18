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

enum grug_type {
    grug_type_number,
    grug_type_bool,
    grug_type_string,
    grug_type_id,
};

// TODO: should this really be transparent? (do we let the user allocate the struct and inspect its members?)
struct grug_state {
    int TODO;
};

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

size_t grug_members_size(struct grug_state* gst, GRUG_FILE_ID script);

// grug stores the entity id in the members. An entities me is intrinsically tied to its members, and vice-versa.
void grug_init_members(struct grug_state* gst, GRUG_FILE_ID script, void* members, GRUG_ID entity_me_id);

// When the game calls update(), it needs to go through and explicitly realloc and call init_members on all the affected entities
void grug_update(struct grug_state* gst);

bool grug_script_was_updated(struct grug_state* gst, GRUG_FILE_ID script);

void grug_deinit(struct grug_state* gst);


// TODO MARK: TODO
#define GRUG_GET_STRING(_state, _args, _index) ((void)(_state), (void)(_args), (void)(_index), (char const*)(0))
#define GRUG_CALL_ARGLESS_VOID(_state, _fn_id, _members) ((void)(_state), (void)(_members), (void)(_fn_id), 0)
#define GRUG_CALL_VOID(_state, _fn_id, _members, _args) ((void)(_state), (void)(_members), (void)(_fn_id), (void)(_args), 0)
#define GRUG_ARGS(...) 0

#ifdef __cplusplus
}
#endif

#endif //GRUG_HEADER_H
