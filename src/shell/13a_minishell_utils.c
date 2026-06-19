#include "minishell.h"

/**
 * @brief Creates a pipe, exiting on failure.
 *
 * Calls pipe(2) and terminates the process with an error message if
 * the system call fails.
 *
 * @param p Two-element array to receive the read and write file descriptors.
 * @return The read end file descriptor, or exits on failure.
 */
int	ft_pipe(int p[2])
{
	if (pipe(p) == -1)
	{
		printf("pipe: %d\n", getpid());
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	return (*p);
}

/**
 * @brief Duplicates a file descriptor, exiting on failure.
 *
 * Calls dup2(2) to make old_fd refer to the same file as new_fd.
 * Terminates the process with an error message if the call fails.
 *
 * @param new_fd The source file descriptor to duplicate.
 * @param old_fd The target file descriptor to replace.
 * @return The value of old_fd on success, or exits on failure.
 */
int	ft_dup2(int new_fd, int old_fd)
{
	int	i;

	i = dup2(new_fd, old_fd);
	if (i < 0)
	{
		printf("Error: Failed to dup2\n");
		exit(EXIT_FAILURE);
	}
	return (i);
}

/**
 * @brief Opens a file, exiting on failure.
 *
 * Calls open(2) with the given flags and permission bits. Terminates the
 * process with an error message if the file cannot be opened.
 *
 * @param file Path to the file to open.
 * @param flags Open flags (e.g. O_RDONLY, O_CREAT).
 * @param permission Permission bits applied when creating a new file.
 * @return The file descriptor, or exits on failure.
 */
int	ft_open(const char *file, int flags, int permission)
{
	int	fd;

	fd = open(file, flags, permission);
	if (fd < 0)
	{
		printf("open: %d\n", getpid());
		ft_putstr_fd("Error: Failed to open file\n", 2);
		exit(EXIT_FAILURE);
	}
	return (fd);
}

/**
 * @brief Closes a file descriptor, exiting on failure.
 *
 * Calls close(2) and terminates the process with an error message if
 * the system call fails.
 *
 * @param fd The file descriptor to close.
 * @return 0 on success, or exits on failure.
 */
int	ft_close(int fd)
{
	if (close(fd) < 0)
	{
		printf("close: %d\n", getpid());
		ft_putstr_fd("Error: Failed to close the fd\n", 2);
		exit(EXIT_FAILURE);
	}
	return (0);
}
