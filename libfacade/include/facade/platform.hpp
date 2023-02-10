#ifndef __FACADE_PLATFORM_HPP
#define __FACADE_PLATFORM_HPP

/// @file platform.hpp
/// @brief Various macros and defines that help determine the platform of the compiler.
///
/// Doxygen mangles the documentation for this particular file, so this is documented manually.
///
/// ## Macros defined by the make system
///
/// * `LIBFACADE_SHARED`: defined when compiling or importing as a shared object
/// * `LIBFACADE_EXPORT`: defined when compiling as a shared object, explicitly noting that symbols should be exported,
///                       not imported.
///
/// ## Macros defined by the compiler state
///
/// * `LIBFACADE_WIN32`: defined when compiling on MSVC
/// * `EXPORT`: on MSVC, if `LIBFACADE_SHARED` is defined and `LIBFACADE_EXPORT` is defined, this value is
///             `__declspec(dllexport)`; if `LIBFACADE_EXPORT` is not defined, this is defined as
///             `__declspec(dllimport)`. if MSVC is not detected, this is defined as
///             `__attribute__((visibility("default")))`.
/// * `PACK(alignment)`: on MSVC, this evaluates to `__pragma(pack(push, alignment))`. if MSVC is not detected,
///                      this evaluates to `__attribute__((packed,aligned(alignment)))`.
/// * `UNPACK()`: on MSVC, this evaluates to `__pragma(pack(pop))`. if MSVC is not detected, this evaluates to nothing.
///

#if defined(_WIN32) || defined(WIN32)
#define LIBFACADE_WIN32
#endif

#if defined(LIBFACADE_SHARED)

#if defined(LIBFACADE_WIN32)

#if defined(LIBFACADE_EXPORT)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

#else
#define EXPORT __attribute__((visibility("default")))
#endif

#else
#define EXPORT
#endif

#if defined(LIBFACADE_WIN32)
#define PACK(alignment) __pragma(pack(push, alignment))
#define UNPACK() __pragma(pack(pop))
#else
#define PACK(alignment) __attribute__((packed,aligned(alignment)))
#define UNPACK()
#endif

#if defined(LIBFACADE_WIN32)
/* this warning is in relation to a right-shift of 64, which is expected to result in a 0 value. */
#pragma warning( disable: 4293 )

/* this warning is in relation to a zero-sized array within a union, which works fine across the compilers we're targetting. */
#pragma warning( disable: 4200 )
#else
/* this warning is in relation to a right-shift of 64, which is expected to result in a 0 value. */
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif

#endif
