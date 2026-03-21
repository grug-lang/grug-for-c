#include <grug.h>

#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)

#include <dlfcn.h>

typedef void* DllLib;
#define load_library(name) dlopen(name, RTLD_NOW | RTLD_LOCAL)
#define load_symbol(lib, name) dlsym(lib, name)

#elif defined(WIN32) 

#include <windows.h>

typedef HMODULE DllLib;
#define load_library(name) LoadLibrary(name)
#define load_symbol(lib, name) (void*)GetProcAddress(lib, name);

#endif

static void (*p_grug_tests_run)(
    const char *,
	const char *,
	test_create_grug_state_t,
	test_destroy_grug_state_t,
    test_compile_grug_file_t,
    test_init_globals_fn_dispatcher_t,
    test_on_fn_dispatcher_t,
    test_dump_file_to_json_t,
    test_generate_file_from_json_t,
    test_game_fn_error_t,
    const char *
);

static void (*p_grug_tests_runtime_error_handler)(
    const char *,
    enum test_grug_runtime_error_type,
    const char *,
    const char *
);

static test_game_fn p_game_fn_nothing;
static test_game_fn p_game_fn_magic;
static test_game_fn p_game_fn_initialize;
static test_game_fn p_game_fn_initialize_bool;
static test_game_fn p_game_fn_identity;
static test_game_fn p_game_fn_max;
static test_game_fn p_game_fn_say;
static test_game_fn p_game_fn_sin;
static test_game_fn p_game_fn_cos;
static test_game_fn p_game_fn_mega;
static test_game_fn p_game_fn_get_false;
static test_game_fn p_game_fn_set_is_happy;
static test_game_fn p_game_fn_mega_f32;
static test_game_fn p_game_fn_mega_i32;
static test_game_fn p_game_fn_draw;
static test_game_fn p_game_fn_blocked_alrm;
static test_game_fn p_game_fn_spawn;
static test_game_fn p_game_fn_has_resource;
static test_game_fn p_game_fn_has_entity;
static test_game_fn p_game_fn_has_string;
static test_game_fn p_game_fn_get_opponent;
static test_game_fn p_game_fn_get_os;
static test_game_fn p_game_fn_set_d;
static test_game_fn p_game_fn_set_opponent;
static test_game_fn p_game_fn_motherload;
static test_game_fn p_game_fn_motherload_subless;
static test_game_fn p_game_fn_offset_32_bit_f32;
static test_game_fn p_game_fn_offset_32_bit_i32;
static test_game_fn p_game_fn_offset_32_bit_string;
static test_game_fn p_game_fn_talk;
static test_game_fn p_game_fn_get_position;
static test_game_fn p_game_fn_set_position;
static test_game_fn p_game_fn_cause_game_fn_error;
static test_game_fn p_game_fn_call_on_b_fn;
static test_game_fn p_game_fn_store;
static test_game_fn p_game_fn_retrieve;
static test_game_fn p_game_fn_box_number;

static const char *saved_grug_tests_root;
static const char *saved_grug_file_path;
static const char *saved_on_fn_name;

struct grug_string alloc_and_read_entire_file(char const* path) {
    FILE* f = fopen(path, "rb");
    assert(f && "Failed to open file");
    // this is kinda a terrible way to do this, but eh
    // This read all function will only be called on valid regular files
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    struct grug_string str = grug_alloc_string(size);
    fread((char*)str.ptr, str.len, 1, f);
    fclose(f);
    str.ptr[str.len] = 0;
    return str;
}

void write_entire_file(char const* path, struct grug_string str) {
    FILE* f = fopen(path, "wb");
    assert(f && "Failed to open file");
    fwrite(str.ptr, str.len, 1, f);
    fclose(f);
}

static const char *compile_grug_file(void* grug_state, const char *grug_file_path) {
	(void)(grug_state);
    (void)grug_file_path;

    return "Not Implemented";
}

static void init_globals_fn_dispatcher(void* grug_state) {
    (void)grug_state;
}

static void on_fn_dispatcher(void* grug_state, const char *on_fn_name, const union test_grug_value args[]) {
    (void)grug_state;
    (void)on_fn_name;
    (void)args;
}


static bool dump_file_to_json(void* grug_state, const char *input_grug_path, const char *output_json_path) {
	(void)grug_state;
    struct grug_string grug_file = alloc_and_read_entire_file(input_grug_path);
    struct grug_string grug_json = grug_to_json(grug_file);
    // TODO: handle potential syntax errors
    write_entire_file(output_json_path, grug_json);
    grug_free_string(grug_json);
    grug_free_string(grug_file);
    return true;
}

static bool generate_file_from_json(void* grug_state, const char *input_json_path, const char *output_grug_path) {
	(void)grug_state;
    struct grug_string grug_json = alloc_and_read_entire_file(input_json_path);
    struct grug_string grug_file = json_to_grug(grug_json);
    // TODO: handle potential syntax errors
    write_entire_file(output_grug_path, grug_file);
    grug_free_string(grug_json);
    grug_free_string(grug_file);
    return true;
}

static void game_fn_error(void* grug_state, const char *message) {
	(void)(grug_state);
    p_grug_tests_runtime_error_handler(message, GRUG_ON_FN_GAME_FN_ERROR, saved_on_fn_name, saved_grug_file_path);
}

