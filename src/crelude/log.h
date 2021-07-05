//! @file log.h
//! Logging given verbosity.
//! You must define a `VERBOSE_VAR` before including this header.
//! Otherwise, the default value is `1`, i.e. only errors & warnings.

#ifndef VERBOSE_VAR
	#define VERBOSE_VAR _verbosity
	static const ifast _verbosity = 1;
#endif

#define ERROR 0
#define WARN  1
#define DEBUG 2
#define INFO  3

#define S_ERROR "  error"
#define S_WARN  "warning"
#define S_DEBUG "  debug"
#define S_INFO  "   info"

#define LOG(LEVEL, FORMATTER, ...) do { \
	if (VERBOSE_VAR >= LEVEL) \
		eprintln("[ " S_##LEVEL " ] log<%s()>: " FORMATTER, __func__, ##__VA_ARGS__); \
} while (0);
