/*
 * Log defs.
 */
#include <stdarg.h>

int	ClientLogInit(void);
int	ServerLogInit(void);
void	log(const char *fmt, ...);
void	warning(const char *fmt, ...);
void	fatal(const char *fmt, ...);
void	pwarning(const char *fmt, ...);
void	pfatal(const char *fmt, ...);
