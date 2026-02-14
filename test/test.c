#include <stdio.h>
#include <inttypes.h>

#include <grug.h>
#include <string.h>

// Game fns get direct access to the grug state / context from which they are called
// For example, in a system with co-routines, each fiber may have its own grug state.
void game_fn_print_string(struct grug_state* gst, grug_id me_caller, const union grug_value args[]) {
    (void)gst;
    printf("Entity %" PRIu64 " said %s\n", me_caller, args[0]._string);
}

bool find_file(struct grug_mod_dir const* dir, grug_file_id* out_id, char const* name) {
    for(size_t file_index = 0; file_index < dir->files_size; ++file_index) {
        struct grug_file* file = &dir->files[file_index];
        // while looking for file is not the right place to report errors, but they need to be reported somewhere and doing it here works.
        if(file->error_msg) {
            printf("File %s has an error on line %i: %s", file->name, (int)file->error_line_number, file->error_msg);
        }

        if(strcmp(file->name, name)) {
            *out_id = file->id;
            return true;
        }
    }

    for(size_t dir_index = 0; dir_index < dir->mods_size; ++dir_index) {
        if(find_file(dir->mods[dir_index], out_id, name)) {
            return true;
        }
    }

    return false;
}

int main(void) {
    // Default settings (libc malloc/free, mods dir in the cwd, etc)
    struct grug_init_settings settings = grug_default_settings();

    // gst "grug state" contains all of the grug library state
    struct grug_state* gst = grug_init(settings);

    // let grug know where to call the print_string game function
    grug_register_game_fn_void(gst, "print_string", game_fn_print_string);

    // Grab the "ID" of the Dog::on_spawn and Dog::on_bark functions
    // This is not a normal grug object id, but a special function id
    grug_on_fn_id on_spawn_fn_id;
    bool found_on_spawn = false;
    grug_on_fn_id on_bark_fn_id;
    bool found_on_bark = false;
    
    struct grug_on_fns on_fns = grug_get_fn_ids(gst);
    for(size_t index = 0; index < on_fns.count; ++index) {
        struct grug_on_fn_entry entry = on_fns.entries[index];

        if(strcmp(entry.entity_name, "Dog")) {
            if(strcmp(entry.on_fn_name, "on_spawn")) {
                on_spawn_fn_id = entry.id;
                found_on_spawn = true;
            } else if(strcmp(entry.on_fn_name, "on_bark")) {
                on_bark_fn_id = entry.id;
                found_on_bark = true;
            }
        }
    }

    if(!found_on_bark || !found_on_spawn) {
        printf("Failed to find an on fn, is the mod_api.json correct?\n");
        grug_deinit(gst);
        return 1;
    }
    
    // your file object is simple a handle to the script, and isn't the script itself 
    grug_file_id labrador_script;
    bool found_labrador_script = find_file(grug_get_mods(gst), &labrador_script, "labrador-Dog.grug");

    if(!found_labrador_script) {
        printf("Expected a script labrador-Dog to exist");
        grug_deinit(gst);
        return 1;
    }

    // this is the object / entity ID of the dog
    grug_id dog1 = 1;
    // The initialization of members might call game fns, so beware that creating an entity may call game fns
    // grug holds on to the id (dog1) so don't change it without re-creating the entity too.
    grug_entity_id dog1_entity = grug_create_entity(gst, labrador_script, dog1);
    // tell this dog that it has spawned into the world
    GRUG_CALL_ARGLESS(gst, on_spawn_fn_id, dog1_entity);
    
    grug_id dog2 = 2;
    grug_entity_id dog2_entity = grug_create_entity(gst, labrador_script, dog2);
    GRUG_CALL_ARGLESS(gst, on_spawn_fn_id, dog2_entity);
    
    GRUG_CALL(gst, on_bark_fn_id, dog1_entity, GRUG_ARG_STRING("Woof"));
    GRUG_CALL(gst, on_bark_fn_id, dog2_entity, GRUG_ARG_STRING("Arf"));

    while(true) {
        // This reloads any script and resource changes, recompiling files if necessary
        // Since you got IDs instead of the actual structures, grug can update things behind the scenes
        // Note that this also re-inits entity members which may call game fns
        struct grug_updates_list updates = grug_update(gst);

        for(size_t i=0; i<updates.count; ++i) {
            struct grug_file* file = updates.updates[i];
            if(file->error_msg) {
                printf("File %s has an error on line %i: %s", file->name, (int)file->error_line_number, file->error_msg);
            }

            if(file->id == labrador_script) {
                // re-call on_spawn - since the members get reset upon reload.
                GRUG_CALL_ARGLESS(gst, on_spawn_fn_id, dog1_entity);
                GRUG_CALL_ARGLESS(gst, on_spawn_fn_id, dog2_entity);
                
                // call these functions again for demonstration
                GRUG_CALL(gst, on_bark_fn_id, dog1_entity, GRUG_ARG_STRING("Woof"));
                GRUG_CALL(gst, on_bark_fn_id, dog2_entity, GRUG_ARG_STRING("Arf"));
            }
        }
    }

    // Technically unreachable (oops) but this will also clean up all the scripts and entities
    grug_deinit(gst);
}
