#pragma once

/**
 * Stores a pointer to a SDL window.
 */
typedef struct Window Window;

/**
 * Creates an instance of `Window` and opens and SDL window. Takes the title of the window as a string and the
 * initial dimensions of the window as positive integers.
 */
Window *window_create(const char *title, int initial_width, int initial_height);

/**
 * Destroys an instance of `Window` and closes an SDL window.
 */
void window_destroy(Window *window);