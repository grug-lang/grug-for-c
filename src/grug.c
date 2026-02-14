#include "grug.h"

struct grug_init_settings grug_default_settings(void) {
    return (struct grug_init_settings){ 
        // TODO: interpreter backend
        .backend = {.obj = 0, .vtable = 0},
        .mods_folder = 0,
        .runtime_error_handler = 0,
        .user_alloc_obj = 0,
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

void grug_register_game_fn_void_argless(struct grug_state* gst, char const* game_fn_name, game_fn_void_argless fn) {
    (void)gst;
    (void)game_fn_name;
    (void)fn;
    // TODO: implement
}

void grug_register_game_fn_value_argless(struct grug_state* gst, char const* game_fn_name, game_fn_value_argless fn) {
    (void)gst;
    (void)game_fn_name;
    (void)fn;
    // TODO: implement
}

void grug_register_game_fn_void(struct grug_state* gst, char const* game_fn_name, game_fn_void fn) {
    (void)gst;
    (void)game_fn_name;
    (void)fn;
    // TODO: implement
}

void grug_register_game_fn_value(struct grug_state* gst, char const* game_fn_name, game_fn_value fn) {
    (void)gst;
    (void)game_fn_name;
    (void)fn;
    // TODO: implement
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

grug_entity_id grug_create_entity(struct grug_state* gst, grug_file_id script, grug_id id) {
    (void)gst;
    (void)script;
    (void)id;
    // TODO: implement
    return 0;
}

grug_file_id grug_entity_get_file(struct grug_state* gst, grug_entity_id entity) {
    (void)gst;
    (void)entity;
    // TODO: implement
    return 0;
}

grug_id grug_entity_get_id(struct grug_state* gst, grug_entity_id entity) {
    (void)gst;
    (void)entity;
    // TODO: implement
    return 0;
}

void grug_deinit_entity(struct grug_state* gst, grug_entity_id entity) {
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

void grug_backend_call_argless(struct grug_state* gst, grug_on_fn_id fn, grug_entity_id entity) {
    (void)gst;
    (void)fn;
    (void)entity;
    // TODO: implement
}

void grug_backend_call(struct grug_state* gst, grug_on_fn_id fn, grug_entity_id entity, const union grug_value args[]) {
    (void)gst;
    (void)fn;
    (void)entity;
    (void)args;
    // TODO: implement
}

struct grug_ast* grug_to_ast(char const* file_str, size_t file_len) {
    (void)file_str;
    (void)file_len;
    // TODO: implement
    return 0;
}

struct grug_ast* json_to_ast(char const* file_str, size_t file_len) {
    (void)file_str;
    (void)file_len;
    // TODO: implement
    return 0;
}

void grug_free_ast(struct grug_ast* ast) {
    (void) ast;
    // TODO: implement
}

char const* ast_to_json(struct grug_ast* ast, size_t* out_len) {
    (void)ast;
    (void)out_len;
    // TODO: implement
    return 0;
}

char const* ast_to_grug(struct grug_ast* ast, size_t* out_len) {
    (void)ast;
    (void)out_len;
    // TODO: implement
    return 0;
}

void grug_free_string(char const* str, size_t len) {
    (void)str;
    (void)len;
    // TODO: implement
}
