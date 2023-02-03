#ifndef __FACADE_PLATFORM_HPP
#define __FACADE_PLATFORM_HPP

#if defined(_WIN32) || defined(WIN32)
#define LIBFACADE_WIN32
#endif

#if defined(_M_AMD64) || defined(__x86_64__)
#define LIBFACADE_64BIT
#endif

#if defined(LIBFACADE_SHARED)
#if defined(LIBFACADE_WIN32)
#define EXPORT __declspec(dllexport)
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
