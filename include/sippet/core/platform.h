#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #define SIPPET_PLATFORM_WINDOWS
  #if defined(_WIN32)
    #define SIPPET_PLATFORM_WINDOWS_WINDOWS_32
  #elif defined(_WIN64)
    #define SIPPET_PLATFORM_WINDOWS_WINDOWS_64
  #endif
#elif defined(__APPLE__) || defined(__MACH__)
  #define SIPPET_PLATFORM_APPLE
  #include <TargetConditionals.h>
  #if TARGET_OS_MAC
    #define SIPPET_PLATFORM_APPLE_MACOS
  #elif TARGET_OS_IPHONE
    #define SIPPET_PLATFORM_APPLE_IOS
  #elif TARGET_OS_TV
    #define SIPPET_PLATFORM_APPLE_TVOS
  #elif TARGET_OS_WATCH
    #define SIPPET_PLATFORM_APPLE_WATCHOS
  #else
    #define SIPPET_PLATFORM_APPLE_UNKNOWN
  #endif
#elif defined(__linux__)
  #define SIPPET_PLATFORM_LINUX
#else
  #define SIPPET_PLATFORM_UNKNOWN
#endif