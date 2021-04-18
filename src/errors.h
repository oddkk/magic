#ifndef STAGE_ERROR_H
#define STAGE_ERROR_H

#include "arena.h"
#include "str.h"
#include <stdarg.h>

#define MGC_ERROR_IMMEDIATELY_PRINT 0

typedef unsigned int file_id_t;
#define MGC_FILE_UNKNOWN ((file_id_t)UINT32_MAX)

struct mgc_location {
	size_t byte_from, byte_to;
	unsigned int line_from, col_from;
	unsigned int line_to, col_to;
	file_id_t file_id;
};

struct mgc_location
mgc_loc_combine(struct mgc_location from, struct mgc_location to);

#define MGC_NO_LOC ((struct mgc_location){.file_id=MGC_FILE_UNKNOWN})

struct mgc_location
mgc_loc_file(file_id_t fid);

static inline bool
mgc_loc_defined(struct mgc_location loc)
{
	return loc.file_id != MGC_FILE_UNKNOWN;
}

enum mgc_error_level {
	MGC_ERROR,
	MGC_WARNING,
	MGC_INFO,
	MGC_APPENDAGE,
};

struct mgc_error {
	enum mgc_error_level level;
	struct string msg;
	struct mgc_location loc;
};

struct mgc_error_context {
	struct arena *string_arena;
	struct arena *transient_arena;

	// NOTE: File name ids are offset by one to facilitate having the sentinel
	// for unknown be 0.
	struct string *file_names;
	size_t num_files;

	struct mgc_error *msgs;
	size_t num_msgs;

	size_t num_errors;
	size_t num_warnings;
};

file_id_t
mgc_err_add_file(struct mgc_error_context *, struct string file_name);

void
mgc_msgv(struct mgc_error_context *, struct mgc_location, enum mgc_error_level,
		const char *fmt, va_list);

__attribute__((__format__ (__printf__, 3, 4))) void
mgc_error(struct mgc_error_context *, struct mgc_location, const char *fmt, ...);

__attribute__((__format__ (__printf__, 3, 4))) void
mgc_warning(struct mgc_error_context *, struct mgc_location, const char *fmt, ...);

__attribute__((__format__ (__printf__, 3, 4))) void
mgc_info(struct mgc_error_context *, struct mgc_location, const char *fmt, ...);

__attribute__((__format__ (__printf__, 3, 4))) void
mgc_appendage(struct mgc_error_context *, struct mgc_location, const char *fmt, ...);

void
print_errors(struct mgc_error_context *);

void
mgc_error_destroy(struct mgc_error_context *ctx);

#endif
