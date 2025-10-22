#ifndef tarnish_common_h
#define tarnish_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NAN_BOXING
// #define DEBUG_PRINT_SCANNING
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#define DEBUG_ASSERTS

#ifdef DEBUG_ASSERTS

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "[%s:%d] Assert failed in %s(): %s\n", __FILE__,         \
              __LINE__, __func__, message);                                    \
      abort();                                                                 \
    }                                                                          \
  } while (false)

#define UNREACHABLE()                                                          \
  do {                                                                         \
    fprintf(stderr, "[%s:%d] This code should not be reached in %s()\n",       \
            __FILE__, __LINE__, __func__);                                     \
    abort();                                                                   \
  } while (false)

#else

#define ASSERT(condition, message)                                             \
  do {                                                                         \
  } while (false)

#if defined(_MSC_VER)
#define UNREACHABLE() __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE()                                                          \
  do {                                                                         \
  } while (false)
#endif

#endif // DEBUG_ASSERTS

#define UINT8_COUNT (UINT8_MAX + 1)

#endif // !tarnish_common_h
