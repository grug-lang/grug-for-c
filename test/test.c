// Nasty, Disgusting, Evil macro tomfoolery
#include "grug_main.h"
#define grug_value test_grug_value
#define game_fn test_game_fn
#define grug_number test_grug_number
#define grug_bool test_grug_bool
#define grug_string test_grug_string
#define grug_id test_grug_id

#include <tests.h>

#undef grug_value
#undef game_fn
#undef grug_number
#undef grug_bool
#undef grug_string
#undef grug_id

// that wasn't enough, grug tests has more name collisions that need to be resolved. They are macros so some care is needed
typedef GRUG_TYPE_BOOL TEST_GRUG_TYPE_BOOL;
typedef GRUG_TYPE_NUMBER TEST_GRUG_TYPE_NUMBER;
typedef GRUG_TYPE_STRING TEST_GRUG_TYPE_STRING;
typedef GRUG_TYPE_ID TEST_GRUG_TYPE_ID;
typedef GRUG_TYPE_ON_FN_ID TEST_GRUG_TYPE_ON_FN_ID;

#undef GRUG_TYPE_BOOL
#undef GRUG_TYPE_NUMBER
#undef GRUG_TYPE_STRING
#undef GRUG_TYPE_ID
#undef GRUG_TYPE_ON_FN_ID

#include <grug.h>
#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

grug_entity_id g_entity = 0;

struct grug_file_id {
	grug_file_id id;
	struct grug_file_id* pnext;
};

struct grug_file_id* g_file_wrappers;

struct grug_file_id* impl_compile_grug_file(struct grug_state* state, const char* file_path, const char** error_out) {
	grug_file_id res = grug_compile_file(state, file_path);
	if(!res) {
		struct grug_error e = grug_get_error(state);
		*error_out = e.message.ptr;
		return 0;
	}
	// TODO: maybe try to see if we already have this id and not allocate a new test wrapper for it every time
	struct grug_file_id* res_ptr = malloc(sizeof(grug_file_id));
	res_ptr->id = res;
	res_ptr->pnext = g_file_wrappers;
	g_file_wrappers = res_ptr;
	*error_out = NULL;
	return res_ptr;
}

void impl_init_globals(struct grug_state* state, struct grug_file_id* file_id) {
	// TODO: something feels wrong about this
	if(g_entity) {
		grug_deinit_entity(state, g_entity);
	}
	g_entity = grug_create_entity(state, file_id->id, 4);
}

void impl_call_export_fn(struct grug_state* state, struct grug_file_id* file_id, const char* fn_name, const union test_grug_value* args, size_t args_count) {
	// TODO Um, maybe it would be better to not search the entire export fn database every single time to call a function
	struct grug_on_fns fns = grug_get_fn_ids(state);
	for(size_t index=0; index<fns.count; index += 1) {
		struct grug_on_fn_entry entry = fns.entries[index];
		if(strcmp(entry.on_fn_name.ptr, fn_name) == 0) {
			assert(file_id->id == grug_entity_get_file_id(state, g_entity));
			// The arg unions should be identical.
			// TODO: is this truly the case 100% of the time?
			grug_call_on_function(state, g_entity, entry.id, (union grug_value*) args, args_count);
			return;
		}
	}
}

bool impl_dump_file_to_json(struct grug_state* state, const char *input_grug_path, const char *output_json_path) {
	(void)state;
	size_t grug_len;
	char* grug_contents = read_all_contents(input_grug_path, &grug_len);
	if(!grug_contents) {
		return true;
	}
	struct grug_error maybe_error;
	struct grug_string json = grug_to_json((struct grug_string){.ptr = grug_contents, .len = grug_len}, &maybe_error);
	if(!json.ptr) {
		free(grug_contents);
		return true;
	}
	FILE* out = fopen(output_json_path, "wb");
	if(!out) {
		grug_free_string(json);
		free(grug_contents);
		return true;
	}
	fwrite(json.ptr, 1, json.len, out);
	fclose(out);
	grug_free_string(json);
	free(grug_contents);
	return false;
}

