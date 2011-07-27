/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Log defs.
 */
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int	loginit(int usesyslog, char *name);
void	logsyslog(void);
void	logflush(void);
void	info(const char *fmt, ...);
void	warning(const char *fmt, ...);
void	error(const char *fmt, ...);
void	errorc(const char *fmt, ...);
void	fatal(const char *fmt, ...) __attribute__((noreturn));
void	pwarning(const char *fmt, ...);
void	pfatal(const char *fmt, ...) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
