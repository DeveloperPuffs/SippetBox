#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * Initializes the Cocoa application by setting up the `NSApplication` shared instance.
 */
void initializeCocoa(void);

/**
 * Processes events and updates active Cocoa windows.
 */
void updateCocoa(void);

/**
 * Terminates the Cocoa application by shutting down the `NSApplication` shared instance.
 */
void terminateCocoa(void);

/**
 * Holds the necessary data for managing a Cocoa window.
 */
struct CocoaWindow;

/**
 * Creates a `CocoaWindow` given the title, and the initial dimensions and opens a window on the
 * system.
 */
struct CocoaWindow *createCocoaWindow(const char *const title, const uint32_t width, const uint32_t height);

/**
 * Fetches the Metal layer used by the Cocoa window. The returned data should be casted back to a
 * `CAMetalLayer`.
 */
void *getCocoaWindowMetalLayer(const struct CocoaWindow *const cocoaWindow);

/**
 * Checks if a Cocoa window should close (e.g. if the close button on the window gets clicked).
 */
bool cocoaWindowShouldClose(const struct CocoaWindow *const cocoaWindow);

/**
 * Cleans up resources used by `CocoaWindow` and removes the corresponding window from the screen.
 */
void destroyCocoaWindow(struct CocoaWindow *const cocoaWindow);