bool impl_generate_file_from_json(struct grug_state* state, const char *input_json_path, const char *output_grug_path) {
	(void)state;
	size_t grug_len;
	char* json_contents = read_all_contents(input_json_path, &grug_len);
	if(!json_contents) {
		return true;
	}
	struct grug_error maybe_error;
	struct grug_string grug = json_to_grug((struct grug_string){.ptr = json_contents, .len = grug_len}, &maybe_error);
	if(!grug.ptr) {
		free(json_contents);
		return true;
	}
	FILE* out = fopen(output_grug_path, "wb");
	if(!out) {
		grug_free_string(grug);
		free(json_contents);
		return true;
	}
	fwrite(grug.ptr, 1, grug.len, out);
	fclose(out);
	grug_free_string(grug);
	free(json_contents);
	return false;
}

void impl_game_fn_error(struct grug_state* state, const char *message) {
	grug_game_fn_runtime_error(state, message);
}

struct test_game_fn_data {
	test_game_fn fn;
	char const* name;
};

static union grug_value test_game_fn_wrapper(struct grug_state* gst, void* fn_data, const union grug_value args[]) {
	struct test_game_fn_data* dat = (struct test_game_fn_data*)fn_data;
	// TODO: is test_grug_value and grug_value always the same?
	union test_grug_value res = dat->fn(gst, (const union test_grug_value*) args);
	// This does not work because C unions are silly
	// return (union grug_value)res;
	union grug_value result_real;
	// TODO: this assumes id is the full size of the union
	result_real._id = res._id;
	return result_real;
}

