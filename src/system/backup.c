#include "system_program.h"

static int	get_project_root(char **project_root);
static int	get_backup_path(char **backup_path, char *project_root);
static int	create_backup(const char *project_root, char *backup_path);
static char	*build_archive_path(const char *project_root, const char *base);
static int	run_backup_tar(char *backup_path, const char *base, const char *archive_path);

/**
 * @brief Archives the path specified by BACKUP_DIR into a timestamped tarball.
 *
 * Reads the BACKUP_DIR environment variable, creates a .tar.gz archive named
 * <basename>_<YYYYMMDD_HHMMSS>.tar.gz in <project_root>/archive/, and forks
 * a child process to run tar.
 *
 * @param argc Number of command-line arguments (unused).
 * @param argv Array of command-line arguments (unused).
 * @return 0 on success, 1 if BACKUP_DIR is unset or a system call fails.
 */
int main(int argc, char **argv) {
	(void)		argc;
	(void)		argv;

	char	*project_root;
	char	*backup_path;
	int		status;

	if (get_project_root(&project_root) == EXIT_FAILURE)
		return (EXIT_FAILURE);
	if (get_backup_path(&backup_path, project_root) == EXIT_FAILURE)
		return (EXIT_FAILURE);

	status = create_backup(project_root, backup_path);

	free(project_root);

	return (status);
}

/**
 * @brief Resolves the project root and ensures its archive directory exists.
 *
 * @param project_root Output pointer set to a newly allocated root path.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE if the archive dir can't be made.
 */
static int	get_project_root(char **project_root)
{
	*project_root = resolve_project_root();

	if (ensure_archive_dir(*project_root) == -1) {
		ft_putstr_fd("Error: Failed to create archive directory\n", 2);
		free(*project_root);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

/**
 * @brief Reads the BACKUP_DIR environment variable.
 *
 * @param backup_path  Output pointer set to the BACKUP_DIR value.
 * @param project_root Project root, freed on failure to avoid a leak.
 * @return EXIT_SUCCESS if BACKUP_DIR is set, EXIT_FAILURE otherwise.
 */
static int	get_backup_path(char **backup_path, char *project_root)
{
	*backup_path = getenv("BACKUP_DIR");
	if (!*backup_path) {
		fprintf(stderr, "BACKUP_DIR not set\n");
		free(project_root);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

/**
 * @brief Builds the archive path and runs tar, reporting the outcome.
 *
 * Owns all error handling for the archiving step: prints the result message
 * and frees the archive path. The caller retains ownership of project_root.
 *
 * @param project_root Project root directory.
 * @param backup_path  Path being archived (modified by dirname() in tar step).
 * @return EXIT_SUCCESS if the tarball was created, EXIT_FAILURE otherwise.
 */
static int	create_backup(const char *project_root, char *backup_path)
{
	char	*archive_path;
	char	*base;

	base = basename(backup_path);
	archive_path = build_archive_path(project_root, base);
	if (!archive_path)
		return (EXIT_FAILURE);

	if (run_backup_tar(backup_path, base, archive_path) == EXIT_FAILURE) {
		fprintf(stderr, "Failed to archive\n");
		free(archive_path);
		return (EXIT_FAILURE);
	}

	printf("Archived %s into %s\n", backup_path, archive_path);
	free(archive_path);
	return (EXIT_SUCCESS);
}

/**
 * @brief Builds the timestamped archive path inside <project_root>/archive/.
 *
 * Formats <project_root>/archive/<base>_<YYYYMMDD_HHMMSS>.tar.gz using the
 * current local time.
 *
 * @param project_root Project root directory.
 * @param base         Basename of the directory being archived.
 * @return A newly allocated path string, or NULL on allocation failure.
 */
static char	*build_archive_path(const char *project_root, const char *base)
{
	time_t		now;
	struct tm	*tm_info;
	char		timebuff[64];
	size_t		len;
	char		*archive_path;

	now = time(NULL);
	tm_info = localtime(&now);
	strftime(timebuff, sizeof(timebuff), "%Y%m%d_%H%M%S", tm_info);

	len = snprintf(NULL, 0, "%s/archive/%s_%s.tar.gz",
			project_root, base, timebuff) + 1;
	archive_path = malloc(len);
	if (!archive_path) {
		perror("malloc");
		return (NULL);
	}
	snprintf(archive_path, len, "%s/archive/%s_%s.tar.gz",
		project_root, base, timebuff);
	return (archive_path);
}

/**
 * @brief Forks a child to create the tarball and waits for it to finish.
 *
 * The child runs `tar -czf <archive_path> -C <dirname> <base>`. Note that
 * dirname() modifies backup_path in place.
 *
 * @param backup_path  Path being archived (modified by dirname()).
 * @param base         Basename of backup_path passed to tar.
 * @param archive_path Destination tarball path.
 * @return EXIT_SUCCESS if tar exited 0, EXIT_FAILURE otherwise.
 */
static int	run_backup_tar(char *backup_path, const char *base,
				const char *archive_path)
{
	pid_t	pid;
	int		status;

	pid = ft_fork();
	if (pid == 0) {
		execlp("tar", "tar", "-czf", archive_path, "-C",
			dirname(backup_path), base, (char *)NULL);
		perror("execlp tar");
		exit(EXIT_FAILURE);
	}
	waitpid(pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return (EXIT_SUCCESS);
	return (EXIT_FAILURE);
}
