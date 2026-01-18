#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <grug.h>

// Game fns get direct access to the grug state / context from which they are called
// For example, in a system with co-routines, each fiber may have its own grug state.
void game_fn_print_string(struct grug_state* gst, GRUG_ID me_caller, const union grug_value args[]) {
    (void)gst;
    printf("Entity %" PRIu64 " said %s\n", me_caller, GRUG_GET_STRING(gst, args, 0));
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
    GRUG_ON_FN_ID on_spawn_fn_id = grug_get_fn_id(gst, "Dog", "on_spawn");
    GRUG_ON_FN_ID on_bark_fn_id = grug_get_fn_id(gst, "Dog", "on_bark");

    // Get access to the script in question.
    // your file object is simple a handle to the script, and isn't the script itself 
    GRUG_FILE_ID labrador_script = grug_get_script(gst, "animals/labrador-Dog.grug");

    // this is the object / entity ID of the dog
    GRUG_ID dog1 = 1;
    // Create space for the script to store its member variables
    void* dog1_members = malloc(grug_members_size(gst, labrador_script));
    // The initialization of members might call game fns, so beware that creating an entity may call game fns
    // the id (dog1 in this case) is stored in the members
    grug_init_members(gst, labrador_script, dog1_members, dog1);
    // tell this dog that it has spawned into the world
    GRUG_CALL_ARGLESS_VOID(gst, on_spawn_fn_id, dog1_members);
    
    GRUG_ID dog2 = 2;
    void* dog2_members = malloc(grug_members_size(gst, labrador_script));
    grug_init_members(gst, labrador_script, dog2_members, dog2);
    GRUG_CALL_ARGLESS_VOID(gst, on_spawn_fn_id, dog2_members);
    
    GRUG_CALL_VOID(gst, on_bark_fn_id, dog1_members, GRUG_ARGS(GRUG_ARG_STRING("Woof")));
    GRUG_CALL_VOID(gst, on_bark_fn_id, dog2_members, GRUG_ARGS(GRUG_ARG_STRING("Arf")));

    while(true) {
        // This reloads any script and resource changes, recompiling files if necessary
        // Since you got IDs instead of the actual structures, grug can update things behind the scenes
        grug_update(gst);
        // but you'll need to make sure to _absolutely never ever forget ever_ to realloc and reinit the globals for all the entities whose script changed
        if(grug_script_was_updated(gst, labrador_script)) {
            // Old members are non-applicable to the new script, members may have been added/removed
            free(dog1_members);
            dog1_members = malloc(grug_members_size(gst, labrador_script));
            grug_init_members(gst, labrador_script, dog1_members, dog1);
            GRUG_CALL_VOID(gst, on_bark_fn_id, dog1_members, GRUG_ARGS(GRUG_ARG_STRING("Woof")));

            free(dog2_members);
            dog2_members = malloc(grug_members_size(gst, labrador_script));
            grug_init_members(gst, labrador_script, dog2_members, dog2);
            GRUG_CALL_VOID(gst, on_bark_fn_id, dog2_members, GRUG_ARGS(GRUG_ARG_STRING("Arf")));
        }
    }

    // Technically unreachable (oops) but this will also clean up all the scripts
    grug_deinit(gst);

    free(dog1_members);
    free(dog2_members);
}
