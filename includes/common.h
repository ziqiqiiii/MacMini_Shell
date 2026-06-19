#ifndef COMMON_H
# define COMMON_H

# include <sys/stat.h>		/* mode_t, S_ISDIR, S_IRUSR, ... */
# include <limits.h>		/* PATH_MAX */
# include <string.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <libgen.h>
# include <signal.h>
# include <fcntl.h>
# include <time.h>
# include <sys/file.h>
# include <sys/stat.h>

# include "libft.h"

/*
** Helpers shared across the standalone system programs (bin/).
** Linked into each program through libcommon.a.
*/

/* ANSI colour escapes (CL_ prefix avoids clashing with curses' COLOR_*) */
# define CL_RED "\x1b[31m"
# define CL_GREEN "\x1b[32m"
# define CL_YELLOW "\x1b[33m"
# define CL_BLUE "\x1b[34m"
# define CL_MAGENTA "\x1b[35m"
# define CL_CYAN "\x1b[36m"
# define CL_RESET "\x1b[0m"

/* Renders a stat(2) mode into an "ls -l" style permission string. */
void	perms_to_string(mode_t mode, char str[11]);

/* Absolute path of the project root, resolved from /proc/self/exe. */
extern char	project_root[PATH_MAX];
char	*resolve_project_root(void);

/* Daemonization + logging for the long-running system programs. */
void	spawn_daemon(void);
void	daemon_log(const char *msg);

#endif
