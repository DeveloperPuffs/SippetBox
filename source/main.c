#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <engine/graphics.h>

typedef struct
{
    Context *context;
    Window *window;
    Renderer *renderer;
} State;

static bool update(State *state);
static void terminate(State *state, int exit_code);

int main()
{
    State state = {};
    state.context = NULL;
    state.window = NULL;
    state.renderer = NULL;

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
        fprintf(stderr, "ERROR: Failed to create context instance\n");
        terminate(&state, EXIT_FAILURE);
    }

    state.window = window_create("SippetBox", 640, 480, state.context);
    if (state.window == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create window instance\n");
        terminate(&state, EXIT_FAILURE);
    }

    state.renderer = renderer_create(state.window);
    if (state.renderer == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create renderer instance\n");
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
    renderer_destroy(state->renderer);
    window_destroy(state->window);
    context_destroy(state->context);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    exit(exit_code);
}