/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Log defs.
 */
#include <stdarg.h>

int	loginit(int usesyslog, char *name);
void	info(const char *fmt, ...);
void	warning(const char *fmt, ...);
void	error(const char *fmt, ...);
void	errorc(const char *fmt, ...);
void	fatal(const char *fmt, ...);
void	pwarning(const char *fmt, ...);
void	pfatal(const char *fmt, ...);
