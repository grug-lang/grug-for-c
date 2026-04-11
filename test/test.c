// Nasty, Disgusting, Evil macro tomfoolery
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
	// TODO: it has just occurred to me that error-checking the grug file in terms of mod_api usage requires the state.
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
	// TODO: it has just occurred to me that error-checking the grug file in terms of mod_api usage requires the state.
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
	// TODO
	(void)state;
	(void)message;
}

struct grug_state* impl_create_grug_state(const char* mod_api_path, const char* mods_dir) {
	struct grug_init_settings settings  = grug_default_settings();
	settings.mod_api_path = mod_api_path;
	settings.mods_dir_path = mods_dir;
	// TODO: handle error
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
