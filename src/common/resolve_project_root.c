#include "common.h"

/* Absolute path of the project root; read directly by the system programs. */
char	project_root[PATH_MAX];

/**
 * @brief Resolves the absolute path of the project root directory.
 *
 * Reads the executable's path via /proc/self/exe. If the binary resides in a
 * "bin" subdirectory (the standalone system programs), navigates one level up
 * so the result is the project root rather than bin/. Falls back to $HOME or
 * "." if the symlink cannot be read.
 *
 * The result is stored in the global `project_root` (used by the system
 * programs) and also returned as a freshly malloc'd string (owned and freed by
 * the shell). Returns NULL on allocation failure.
 */
char	*resolve_project_root(void)
{
	char		temp1[PATH_MAX];
	const char	*home;
	char		*dir;
	ssize_t		n;

	n = readlink("/proc/self/exe", temp1, sizeof(temp1) - 1);
	if (n == -1)
	{
		perror("readlink");
		home = getenv("HOME");
		if (home)
			dir = (char *)home;
		else
			dir = ".";
	}
	else
	{
		temp1[n] = '\0';
		dir = dirname(temp1);
		if (strcmp(basename(dir), "bin") == 0)
			dir = dirname(dir);
	}
	ft_strlcpy(project_root, dir, sizeof(project_root));
	return (ft_strdup(project_root));
}
