#include "common.h"


char	project_root[PATH_MAX];

/**
 * @brief Resolves the absolute path of the project root directory.
 *
 * Reads the executable's path via /proc/self/exe and navigates one level up
 * if the binary resides in a "bin" subdirectory. Falls back to $HOME or "."
 * if the symlink cannot be read. Result is stored in the global project_root.
 */
void	resolve_project_root(void)
{
	char		path[PATH_MAX];
	char		*dir;
	ssize_t		n;
	const char	*home;

	n = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (n == -1)
	{
		perror("readlink");
		home = getenv("HOME");
		if (home)
			strncpy(project_root, home, sizeof(project_root) - 1);
		else
			strcpy(project_root, ".");
		project_root[sizeof(project_root) - 1] = '\0';
		return ;
	}
	path[n] = '\0';
	dir = dirname(path);
	if (strcmp(basename(dir), "bin") == 0)
		strncpy(project_root, dirname(dir), sizeof(project_root) - 1);
	else
		strncpy(project_root, dir, sizeof(project_root) - 1);
	project_root[sizeof(project_root) - 1] = '\0';
}
