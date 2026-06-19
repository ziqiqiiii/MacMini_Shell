#include "minishell.h"

/**
 * @brief Forks a child process, exiting on failure.
 *
 * Calls fork(2) and terminates the process with an error message if
 * the system call fails.
 *
 * @return The PID of the child process in the parent (0 in the child),
 *         or exits on failure.
 */
int	ft_fork(void)
{
	pid_t	child;

	child = fork();
	if (child < 0)
	{
		printf("fork: %d\n", getpid());
		ft_putstr_fd("Error: Failed to create child process\n", 2);
		exit(EXIT_FAILURE);
	}
	return (child);
}

/**
 * @brief Sends SIGKILL to a stopped child process and waits for it to terminate.
 *
 * Checks if the child indicated by g_exit_status was stopped (WIFSTOPPED).
 * If so, kills it with SIGKILL and waits for the process to fully exit.
 *
 * @param pid The PID of the child process to kill.
 */
void ft_kill(int pid)
{
	if (WIFSTOPPED(g_exit_status))
	{
		kill(pid, SIGKILL);
		waitpid(pid, &g_exit_status, 0);
	}
}


/**
 * @brief Wraps the tcgetattr function, providing error handling.
 *
 * This function retrieves the parameters associated with the 
 * terminal referred to by the given file descriptor and stores
 * them in the termios structure.
 *
 * @param fd The file descriptor of the terminal.
 * @param termios_p Pointer to the termios structure.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int	ft_tcgetattr(int fd, struct termios *termios_p)
{
	if (!isatty(fd))
		return (EXIT_SUCCESS);
	if (tcgetattr(fd, termios_p) == -1)
	{
		perror("tcgetattr failed");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

/**
 * @brief Wraps the tcsetattr function, providing error handling.
 *
 * This function sets the terminal parameters associated with 
 * the terminal referred to by the given file descriptor.
 *
 * @param fd The file descriptor of the terminal.
 * @param optional_actions Specifies when the changes take effect.
 * @param termios_p Pointer to the termios structure.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int	ft_tcsetattr(int fd, int optional_actions, struct termios *termios_p)
{
	if (!isatty(fd))
		return (EXIT_SUCCESS);
	if (tcsetattr(fd, optional_actions, termios_p) == -1)
	{
		perror("tcsetattr failed");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
