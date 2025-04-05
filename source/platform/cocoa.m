#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <sippet/core/message.h>
#include "cocoa.h"

@interface ApplicationDelegate:NSObject <NSApplicationDelegate>
@end

@implementation ApplicationDelegate
-(void)applicationDidFinishLaunching:(NSNotification *)notification {
  sippetSendMessage(SIPPET_MESSAGE_SEVERITY_DEBUG, "Launched Cocoa application");
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return NO;
}

-(void)applicationWillTerminate:(NSNotification *)notification {
  sippetSendMessage(SIPPET_MESSAGE_SEVERITY_DEBUG, "Terminating Cocoa application");
}
@end

void sippetInitializeCocoa(void) {
  [NSApplication sharedApplication];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
  [[NSApplication sharedApplication] setDelegate:[[ApplicationDelegate alloc] init]];
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
  [[NSApplication sharedApplication] finishLaunching];
}

void sippetUpdateCocoa(void) {
  NSEvent *event;
  while ((event = [[NSApplication sharedApplication] nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES]))
    [[NSApplication sharedApplication] sendEvent:event];
  [[NSApplication sharedApplication] updateWindows];
}

void sippetTerminateCocoa(void) {
  [[NSApplication sharedApplication] terminate:nil];
}

@interface WindowDelegate: NSObject <NSWindowDelegate>
@property(nonatomic) struct SippetCocoaWindow *window;
-(instancetype)initWithWindow:(struct SippetCocoaWindow *)window;
@end

struct SippetCocoaWindow {
  NSWindow *window;
  WindowDelegate *windowDelegate;
  CAMetalLayer *metalLayer;
  bool shouldClose;
};

@implementation WindowDelegate
-(instancetype)initWithWindow:(struct SippetCocoaWindow *)window {
  self = [super init];
  if (!self) return nil;
  self.window = window;
  return self;
}

-(BOOL)windowShouldClose:(NSWindow *)sender {
  if (self.window) self.window->shouldClose = true;
  return NO;
}
@end

struct SippetCocoaWindow *sippetCreateCocoaWindow(const char *const title, const uint32_t width, const uint32_t height) {
  struct SippetCocoaWindow *const cocoaWindow = malloc(sizeof(struct SippetCocoaWindow));
  if (!cocoaWindow) return NULL;

  cocoaWindow->window = [
    [NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height)
    styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
    backing:NSBackingStoreBuffered
    defer:NO
  ];

  if (!cocoaWindow->window) return (sippetSendMessage(SIPPET_MESSAGE_SEVERITY_ERROR, "Failed to create Cocoa window (NSWindow)"), sippetDestroyCocoaWindow(cocoaWindow), NULL);

  [cocoaWindow->window setTitle:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
  [cocoaWindow->window makeKeyAndOrderFront:nil];
  [cocoaWindow->window center];

  cocoaWindow->windowDelegate = [[WindowDelegate alloc] initWithWindow:cocoaWindow];
  if (!cocoaWindow->windowDelegate) return (sippetSendMessage(SIPPET_MESSAGE_SEVERITY_ERROR, "Failed to create Cocoa window delegate"), sippetDestroyCocoaWindow(cocoaWindow), NULL);

  [cocoaWindow->window setDelegate:cocoaWindow->windowDelegate];

  cocoaWindow->metalLayer = [CAMetalLayer layer];
  cocoaWindow->window.contentView.layer = cocoaWindow->metalLayer;
  cocoaWindow->window.contentView.wantsLayer = YES;

  sippetSendMessage(SIPPET_MESSAGE_SEVERITY_DEBUG, "Created Cocoa window");
  return cocoaWindow;
}

void *sippetGetCocoaWindowMetalLayer(const struct SippetCocoaWindow *const cocoaWindow) {
  return cocoaWindow ? cocoaWindow->metalLayer : NULL;
}

bool sippetCocoaWindowShouldClose(const struct SippetCocoaWindow *const cocoaWindow) {
  return !cocoaWindow || cocoaWindow->shouldClose;
}

void sippetDestroyCocoaWindow(struct SippetCocoaWindow *const cocoaWindow) {
  if (!cocoaWindow) return;
  if (cocoaWindow->window) {
    [cocoaWindow->window orderOut:nil];
    [cocoaWindow->window close];
  }

  [cocoaWindow->windowDelegate release];
  free(cocoaWindow);
}