#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include <engine/graphics.h>

typedef struct
{
   SDL_Window *window;
   GraphicsBackend *graphics_backend;
} State;

static bool update(State *state);
static void terminate(State *state, int exit_code);

int main()
{
    State state = {};
    state.window = NULL;
    state.graphics_backend = NULL;

    if (!SDL_Init(SDL_INIT_VIDEO)) terminate(&state, EXIT_FAILURE);

    state.graphics_backend = graphics_backend_create();
    if (!state.graphics_backend) terminate(&state, EXIT_FAILURE);
    
    state.window = SDL_CreateWindow("SippetBox", 640, 480, SDL_WINDOW_VULKAN);
    if (!state.window)
    {
        printf("ERROR: Failed to create window: %s\n", SDL_GetError());
        terminate(&state, EXIT_FAILURE);
    }

    // This will keep waiting for the `update()` function to return `false`
    while (update(&state));
    
    terminate(&state, EXIT_SUCCESS);
}

static bool update(State *state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
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
    if (state->graphics_backend)
    {
        graphics_backend_destroy(state->graphics_backend);
    }

    if (state->window)
    {
        SDL_DestroyWindow(state->window);
    }

    SDL_Quit();
    exit(exit_code);
}