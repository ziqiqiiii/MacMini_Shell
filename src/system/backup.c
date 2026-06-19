#include "system_program.h"


/*
void backup(void) {
        char *target = getenv("BACKUP_DIR");
        if (!target) {
                fprintf(stderr, "BACKUP_DIR env not set\n");
                exit(EXIT_FAILURE);
        }

        struct stat st;
        if (stat(target, &st) == -1) {
                perror("stat");
                exit(EXIT_FAILURE);
        }

        int is_dir = S_ISDIR(st.st_mode);
        int is_file = S_ISREG(st.st_mode);
}
*/

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
        (void)argc;
        (void)argv;

        char *project_root = resolve_project_root();

        const char *backup_path = getenv("BACKUP_DIR");
        if (!backup_path) {
                fprintf(stderr, "BACKUP_DIR not set\n");
                free(project_root);
                return 1;
        }

        struct stat st;
        if (stat(backup_path, &st) == -1) {
                perror("stat");
                free(project_root);
                return 1;
        }

        if (ensure_archive_dir(project_root) == -1) {
                fprintf(stderr, "Failed to create archive directory\n");
                free(project_root);
                return 1;
        }

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timebuff[64];
        strftime(timebuff, sizeof(timebuff), "%Y%m%d_%H%M%S", tm_info);

        const char *base = basename((char *)backup_path);

        // char archive_path[PATH_MAX];
        size_t len = snprintf(NULL, 0, "%s/archive/%s_%s.tar.gz", project_root,
                              base, timebuff) +
                     1;

        char *archive_path = malloc(len);
        if (!archive_path) {
                perror("malloc");
                exit(EXIT_FAILURE);
        }
        // snprintf(archive_path, sizeof(archive_path),
        // "%s/archive/%s_%s.tar.gz",
        //          project_root, base, timebuff);
        snprintf(archive_path, len, "%s/archive/%s_%s.tar.gz", project_root,
                 base, timebuff);

        // create a child to make the tarball

        pid_t pid = fork();
        if (pid == 0) {
                execlp("tar", "tar", "-czf", archive_path, "-C",
                       dirname((char *)backup_path), base, (char *)NULL);
                perror("execlp tar");
                exit(1);
        } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
                        printf("Archived %s into %s\n", backup_path,
                               archive_path);
                } else {
                        fprintf(stderr, "Failed to archive\n");
                }
        } else {
                perror("fork tar");
                free(archive_path);
                free(project_root);
                return 1;
        }

        free(archive_path);
        free(project_root);
        return 0;
}