struct test_game_fn_data game_fn_nothing_dat = {.fn = game_fn_nothing, .name = "game_fn_nothing"};
struct test_game_fn_data game_fn_magic_dat = {.fn = game_fn_magic, .name = "game_fn_magic"};
struct test_game_fn_data game_fn_initialize_dat = {.fn = game_fn_initialize, .name = "game_fn_initialize"};
struct test_game_fn_data game_fn_initialize_bool_dat = {.fn = game_fn_initialize_bool, .name = "game_fn_initialize_bool"};
struct test_game_fn_data game_fn_identity_dat = {.fn = game_fn_identity, .name = "game_fn_identity"};
struct test_game_fn_data game_fn_max_dat = {.fn = game_fn_max, .name = "game_fn_max"};
struct test_game_fn_data game_fn_say_dat = {.fn = game_fn_say, .name = "game_fn_say"};
struct test_game_fn_data game_fn_sin_dat = {.fn = game_fn_sin, .name = "game_fn_sin"};
struct test_game_fn_data game_fn_cos_dat = {.fn = game_fn_cos, .name = "game_fn_cos"};
struct test_game_fn_data game_fn_mega_dat = {.fn = game_fn_mega, .name = "game_fn_mega"};
struct test_game_fn_data game_fn_get_false_dat = {.fn = game_fn_get_false, .name = "game_fn_get_false"};
struct test_game_fn_data game_fn_set_is_happy_dat = {.fn = game_fn_set_is_happy, .name = "game_fn_set_is_happy"};
struct test_game_fn_data game_fn_mega_f32_dat = {.fn = game_fn_mega_f32, .name = "game_fn_mega_f32"};
struct test_game_fn_data game_fn_mega_i32_dat = {.fn = game_fn_mega_i32, .name = "game_fn_mega_i32"};
struct test_game_fn_data game_fn_draw_dat = {.fn = game_fn_draw, .name = "game_fn_draw"};
struct test_game_fn_data game_fn_blocked_alrm_dat = {.fn = game_fn_blocked_alrm, .name = "game_fn_blocked_alrm"};
struct test_game_fn_data game_fn_spawn_dat = {.fn = game_fn_spawn, .name = "game_fn_spawn"};
struct test_game_fn_data game_fn_spawn_d_dat = {.fn = game_fn_spawn_d, .name = "game_fn_spawn_d"};
struct test_game_fn_data game_fn_has_resource_dat = {.fn = game_fn_has_resource, .name = "game_fn_has_resource"};
struct test_game_fn_data game_fn_has_entity_dat = {.fn = game_fn_has_entity, .name = "game_fn_has_entity"};
struct test_game_fn_data game_fn_has_string_dat = {.fn = game_fn_has_string, .name = "game_fn_has_string"};
struct test_game_fn_data game_fn_get_opponent_dat = {.fn = game_fn_get_opponent, .name = "game_fn_get_opponent"};
struct test_game_fn_data game_fn_get_os_dat = {.fn = game_fn_get_os, .name = "game_fn_get_os"};
struct test_game_fn_data game_fn_set_d_dat = {.fn = game_fn_set_d, .name = "game_fn_set_d"};
struct test_game_fn_data game_fn_set_opponent_dat = {.fn = game_fn_set_opponent, .name = "game_fn_set_opponent"};
struct test_game_fn_data game_fn_motherload_dat = {.fn = game_fn_motherload, .name = "game_fn_motherload"};
struct test_game_fn_data game_fn_motherload_subless_dat = {.fn = game_fn_motherload_subless, .name = "game_fn_motherload_subless"};
struct test_game_fn_data game_fn_offset_32_bit_f32_dat = {.fn = game_fn_offset_32_bit_f32, .name = "game_fn_offset_32_bit_f32"};
struct test_game_fn_data game_fn_offset_32_bit_i32_dat = {.fn = game_fn_offset_32_bit_i32, .name = "game_fn_offset_32_bit_i32"};
struct test_game_fn_data game_fn_offset_32_bit_string_dat = {.fn = game_fn_offset_32_bit_string, .name = "game_fn_offset_32_bit_string"};
struct test_game_fn_data game_fn_talk_dat = {.fn = game_fn_talk, .name = "game_fn_talk"};
struct test_game_fn_data game_fn_get_position_dat = {.fn = game_fn_get_position, .name = "game_fn_get_position"};
struct test_game_fn_data game_fn_set_position_dat = {.fn = game_fn_set_position, .name = "game_fn_set_position"};
struct test_game_fn_data game_fn_cause_game_fn_error_dat = {.fn = game_fn_cause_game_fn_error, .name = "game_fn_cause_game_fn_error"};
struct test_game_fn_data game_fn_call_on_b_fn_dat = {.fn = game_fn_call_on_b_fn, .name = "game_fn_call_on_b_fn"};
struct test_game_fn_data game_fn_store_dat = {.fn = game_fn_store, .name = "game_fn_store"};
struct test_game_fn_data game_fn_print_csv_dat = {.fn = game_fn_print_csv, .name = "game_fn_print_csv"};
struct test_game_fn_data game_fn_retrieve_dat = {.fn = game_fn_retrieve, .name = "game_fn_retrieve"};
struct test_game_fn_data game_fn_box_number_dat = {.fn = game_fn_box_number, .name = "game_fn_box_number"};

