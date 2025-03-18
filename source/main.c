#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <engine/logging.h>
#include <engine/graphics.h>

typedef struct
{
    Context *context;
    Window *window;
} State;

static int update(State *state);
static void terminate(State *state, int exit_code);

int main()
{
    State state = {};
    state.context = NULL;
    state.window = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        log_error("Failed to initialize SDL: %s", SDL_GetError());
        terminate(&state, EXIT_FAILURE);
    }

    if (SDL_Vulkan_LoadLibrary(NULL) == false)
    {
        log_error("Failed to load Vulkan Library with SDL: %s", SDL_GetError());
        terminate(&state, EXIT_FAILURE);
    }

    state.context = context_create();
    if (state.context == NULL)
    {
        log_error("Failed to create context instance");
        terminate(&state, EXIT_FAILURE);
    }

    state.window = window_create("SippetBox", 640, 480, state.context);
    if (state.window == NULL)
    {
        log_error("Failed to create window instance");
        terminate(&state, EXIT_FAILURE);
    }

    log_debug("Starting program loop ...");

    // This will keep waiting for the `update()` function to return `false`
    while (update(&state) == 1);

    terminate(&state, EXIT_SUCCESS);
}

static void render(void *user_data);

static int update(State *state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) == true)
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            return 0;
        }
    }

    if (window_render_frame(state->window, render, state) == 0)
    {
        log_error("Failed to render frame");
        return 0;
    }

    return 1;
}

static void render(void *user_data)
{
    State *state = (State *)user_data;
    window_render_triangle(state->window);
    return;
}

static void terminate(State *state, int exit_code)
{
    window_destroy(state->window);
    context_destroy(state->context);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    exit(exit_code);
}