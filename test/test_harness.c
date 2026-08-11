// Nasty, Disgusting, Evil tomfoolery
// This first set adds `test_` prefix to all of the types in the tests.h header
#define grug_value test_grug_value
#define game_fn test_game_fn
#define grug_number test_grug_number
#define grug_bool test_grug_bool
#define grug_string test_grug_string
#define grug_id test_grug_id
#define grug_type test_grug_type

// This second set guarantees the types match, but they also need undefs later since grug-for-c uses those names for something else
#define GRUG_TYPE_NUMBER double
#define GRUG_TYPE_BOOL bool
#define GRUG_TYPE_STRING const char*
#define GRUG_TYPE_ID uint64_t
#define GRUG_TYPE_ON_FN_ID uint64_t

#include <tests.h>

// Undef everything to remove the name collisions
#undef grug_value
#undef game_fn
#undef grug_number
#undef grug_bool
#undef grug_string
#undef grug_id
#undef grug_type

#undef GRUG_TYPE_NUMBER
#undef GRUG_TYPE_BOOL
#undef GRUG_TYPE_STRING
#undef GRUG_TYPE_ID
#undef GRUG_TYPE_ON_FN_ID

#include <grug_main.h>

#undef GRUG_TYPE_BOOL
#undef GRUG_TYPE_NUMBER
#undef GRUG_TYPE_STRING
#undef GRUG_TYPE_ID
#undef GRUG_TYPE_ON_FN_ID

