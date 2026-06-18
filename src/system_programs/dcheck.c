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
 * @brief Displays the status of all registered daemons.
 *
 * Reads tmp/daemons.reg and prints each entry with its name, PID, start
 * timestamp, and whether the process is still alive (checked via /proc/<pid>).
 * Prints a summary count of active daemons at the end.
 */
void dcheck(void) {
        char reg_path[PATH_MAX];
        strncpy(reg_path, project_root, sizeof(reg_path) - 1);
        strncat(reg_path, "/tmp/daemons.reg",
                sizeof(reg_path) - strlen(reg_path) - 1);

        // check for daemons created with the same name and add a number to
        // differentiate it
        FILE *fd = fopen(reg_path, "r");
        int count = 0;

        if (!fd) {
                perror("dcheck open reg");
                return;
        }
        char line[1024];
        printf("------------------------------------------\n");
        printf("Registered Daemons (%s)\n", "tmp/daemons.reg");
        printf("------------------------------------------\n");

        while (fgets(line, sizeof(line), fd)) {
                char name[128], ts[128];
                int pid;

                if (sscanf(line, "%s %d %[^\n]", name, &pid, ts) != 3)
                        continue;

                char proc_path[128];
                snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
                int check = (access(proc_path, F_OK) == 0);

                if (check)
                        ++count;

                printf("[%c] %-12s \tPID: %-6d %s%s\n", check ? '1' : '-', name,
                       pid, check ? "Started: " : "(inactive)",
                       check ? ts : "");
        }
        fclose(fd);
        printf("------------------------------------------\n");
        printf("Active daemons: %d\n", count);
}

/**
 * @brief Displays all entries in the daemon graveyard.
 *
 * Reads tmp/cematary.reg and prints each buried daemon entry with its name
 * and PID. All entries are shown as inactive. Prints a count of buried daemons
 * at the end.
 */
void dcheck_graveyard(void) {
        char reg_path[PATH_MAX];
        strncpy(reg_path, project_root, sizeof(reg_path) - 1);
        strncat(reg_path, "/tmp/cematary.reg",
                sizeof(reg_path) - strlen(reg_path) - 1);

        // check for daemons created with the same name and add a number to
        // differentiate it
        FILE *fd = fopen(reg_path, "r");
        int count = 0;

        if (!fd) {
                perror("dcheck cematary open reg");
                return;
        }
        char line[1024];
        printf("\n------------------------------------------\n");
        printf("Daemon Cematary (%s)\n", "tmp/cematary.reg");
        printf("------------------------------------------\n");

        while (fgets(line, sizeof(line), fd)) {
                char name[128], ts[128];
                int pid;

                if (sscanf(line, "%s %d %[^\n]", name, &pid, ts) != 3)
                        continue;

                printf("[%c] %-12s \tPID: %-6d %s%s\n", '-', name, pid,
                       "(inactive)", "");

                ++count;
        }
        fclose(fd);
        printf("------------------------------------------\n");
        printf("Daemons burried: %d\n\n", count);
}

/**
 * @brief Entry point for the dcheck utility.
 *
 * Resolves the project root, then prints both the active daemon registry
 * and the graveyard of terminated daemons.
 *
 * @param argc Number of command-line arguments (unused).
 * @param argv Array of command-line arguments (unused).
 * @return 0 on success.
 */
int main(int argc, char **argv) {
        (void)argc;
        (void)argv;

        resolve_project_root();
        dcheck();
        dcheck_graveyard();

        return 0;
}
