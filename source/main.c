#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <engine/context.h>
#include <engine/window.h>

typedef struct
{
    Context *context;
    Window *window;
} State;

static bool update(State *state);
static void terminate(State *state, int exit_code);

int main()
{
    State state = {};
    state.context = NULL;
    state.window = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        fprintf(stderr, "ERROR: Failed to initialize SDL\n");
        terminate(&state, EXIT_FAILURE);
    }

    if (SDL_Vulkan_LoadLibrary(NULL) == false)
    {
        fprintf(stderr, "ERROR: Failed to load Vulkan Library with SDL\n");
        terminate(&state, EXIT_FAILURE);
    }

    state.context = context_create();
    if (state.context == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create graphics backend instance\n");
        terminate(&state, EXIT_FAILURE);
    }

    state.window = window_create("SippetBox", 640, 480);
    if (state.window == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create window instance\n");
        terminate(&state, EXIT_FAILURE);
    }

    // This will keep waiting for the `update()` function to return `false`
    while (update(&state) == true);

    terminate(&state, EXIT_SUCCESS);
}

static bool update(State *state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) == true)
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            return false;
        }
    }

    return true;
}

static void terminate(State *state, int exit_code)
{
    window_destroy(state->window);
    context_destroy(state->context);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    exit(exit_code);
}