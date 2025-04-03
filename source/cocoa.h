#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * Initializes the Cocoa application by setting up the `NSApplication` shared instance.
 */
void sippetInitializeCocoa(void);

/**
 * Processes events and updates active Cocoa windows.
 */
void sippetUpdateCocoa(void);

/**
 * Terminates the Cocoa application by shutting down the `NSApplication` shared instance.
 */
void sippetTerminateCocoa(void);

/**
 * Holds the necessary data for managing a Cocoa window.
 */
struct SippetCocoaWindow;

/**
 * Creates a `SippetCocoaWindow` given the title, and the initial dimensions and opens a window on
 * the system.
 */
struct SippetCocoaWindow *sippetCreateCocoaWindow(const char *const title, const uint32_t width, const uint32_t height);

/**
 * Fetches the Metal layer used by the Cocoa window. The returned data should be casted back to a
 * `CAMetalLayer`.
 */
void *sippetGetCocoaWindowMetalLayer(const struct SippetCocoaWindow *const cocoaWindow);

/**
 * Checks if a Cocoa window should close (e.g. if the close button on the window gets clicked).
 */
bool sippetCocoaWindowShouldClose(const struct SippetCocoaWindow *const cocoaWindow);

/**
 * Cleans up resources used by `SippetCocoaWindow` and removes the corresponding window from the
 * screen.
 */
void sippetDestroyCocoaWindow(struct SippetCocoaWindow *const cocoaWindow);