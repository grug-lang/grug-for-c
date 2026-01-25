#include <stdio.h>
#include <inttypes.h>

#include <grug.h>

// Game fns get direct access to the grug state / context from which they are called
// For example, in a system with co-routines, each fiber may have its own grug state.
void game_fn_print_string(struct grug_state* gst, grug_id me_caller, const union grug_value args[]) {
    (void)gst;
    printf("Entity %" PRIu64 " said %s\n", me_caller, args[0]._string);
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
    grug_on_fn_id on_spawn_fn_id = grug_get_fn_id(gst, "Dog", "on_spawn");
    grug_on_fn_id on_bark_fn_id = grug_get_fn_id(gst, "Dog", "on_bark");

    // your file object is simple a handle to the script, and isn't the script itself 
    grug_file_id labrador_script = grug_get_script(gst, "animals/labrador-Dog.grug");

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
            grug_file_id updated_file = updates.updates[i].file;
            if(updated_file == labrador_script) {
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
