#include "system_program.h"
#include <libgen.h>

char project_root[PATH_MAX];

/**
 * @brief Resolves the absolute path of the project root directory.
 *
 * Reads the executable's path via /proc/self/exe and navigates one level up
 * if the binary resides in a "bin" subdirectory. Falls back to $HOME or "."
 * if the symlink cannot be read. Result is stored in the global project_root.
 */
void resolve_project_root(void) {
        printf("resolving project root \n");
        char path[PATH_MAX];
        // should return in the /bin dir, need to back one out
        ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (n == -1) {
                // fallback if unable to get path
                perror("readlink");
                const char *home = getenv("HOME");
                if (home)
                        strncpy(project_root, home, sizeof(project_root));
                else
                        strcpy(project_root, ".");
                return;
        }

        path[n] = '\0';
        char *dir = dirname(path);
        if (strcmp(basename(dir), "bin") == 0)
                strncpy(project_root, dirname(dir), sizeof(project_root) - 1);
        else
                strncpy(project_root, dir, sizeof(project_root) - 1);

        project_root[sizeof(project_root) - 1] = '\0';
        printf("project root path: %s\n", project_root);
}

/**
 * @brief Daemonizes the current process using a double-fork.
 *
 * Performs the standard Unix daemonization sequence: first fork exits the
 * parent, setsid() creates a new session, a second fork prevents the daemon
 * from reacquiring a terminal. Closes all open file descriptors and redirects
 * stdin/stdout/stderr to /dev/null.
 */
void spawn_daemon(void) {
        printf("some kind of daemon spawning program\n");
        printf(
            "spawning a daemon, remember to run ./daemonslayer to kill them\n");
        // first fork
        pid_t pid = fork();
        if (pid < 0)
                exit(EXIT_FAILURE);
        if (pid > 0)
                exit(EXIT_SUCCESS);

        if (setsid() < 0)
                exit(EXIT_FAILURE);

        // disable signals
        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        // second child fork
        pid = fork();
        if (pid < 0)
                exit(EXIT_FAILURE);
        if (pid > 0)
                exit(EXIT_SUCCESS);

        umask(0); // allow daemon perms
        // chdir("/"); //change wd to / (always exists)
        // staying in projects root to write to tmp

        // close all fd
        for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
                close(fd);

        // dump stdin to /dev/null
        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd != -1) {
                dup2(null_fd, STDIN_FILENO);
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                if (null_fd > 2)
                        close(null_fd);
        }
}

/**
 * @brief Appends a spawn event for the dspawn daemon to the log file.
 *
 * Writes a timestamped entry to tmp/dspawn.log recording the current PID.
 */
void daemon_spawn_log(void) {
        char log_path[PATH_MAX];
        strncpy(log_path, project_root, sizeof(log_path) - 1);
        strncat(log_path, "/tmp/dspawn.log",
                sizeof(log_path) - strlen(log_path) - 1);

        int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
                perror("log open");
                return;
        }

        time_t now = time(NULL);
        dprintf(fd, "%sStarted dspawn daemon [%d].\n", ctime(&now), getpid());
        close(fd);
}

/**
 * @brief Appends an arbitrary message from the daemon to the log file.
 *
 * Writes a timestamped entry to tmp/dspawn.log containing the current PID
 * and the provided message string.
 *
 * @param msg Message string to record in the log.
 */
void daemon_log(const char *msg) {
        char log_path[PATH_MAX];
        strncpy(log_path, project_root, sizeof(log_path) - 1);
        strncat(log_path, "/tmp/dspawn.log",
                sizeof(log_path) - strlen(log_path) - 1);

        int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
                perror("log open");
                return;
        }

        time_t now = time(NULL);
        dprintf(fd, "%sLogging dspawn daemon [%d] message: %s.\n", ctime(&now),
                getpid(), msg);
        close(fd);
}

/**
 * @brief Registers a daemon by writing its name, PID, and timestamp to the registry.
 *
 * Appends an entry to tmp/daemons.reg. If a daemon with the same name already
 * exists, a numeric suffix (.1, .2, …) is appended to the stored name to
 * avoid collisions.
 *
 * @param name Base name to register for the current daemon process.
 */
void daemon_register(const char *name) {
        char reg_path[PATH_MAX];
        strncpy(reg_path, project_root, sizeof(reg_path) - 1);
        strncat(reg_path, "/tmp/daemons.reg",
                sizeof(reg_path) - strlen(reg_path) - 1);

        // check for daemons created with the same name and add a number to
        // differentiate it
        FILE *fp = fopen(reg_path, "r");
        int count = 0;

        if (fp) {
                char line[1024];
                while (fgets(line, sizeof(line), fp)) {
                        if (strncmp(line, name, strlen(name)) == 0 &&
                            (line[strlen(name)] == ' ' ||
                             line[strlen(name)] == '.'))
                                count++;
                }
                fclose(fp);
        }

        char modified_name[64];
        if (count == 0)
                snprintf(modified_name, sizeof(modified_name), "%s", name);
        else
                snprintf(modified_name, sizeof(modified_name), "%s.%d", name,
                         count);

        // add entry
        int fd = open(reg_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
                perror("daemon register logs open");
                return;
        }

        time_t now = time(NULL);
        char *ts = ctime(&now);
        if (ts)
                ts[strcspn(ts, "\n")] = 0;

        dprintf(fd, "%s %d %s\n", modified_name, getpid(), ts);
        close(fd);
}

/**
 * @brief Main event loop of the dspawn daemon.
 *
 * Runs indefinitely, logging one work-cycle message every 10 seconds.
 */
void daemon_work(void) {
        while (1) {
                daemon_log("one work cycle");
                sleep(10);
        }
}

/**
 * @brief Entry point for the dspawn daemon.
 *
 * Resolves the project root, daemonizes the process, registers itself under
 * the name "dspawnowo", logs the spawn event, and enters the work loop.
 *
 * @param argc Number of command-line arguments (unused).
 * @param argv Array of command-line arguments (unused).
 * @return 0 (never reached in normal operation).
 */
int main(int argc, char **argv) {
        (void)argc;
        (void)argv;

        resolve_project_root();
        spawn_daemon();
        daemon_register("dspawnowo");
        daemon_spawn_log();
        daemon_log("test");
        daemon_work();
        return 0;
}