static void impl_grug_tests_runtime_error_handler(struct grug_state* gst, void* obj) {
	(void)obj;
	struct grug_error e = grug_get_error(gst);

	struct grug_callstack calls = grug_get_callstack(gst);
	
	char const* on_fn_name;
	for(size_t i=1; i <= calls.num_entries; i += 1) {
		if(calls.entries[calls.num_entries - i].type == GRUG_CALLSTACK_ENTRY_TYPE_ON_FN) {
			on_fn_name = calls.entries[calls.num_entries - 1].fn_name.ptr;
		}
	}

	if(!on_fn_name) {
		on_fn_name = "Unknown";
	}
	
	switch(e.error_type) {
		case GRUG_ERROR_TYPE_NONE: {
			grug_tests_runtime_error_handler("Unknown", GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, "Unknown");
			return;
		}
		case GRUG_ERROR_TYPE_INIT: {
			grug_tests_runtime_error_handler(e.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, "Unknown");
			return;
		}
		case GRUG_ERROR_TYPE_COMPILE: {
			grug_tests_runtime_error_handler(e.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, e.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_STACK_OVERFLOW: {
			grug_tests_runtime_error_handler(e.message.ptr, GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, e.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_TIME_LIMIT_EXCEEDED: {
			grug_tests_runtime_error_handler(e.message.ptr, GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, e.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_GAME_FN_ERROR: {
			grug_tests_runtime_error_handler(e.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, e.file.file_name.ptr);
			return;
		}
		default: {
			assert(false);
		}
	}
}

struct grug_runtime_error_handler error_handler = {
	.drop_fn = 0,
	.handler_fn = impl_grug_tests_runtime_error_handler,
	.user_data = 0,
};

struct grug_state* impl_create_grug_state(const char* mod_api_path, const char* mods_dir) {
	struct grug_init_settings settings  = grug_default_settings();
	settings.mod_api_path = mod_api_path;
	settings.mods_dir_path = mods_dir;
	settings.runtime_error_handler = error_handler;
	struct grug_error e;
	struct grug_state* gst = grug_init(settings, &e);
	if(!gst) {
		fprintf(stderr, "Failed to create state: %s", e.message.ptr);
		grug_free_error(e);
		return 0;
	}
	// Register... well, everything
	grug_register_game_fn(gst, "game_fn_nothing", &game_fn_nothing_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_magic", &game_fn_magic_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_initialize", &game_fn_initialize_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_initialize_bool", &game_fn_initialize_bool_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_identity", &game_fn_identity_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_max", &game_fn_max_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_say", &game_fn_say_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_sin", &game_fn_sin_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_cos", &game_fn_cos_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega", &game_fn_mega_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_false", &game_fn_get_false_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_is_happy", &game_fn_set_is_happy_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega_f32", &game_fn_mega_f32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega_i32", &game_fn_mega_i32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_draw", &game_fn_draw_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_blocked_alrm", &game_fn_blocked_alrm_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_spawn", &game_fn_spawn_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_spawn_d", &game_fn_spawn_d_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_resource", &game_fn_has_resource_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_entity", &game_fn_has_entity_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_string", &game_fn_has_string_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_opponent", &game_fn_get_opponent_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_os", &game_fn_get_os_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_d", &game_fn_set_d_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_opponent", &game_fn_set_opponent_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_motherload", &game_fn_motherload_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_motherload_subless", &game_fn_motherload_subless_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_f32", &game_fn_offset_32_bit_f32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_i32", &game_fn_offset_32_bit_i32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_string", &game_fn_offset_32_bit_string_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_talk", &game_fn_talk_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_position", &game_fn_get_position_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_position", &game_fn_set_position_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_cause_game_fn_error", &game_fn_cause_game_fn_error_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_call_on_b_fn", &game_fn_call_on_b_fn_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_store", &game_fn_store_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_print_csv", &game_fn_print_csv_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_retrieve", &game_fn_retrieve_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_box_number", &game_fn_box_number_dat, test_game_fn_wrapper);
	return gst;
}

void impl_destroy_grug_state(struct grug_state* state) {
	while(g_file_wrappers) {
		struct grug_file_id* next = g_file_wrappers->pnext;
		free(g_file_wrappers);
		g_file_wrappers = next;
	}
	grug_deinit(state);
}

struct grug_state_vtable vtable = {
	.create_grug_state = impl_create_grug_state,
	.destroy_grug_state = impl_destroy_grug_state,
	.compile_grug_file = impl_compile_grug_file,
	.init_globals = impl_init_globals,
	.call_export_fn = impl_call_export_fn,
	.dump_file_to_json = impl_dump_file_to_json,
	.generate_file_from_json = impl_generate_file_from_json,
	.game_fn_error = impl_game_fn_error,
};

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	char const* whitelisted_test = NULL;
	if(argc > 1) {
		whitelisted_test = argv[1];
	}
	char const* tests_dir_path = "grug-tests/tests";
	if(argc > 2) {
		tests_dir_path = argv[2];
	}
	char const* mod_api_path = "grug-tests/mod_api.json";
	if(argc > 3) {
		mod_api_path = argv[3];
	}
	grug_tests_run(tests_dir_path, mod_api_path, vtable, whitelisted_test);
	return 0;
}
