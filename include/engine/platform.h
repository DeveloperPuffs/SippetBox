#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #if defined(_WIN32)
        #define PLATFORM_WINDOWS_WIN32
    #elif defined(_WIN64)
        #define PLATFORM_WINDOWS_WIN64
    #endif
#elif defined(__APPLE__) || defined(__MACH__)
    #define PLATFORM_APPLE
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define PLATFORM_APPLE_MACOS
    #elif TARGET_OS_IPHONE
        #define PLATFORM_APPLE_IOS
    #elif TARGET_OS_TV
        #define PLATFORM_APPLE_TVOS
    #elif TARGET_OS_WATCH
        #define PLATFORM_APPLE_WATCHOS
    #else
        #define PLATFORM_APPLE_UNKNOWN
    #endif
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__unix__)
    #define PLATFORM_UNIX
#elif defined(__ANDROID__)
    #define PLATFORM_ANDROID
#elif defined(__FreeBSD__)
    #define PLATFORM_FREEBSD
#else
    #define PLATFORM_UNKNOWN
#endif