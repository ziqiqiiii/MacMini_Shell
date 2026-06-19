#include "common.h"

/**
 * @brief Daemonizes the current process using a double-fork.
 *
 * Performs the standard Unix daemonization sequence: first fork exits the
 * parent, setsid() creates a new session, a second fork prevents the daemon
 * from reacquiring a terminal. Closes all open file descriptors and redirects
 * stdin/stdout/stderr to /dev/null.
 */
void	spawn_daemon(void)
{
	pid_t	pid;
	int		fd;
	int		null_fd;

	printf("some kind of daemon spawning program\n");
	printf("spawning a daemon, remember to run ./daemonslayer to kill them\n");
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if (setsid() < 0)
		exit(EXIT_FAILURE);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	umask(0);
	fd = sysconf(_SC_OPEN_MAX);
	while (fd >= 0)
		close(fd--);
	null_fd = open("/dev/null", O_RDWR);
	if (null_fd != -1)
	{
		dup2(null_fd, STDIN_FILENO);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
		if (null_fd > 2)
			close(null_fd);
	}
}

/**
 * @brief Appends a timestamped message to the daemon log file.
 *
 * Writes to <project_root>/tmp/dspawn.log, taking an exclusive flock(2) while
 * appending so concurrent daemons do not interleave their lines.
 *
 * @param project_root Absolute path of the project root.
 * @param msg          Message string to record in the log.
 */
void	daemon_log(const char *project_root, const char *msg)
{
	char	log_path[PATH_MAX];
	int		fd;
	time_t	now;

	strncpy(log_path, project_root, sizeof(log_path) - 1);
	log_path[sizeof(log_path) - 1] = '\0';
	strncat(log_path, "/tmp/dspawn.log",
		sizeof(log_path) - strlen(log_path) - 1);
	fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1)
	{
		perror("log open");
		return ;
	}
	now = time(NULL);
	flock(fd, LOCK_EX);
	dprintf(fd, "%sLogging dspawn daemon [%d] message: %s.\n", ctime(&now),
		getpid(), msg);
	close(fd);
}
