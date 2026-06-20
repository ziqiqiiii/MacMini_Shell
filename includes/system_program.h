# ifndef SYSTEM_PROGRAM_H
# define SYSTEM_PROGRAM_H

# include <dirent.h>
# include <errno.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# include <dirent.h>        /* "readdir" etc. are defined here. */
# include <ctype.h>
# include <fcntl.h>
# include <limits.h>
# include <pwd.h>
# include <signal.h>
# include <sys/resource.h>
# include <sys/stat.h>
# include <sys/utsname.h>
# include <syslog.h>
# include <time.h>
# include <libgen.h>
# include <pthread.h>
# include <sys/file.h>
# include <linux/limits.h>

# include "libft.h"
# include "get_next_line.h"
# include "common.h"

# define SHELL_BUFFERSIZE 256
# define SHELL_INPUT_DELIM " \t\r\n\a"
# define SHELL_OPT_DELIM "-"
# define MAX_DAEMONS 64

#endif