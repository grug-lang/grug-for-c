// Nasty, Disgusting, Evil macro tomfoolery
#include "grug_main.h"
#include <alloca.h>
#include <string.h>
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

grug_entity_id g_entity = 0;

struct grug_file_id* impl_compile_grug_file(struct grug_state* state, const char* file_path, const char** error_out) {
	grug_file_id res = grug_compile_file(state, file_path);
	if(!res) {
		// Bro what in the world, how is it even physically possible that we forgot a function to get non-init compilation errors
		// TODO: fix
		*error_out = "Something went wrong";
		return 0;
	}
	// TODO: This assumes a pointer is big enough to hold the ID, which breaks on anything but 64 bit platforms
	return (void*)(uintptr_t)res;
}

void impl_init_globals(struct grug_state* state, struct grug_file_id* file_id) {
	// TODO: something feels wrong about this
	if(g_entity) {
		grug_deinit_entity(state, g_entity);
	}
	g_entity = grug_create_entity(state, (grug_file_id)(uintptr_t)file_id, 4);
}

void impl_call_export_fn(struct grug_state* state, struct grug_file_id* file_id, const char* fn_name, const union test_grug_value* args, size_t args_count) {
	// TODO Um, maybe it would be better to not search the entire export fn database every single time to call a function
	struct grug_on_fns fns = grug_get_fn_ids(state);
	for(size_t index=0; index<fns.count; index += 1) {
		struct grug_on_fn_entry entry = fns.entries[index];
		if(strcmp(entry.on_fn_name.ptr, fn_name) == 0) {
			// The arg unions should be identical.
			// TODO: is this truly the case 100% of the time?
			grug_call_on_function(state, g_entity, entry.id, (union grug_value*) args, args_count);
		}
	}
	// TODO: assert that this matches the file id that g_entity is an instance of
	(void)file_id;
}

bool impl_dump_file_to_json(struct grug_state* state, const char *input_grug_path, const char *output_json_path) {
	(void)state;
	(void)input_grug_path;
	(void)output_json_path;
	return false;
}

bool impl_generate_file_from_json(struct grug_state* state, const char *input_json_path, const char *output_grug_path) {
	(void)state;
	(void)input_json_path;
	(void)output_grug_path;
	return false;
}

void impl_game_fn_error(struct grug_state* state, const char *message) {
	(void)state;
	(void)message;
}

struct grug_state* impl_create_grug_state(const char* mod_api_path, const char* mods_dir) {
	struct grug_init_settings settings  = grug_default_settings();
	settings.mod_api_path = mod_api_path;
	settings.mods_dir_path = mods_dir;
	struct grug_state* gst = grug_init(settings);
	return gst;
}

void impl_destroy_grug_state(struct grug_state* state) {
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