static void* create_grug_state(const char* mod_api_dir, const char* mods_dir) {
    struct grug_init_settings settings = grug_default_settings();
    settings.mods_dir_path = GRUG_WRAP_STRING(mods_dir);
    settings.mod_api_path = GRUG_WRAP_STRING(mod_api_dir);
    struct grug_state* gst = grug_init(settings);
	return (void*)gst;
}

static void destroy_grug_state(void* state) {
	grug_deinit((struct grug_state*)state);
}


static void* load_sym(void *h, const char *name) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #pragma GCC diagnostic ignored "-Wint-conversion"
    void* p = load_symbol(h, name);
    assert(p && "Failed to load required symbol from tests.so");
    return p;
    #pragma GCC diagnostic pop
}

static char const* find_and_load_tests_so(void) {

	#if defined(__linux__)
	#define LIBNAME "libtests.so"
	#elif defined(WIN32)
	#define LIBNAME "tests.dll"
	#endif

    // Look around for where the tests lib might be
    char const* const locations[] = {
        "./",
        "../",
        "../grug-tests/",
        "../../",
        "../../grug-tests/",
        0
    };

    char buffer[512];

    DllLib h = 0;
    size_t location_index = 0;
    while(!h && locations[location_index] != 0) {
        snprintf(buffer, 512, "%s%s", locations[location_index], "build/" LIBNAME);
        h = load_library(buffer);
        if(h) {
            printf("Found grug tests lib at %s\n", buffer);
        }
        location_index += 1;
    }
    assert(h && "Could not load shared library");

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    p_grug_tests_run = load_sym(h, "grug_tests_run");
    p_grug_tests_runtime_error_handler = load_sym(h, "grug_tests_runtime_error_handler");

    p_game_fn_nothing = load_sym(h, "game_fn_nothing");
    p_game_fn_magic = load_sym(h, "game_fn_magic");
    p_game_fn_initialize = load_sym(h, "game_fn_initialize");
    p_game_fn_initialize_bool = load_sym(h, "game_fn_initialize_bool");
    p_game_fn_identity = load_sym(h, "game_fn_identity");
    p_game_fn_max = load_sym(h, "game_fn_max");
    p_game_fn_say = load_sym(h, "game_fn_say");
    p_game_fn_sin = load_sym(h, "game_fn_sin");
    p_game_fn_cos = load_sym(h, "game_fn_cos");
    p_game_fn_mega = load_sym(h, "game_fn_mega");
    p_game_fn_get_false = load_sym(h, "game_fn_get_false");
    p_game_fn_set_is_happy = load_sym(h, "game_fn_set_is_happy");
    p_game_fn_mega_f32 = load_sym(h, "game_fn_mega_f32");
    p_game_fn_mega_i32 = load_sym(h, "game_fn_mega_i32");
    p_game_fn_draw = load_sym(h, "game_fn_draw");
    p_game_fn_blocked_alrm = load_sym(h, "game_fn_blocked_alrm");
    p_game_fn_spawn = load_sym(h, "game_fn_spawn");
    p_game_fn_has_resource = load_sym(h, "game_fn_has_resource");
    p_game_fn_has_entity = load_sym(h, "game_fn_has_entity");
    p_game_fn_has_string = load_sym(h, "game_fn_has_string");
    p_game_fn_get_opponent = load_sym(h, "game_fn_get_opponent");
    p_game_fn_get_os = load_sym(h, "game_fn_get_os");
    p_game_fn_set_d = load_sym(h, "game_fn_set_d");
    p_game_fn_set_opponent = load_sym(h, "game_fn_set_opponent");
    p_game_fn_motherload = load_sym(h, "game_fn_motherload");
    p_game_fn_motherload_subless = load_sym(h, "game_fn_motherload_subless");
    p_game_fn_offset_32_bit_f32 = load_sym(h, "game_fn_offset_32_bit_f32");
    p_game_fn_offset_32_bit_i32 = load_sym(h, "game_fn_offset_32_bit_i32");
    p_game_fn_offset_32_bit_string = load_sym(h, "game_fn_offset_32_bit_string");
    p_game_fn_talk = load_sym(h, "game_fn_talk");
    p_game_fn_get_position = load_sym(h, "game_fn_get_position");
    p_game_fn_set_position = load_sym(h, "game_fn_set_position");
    p_game_fn_cause_game_fn_error = load_sym(h, "game_fn_cause_game_fn_error");
    p_game_fn_call_on_b_fn = load_sym(h, "game_fn_call_on_b_fn");
    p_game_fn_store = load_sym(h, "game_fn_store");
    p_game_fn_retrieve = load_sym(h, "game_fn_retrieve");
    p_game_fn_box_number = load_sym(h, "game_fn_box_number");
    #pragma GCC diagnostic pop

    // Let the caller know where we found it
    return locations[location_index-1];
}

int main(int argc, const char *argv[]) {
    saved_grug_tests_root = find_and_load_tests_so();

    char tests_dir[512];
    snprintf(tests_dir, 512, "%s%s", saved_grug_tests_root, "tests");

    char mod_api[512];
    snprintf(mod_api, 512, "%s%s", saved_grug_tests_root, "mod_api.json");

    const char *whitelisted_test = NULL;
    if (argc == 2) {
        whitelisted_test = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s <test name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    p_grug_tests_run(
        tests_dir,
		mod_api,
		create_grug_state,
		destroy_grug_state,
        compile_grug_file,
        init_globals_fn_dispatcher,
        on_fn_dispatcher,
        dump_file_to_json,
        generate_file_from_json,
        game_fn_error,
        whitelisted_test
    );
}
