#include "system_program.h"

/**
 * @brief Appends a spawn event for the dspawn daemon to the log file.
 *
 * Writes a timestamped entry to tmp/dspawn.log recording the current PID.
 */

void daemon_spawn_log(const char *project_root) {
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
 * @brief Registers a daemon by writing its name, PID, and timestamp to the registry.
 *
 * Appends an entry to tmp/daemons.reg. If a daemon with the same name already
 * exists, a numeric suffix (.1, .2, …) is appended to the stored name to
 * avoid collisions.
 *
 * @param name Base name to register for the current daemon process.
 */
void daemon_register(const char *project_root, const char *name) {
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
void daemon_work(const char *project_root) {
        while (1) {
                daemon_log(project_root, "one work cycle");
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

        char *project_root = resolve_project_root();

        spawn_daemon();
        daemon_register(project_root, "dspawnowo");
        daemon_spawn_log(project_root);
        daemon_log(project_root, "test");
        daemon_work(project_root);

        free(project_root);
        return 0;
}
