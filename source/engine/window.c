#include <stdlib.h>

#include <SDL3/SDL.h>

#include <engine/window.h>

typedef struct Window
{
    SDL_Window *sdl_window_handle;
} Window;

Window *window_create(const char *title, int initial_width, int initial_height)
{
    Window *window = (Window *)calloc(1, sizeof(Window));
    if (window == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for window instance\n");
        return NULL;
    }

    window->sdl_window_handle = SDL_CreateWindow(title, initial_width, initial_height, SDL_WINDOW_VULKAN);
    if (window->sdl_window_handle == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create SDL window: %s\n", SDL_GetError());
        window_destroy(window);
        return NULL;
    }

    return window;
}

void window_destroy(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    SDL_DestroyWindow(window->sdl_window_handle);
    free(window);
}