#include <alloca.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO(bluesillybeard): isn't this function already implemented somewhere else? Maybe it should be moved to a file utilities somewhere.
static char* read_all_contents(char const* file_path, size_t* out_len) {
	struct block {
		char data[1024];
		size_t data_len;
		struct block* pnext;
	};

	struct block* first = 0;
	struct block* last = 0;
	size_t total_size = 0;

	FILE* file = fopen(file_path, "rb");

	if(!file) {
		if(out_len) {
			*out_len = 0;
		}
		return NULL;
	}

	first = malloc(sizeof(struct block));

	first->data_len = fread(first->data, 1, 1024, file);
	total_size += first->data_len;

	last = first;

	while(!feof(file)) {
		struct block* new = malloc(sizeof(struct block));
		last->pnext = new;
		last = new;
		last->data_len = fread(last->data, 1, 1024, file);
		total_size += last->data_len;
	}

	(void)fclose(file);

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

struct grug_file_id {
	grug_file_id id;
	struct grug_file_id* pnext;
};

struct grug_file_id* g_file_wrappers; // NOLINT: this cannot be const

struct grug_file_id* impl_compile_grug_file(struct grug_state* state, const char* file_path, const char** error_out) {
	grug_file_id res = grug_compile_file(state, file_path);
	if(!res) {
		struct grug_error error = grug_get_error(state);
		*error_out = error.message.ptr;
		return 0;
	}
	// TODO(bluesillybeard): maybe try to see if we already have this id and not allocate a new test wrapper for it every time
	struct grug_file_id* res_ptr = malloc(sizeof(struct grug_file_id));
	res_ptr->id = res;
	res_ptr->pnext = g_file_wrappers;
	g_file_wrappers = res_ptr;
	*error_out = NULL;
	return res_ptr;
}

void impl_destroy_grug_file(struct grug_state* state, struct grug_file_id* file) {
	(void)state;
	(void)file;
	// TODO(bluesillybeard): implement
}

struct grug_entity_id* impl_create_entity(struct grug_state* state, struct grug_file_id* file, const char** error_out) {
	// TODO(bluesillybeard): implement
	(void)state;
	(void)file;
	(void)error_out;
	return NULL;
}

void impl_destroy_entity(struct grug_state* state, struct grug_entity_id* entity) {
	// TODO(bluesillybeard): implement
	(void)state;
	(void)entity;
}

void impl_update(struct grug_state* state, const char** error_out) {
	(void)state;
	(void)error_out;
	// TODO(bluesillybeard): implement
}

void impl_call_export_fn(struct grug_state* state, struct grug_entity_id* entity, const char* fn_name, const union test_grug_value* args, size_t args_count) {
	grug_entity_id entity_id = *((grug_entity_id*)entity);
	// TODO(bluesillybeard): maybe it would be better to not search the entire export fn database every single time to call a function
	struct grug_on_fns fns = grug_get_fn_ids(state);
	for(size_t index=0; index<fns.count; index += 1) {
		struct grug_on_fn_entry entry = fns.entries[index];
		if(strcmp(entry.on_fn_name.ptr, fn_name) == 0) {
			// The arg unions should be identical.
			grug_call_on_function(state, entity_id, entry.id, (union grug_value*) args, args_count);
			return;
		}
	}
}

bool impl_grug_to_json(struct grug_state* state, const char *input_grug_buffer, char *output_json_buffer, size_t output_buffer_len) {
	bool result = true;
	(void)state;
	struct grug_string input = {
		.ptr = (char*)input_grug_buffer,
		.len = strlen(input_grug_buffer),
	};
	struct grug_error error = {0};
	struct grug_string json = grug_to_json(input, &error);
	if(json.len == 0) {
		result = false;
	}
	size_t size = json.len + 1;
	if(output_buffer_len < size) {
		size = output_buffer_len;
		result = false;
	}

	memcpy(output_json_buffer, json.ptr, size-1);
	output_json_buffer[size-1] = 0;

	grug_free_string(json);
	return result;
}

bool impl_json_to_grug(struct grug_state* state, const char *input_json_buffer, char *output_grug_buffer, size_t output_buffer_len) {
	(void)state;
	bool result = true;
	struct grug_string input = {
		.ptr = (char*)input_json_buffer,
		.len = strlen(input_json_buffer),
	};
	struct grug_error error = {0};
	struct grug_string grug = json_to_grug(input, &error);
	if(grug.len == 0) {
		result = false;
	}
	size_t size = grug.len + 1;
	if(output_buffer_len < size) {
		size = output_buffer_len;
		result = false;
	}

	memcpy(output_grug_buffer, grug.ptr, size-1);
	output_grug_buffer[size-1] = 0;

	grug_free_string(grug);
	return result;
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
	union test_grug_value res = dat->fn(gst, (const union test_grug_value*) args);
	// This does not work because C unions are silly
	// return (union grug_value)res;
	union grug_value result_real;
	assert(sizeof(union grug_value) == sizeof(union test_grug_value));
	memcpy(&result_real, &res, sizeof(union grug_value));
	return result_real;
}

struct test_game_fn_data const game_fn_nothing_dat = {.fn = game_fn_nothing, .name = "game_fn_nothing"};
struct test_game_fn_data const game_fn_magic_dat = {.fn = game_fn_magic, .name = "game_fn_magic"};
struct test_game_fn_data const game_fn_initialize_dat = {.fn = game_fn_initialize, .name = "game_fn_initialize"};
struct test_game_fn_data const game_fn_initialize_bool_dat = {.fn = game_fn_initialize_bool, .name = "game_fn_initialize_bool"};
struct test_game_fn_data const game_fn_identity_dat = {.fn = game_fn_identity, .name = "game_fn_identity"};
struct test_game_fn_data const game_fn_max_dat = {.fn = game_fn_max, .name = "game_fn_max"};
struct test_game_fn_data const game_fn_say_dat = {.fn = game_fn_say, .name = "game_fn_say"};
struct test_game_fn_data const game_fn_sin_dat = {.fn = game_fn_sin, .name = "game_fn_sin"};
struct test_game_fn_data const game_fn_cos_dat = {.fn = game_fn_cos, .name = "game_fn_cos"};
struct test_game_fn_data const game_fn_mega_dat = {.fn = game_fn_mega, .name = "game_fn_mega"};
struct test_game_fn_data const game_fn_get_false_dat = {.fn = game_fn_get_false, .name = "game_fn_get_false"};
struct test_game_fn_data const game_fn_set_is_happy_dat = {.fn = game_fn_set_is_happy, .name = "game_fn_set_is_happy"};
struct test_game_fn_data const game_fn_mega_f32_dat = {.fn = game_fn_mega_f32, .name = "game_fn_mega_f32"};
struct test_game_fn_data const game_fn_mega_i32_dat = {.fn = game_fn_mega_i32, .name = "game_fn_mega_i32"};
struct test_game_fn_data const game_fn_draw_dat = {.fn = game_fn_draw, .name = "game_fn_draw"};
struct test_game_fn_data const game_fn_blocked_alrm_dat = {.fn = game_fn_blocked_alrm, .name = "game_fn_blocked_alrm"};
struct test_game_fn_data const game_fn_spawn_dat = {.fn = game_fn_spawn, .name = "game_fn_spawn"};
struct test_game_fn_data const game_fn_spawn_d_dat = {.fn = game_fn_spawn_d, .name = "game_fn_spawn_d"};
struct test_game_fn_data const game_fn_has_resource_dat = {.fn = game_fn_has_resource, .name = "game_fn_has_resource"};
struct test_game_fn_data const game_fn_has_entity_dat = {.fn = game_fn_has_entity, .name = "game_fn_has_entity"};
struct test_game_fn_data const game_fn_has_string_dat = {.fn = game_fn_has_string, .name = "game_fn_has_string"};
struct test_game_fn_data const game_fn_get_opponent_dat = {.fn = game_fn_get_opponent, .name = "game_fn_get_opponent"};
struct test_game_fn_data const game_fn_get_os_dat = {.fn = game_fn_get_os, .name = "game_fn_get_os"};
struct test_game_fn_data const game_fn_set_d_dat = {.fn = game_fn_set_d, .name = "game_fn_set_d"};
struct test_game_fn_data const game_fn_set_opponent_dat = {.fn = game_fn_set_opponent, .name = "game_fn_set_opponent"};
struct test_game_fn_data const game_fn_motherload_dat = {.fn = game_fn_motherload, .name = "game_fn_motherload"};
struct test_game_fn_data const game_fn_motherload_subless_dat = {.fn = game_fn_motherload_subless, .name = "game_fn_motherload_subless"};
struct test_game_fn_data const game_fn_offset_32_bit_f32_dat = {.fn = game_fn_offset_32_bit_f32, .name = "game_fn_offset_32_bit_f32"};
struct test_game_fn_data const game_fn_offset_32_bit_i32_dat = {.fn = game_fn_offset_32_bit_i32, .name = "game_fn_offset_32_bit_i32"};
struct test_game_fn_data const game_fn_offset_32_bit_string_dat = {.fn = game_fn_offset_32_bit_string, .name = "game_fn_offset_32_bit_string"};
struct test_game_fn_data const game_fn_talk_dat = {.fn = game_fn_talk, .name = "game_fn_talk"};
struct test_game_fn_data const game_fn_get_position_dat = {.fn = game_fn_get_position, .name = "game_fn_get_position"};
struct test_game_fn_data const game_fn_set_position_dat = {.fn = game_fn_set_position, .name = "game_fn_set_position"};
struct test_game_fn_data const game_fn_cause_game_fn_error_dat = {.fn = game_fn_cause_game_fn_error, .name = "game_fn_cause_game_fn_error"};
struct test_game_fn_data const game_fn_call_on_b_fn_dat = {.fn = game_fn_call_on_b_fn, .name = "game_fn_call_on_b_fn"};
struct test_game_fn_data const game_fn_store_dat = {.fn = game_fn_store, .name = "game_fn_store"};
struct test_game_fn_data const game_fn_print_csv_dat = {.fn = game_fn_print_csv, .name = "game_fn_print_csv"};
struct test_game_fn_data const game_fn_retrieve_dat = {.fn = game_fn_retrieve, .name = "game_fn_retrieve"};
struct test_game_fn_data const game_fn_box_number_dat = {.fn = game_fn_box_number, .name = "game_fn_box_number"};

static void impl_grug_tests_runtime_error_handler(struct grug_state* gst, void* obj) {
	(void)obj;
	struct grug_error error = grug_get_error(gst);

	struct grug_callstack calls = grug_get_callstack(gst);
	
	char const* on_fn_name = 0;
	for(size_t i=1; i <= calls.num_entries; i += 1) {
		if(calls.entries[calls.num_entries - i].type == GRUG_CALLSTACK_ENTRY_TYPE_ON_FN) {
			on_fn_name = calls.entries[calls.num_entries - 1].fn_name.ptr;
		}
	}

	if(!on_fn_name) {
		on_fn_name = "Unknown";
	}
	
	switch(error.error_type) {
		case GRUG_ERROR_TYPE_NONE: {
			grug_tests_runtime_error_handler("Unknown", GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, "Unknown");
			return;
		}
		case GRUG_ERROR_TYPE_INIT: {
			grug_tests_runtime_error_handler(error.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, "Unknown");
			return;
		}
		case GRUG_ERROR_TYPE_COMPILE: {
			grug_tests_runtime_error_handler(error.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, error.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_STACK_OVERFLOW: {
			grug_tests_runtime_error_handler(error.message.ptr, GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, error.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_TIME_LIMIT_EXCEEDED: {
			grug_tests_runtime_error_handler(error.message.ptr, GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, error.file.file_name.ptr);
			return;
		}
		case GRUG_ERROR_TYPE_RUNTIME_GAME_FN_ERROR: {
			grug_tests_runtime_error_handler(error.message.ptr, GRUG_ON_FN_GAME_FN_ERROR, on_fn_name, error.file.file_name.ptr);
			return;
		}
		default: {
			assert(false);
		}
	}
}

struct grug_runtime_error_handler const error_handler = {
	.drop_fn = 0,
	.handler_fn = impl_grug_tests_runtime_error_handler,
	.user_data = 0,
};

struct grug_state* impl_create_grug_state(const char* mod_api_path, const char* mods_dir, bool safe_mode) {
	struct grug_init_settings settings  = grug_default_settings();
	settings.mod_api_path = mod_api_path;
	settings.mods_dir_path = mods_dir;
	settings.runtime_error_handler = error_handler;
	struct grug_error error;
	struct grug_state* gst = grug_init(settings, &error);
	if(!gst) {
		(void)fprintf(stderr, "Failed to create state: %s", error.message.ptr);
		grug_free_error(error);
		return 0;
	}
	grug_set_fast_mode(gst, !safe_mode);
	// Register... well, everything
	grug_register_game_fn(gst, "game_fn_nothing", (void*)&game_fn_nothing_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_magic", (void*)&game_fn_magic_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_initialize", (void*)&game_fn_initialize_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_initialize_bool", (void*)&game_fn_initialize_bool_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_identity", (void*)&game_fn_identity_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_max", (void*)&game_fn_max_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_say", (void*)&game_fn_say_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_sin", (void*)&game_fn_sin_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_cos", (void*)&game_fn_cos_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega", (void*)&game_fn_mega_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_false", (void*)&game_fn_get_false_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_is_happy", (void*)&game_fn_set_is_happy_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega_f32", (void*)&game_fn_mega_f32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_mega_i32", (void*)&game_fn_mega_i32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_draw", (void*)&game_fn_draw_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_blocked_alrm", (void*)&game_fn_blocked_alrm_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_spawn", (void*)&game_fn_spawn_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_spawn_d", (void*)&game_fn_spawn_d_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_resource", (void*)&game_fn_has_resource_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_entity", (void*)&game_fn_has_entity_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_has_string", (void*)&game_fn_has_string_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_opponent", (void*)&game_fn_get_opponent_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_os", (void*)&game_fn_get_os_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_d", (void*)&game_fn_set_d_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_opponent", (void*)&game_fn_set_opponent_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_motherload", (void*)&game_fn_motherload_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_motherload_subless", (void*)&game_fn_motherload_subless_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_f32", (void*)&game_fn_offset_32_bit_f32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_i32", (void*)&game_fn_offset_32_bit_i32_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_offset_32_bit_string", (void*)&game_fn_offset_32_bit_string_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_talk", (void*)&game_fn_talk_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_get_position", (void*)&game_fn_get_position_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_set_position", (void*)&game_fn_set_position_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_cause_game_fn_error", (void*)&game_fn_cause_game_fn_error_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_call_on_b_fn", (void*)&game_fn_call_on_b_fn_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_store", (void*)&game_fn_store_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_print_csv", (void*)&game_fn_print_csv_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_retrieve", (void*)&game_fn_retrieve_dat, test_game_fn_wrapper);
	grug_register_game_fn(gst, "game_fn_box_number", (void*)&game_fn_box_number_dat, test_game_fn_wrapper);
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

struct grug_state_vtable const vtable = {
	.create_grug_state = &impl_create_grug_state,
	.destroy_grug_state = &impl_destroy_grug_state,
	.compile_grug_file = &impl_compile_grug_file,
	.destroy_grug_file = &impl_destroy_grug_file,
	.create_entity = &impl_create_entity,
	.destroy_entity = &impl_destroy_entity,
	.update = &impl_update,
	.call_export_fn = &impl_call_export_fn,
	.grug_to_json = &impl_grug_to_json,
	.json_to_grug = &impl_json_to_grug,
	.game_fn_error = &impl_game_fn_error,
};

int main(int argc, char** argv) {
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
