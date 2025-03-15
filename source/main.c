#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

typedef struct
{
   SDL_Window *window;
} State;

static bool update(State *state);

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        exit(EXIT_FAILURE);
    }

    State state = {};
    
    state.window = SDL_CreateWindow("SippetBox", 640, 480, SDL_WINDOW_VULKAN);
    if (state.window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if (update(&state) == false)
        {
            break;
        }
    }

    SDL_DestroyWindow(state.window);
    SDL_Quit();

    exit(EXIT_SUCCESS);
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