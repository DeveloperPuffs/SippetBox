#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <engine/logging.h>
#include <engine/cocoa.h>

@interface ApplicationDelegate:NSObject <NSApplicationDelegate>
@end

@implementation ApplicationDelegate
-(void)applicationDidFinishLaunching:(NSNotification *)notification {
  logDebug("Launched Cocoa application");
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return NO;
}

-(void)applicationWillTerminate:(NSNotification *)notification {
  logDebug("Terminating Cocoa application");
}
@end

void initializeCocoa(void) {
  [NSApplication sharedApplication];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
  [[NSApplication sharedApplication] setDelegate:[[ApplicationDelegate alloc] init]];
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
  [[NSApplication sharedApplication] finishLaunching];
}

void updateCocoa(void) {
  NSEvent *event;
  while ((event = [[NSApplication sharedApplication] nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES]))
    [[NSApplication sharedApplication] sendEvent:event];
  [[NSApplication sharedApplication] updateWindows];
}

void terminateCocoa(void) {
  [[NSApplication sharedApplication] terminate:nil];
}

@interface WindowDelegate: NSObject <NSWindowDelegate>
@property(nonatomic) struct CocoaWindow *window;
-(instancetype)initWithWindow:(struct CocoaWindow *)window;
@end

struct CocoaWindow {
  NSWindow *window;
  WindowDelegate *windowDelegate;
  CAMetalLayer *metalLayer;
  bool shouldClose;
};

@implementation WindowDelegate
-(instancetype)initWithWindow:(struct CocoaWindow *)window {
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

struct CocoaWindow *createCocoaWindow(const char *const title, const uint32_t width, const uint32_t height) {
  struct CocoaWindow *const cocoaWindow = malloc(sizeof(struct CocoaWindow));
  if (!cocoaWindow) return NULL;

  cocoaWindow->window = [
    [NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height)
    styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
    backing:NSBackingStoreBuffered
    defer:NO
  ];

  if (!cocoaWindow->window) return (logError("Failed to create Cocoa window (NSWindow)"), destroyCocoaWindow(cocoaWindow), NULL);

  [cocoaWindow->window setTitle:[NSString stringWithCString:title encoding:NSUTF8StringEncoding]];
  [cocoaWindow->window makeKeyAndOrderFront:nil];
  [cocoaWindow->window center];

  cocoaWindow->windowDelegate = [[WindowDelegate alloc] initWithWindow:cocoaWindow];
  if (!cocoaWindow->windowDelegate) return (logError("Failed to create Cocoa window delegate"), destroyCocoaWindow(cocoaWindow), NULL);

  [cocoaWindow->window setDelegate:cocoaWindow->windowDelegate];

  cocoaWindow->metalLayer = [CAMetalLayer layer];
  cocoaWindow->window.contentView.layer = cocoaWindow->metalLayer;
  cocoaWindow->window.contentView.wantsLayer = YES;

  logDebug("Created Cocoa window");
  return cocoaWindow;
}

void *getCocoaWindowMetalLayer(const struct CocoaWindow *const cocoaWindow) {
  return cocoaWindow ? cocoaWindow->metalLayer : NULL;
}

bool cocoaWindowShouldClose(const struct CocoaWindow *const cocoaWindow) {
  return !cocoaWindow || cocoaWindow->shouldClose;
}

void destroyCocoaWindow(struct CocoaWindow *const cocoaWindow) {
  if (!cocoaWindow) return;
  if (cocoaWindow->window) {
    [cocoaWindow->window orderOut:nil];
    [cocoaWindow->window close];
  }

  [cocoaWindow->windowDelegate release];
  free(cocoaWindow);
}