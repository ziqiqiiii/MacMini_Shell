#include "system_program.h"
/*
 List all files matching the name in args[1] under current directory and
 subdirectories
*/
/**
 * @brief Recursively searches for files whose names contain a keyword.
 *
 * Starting from the directory in args[0] (or "." when args[0] is "find"),
 * traverses all subdirectories and prints the path of every entry whose name
 * contains the substring in args[1].
 *
 * @param args Array where args[0] is the search root and args[1] is the keyword.
 * @return EXIT_SUCCESS on completion, 1 if the keyword argument is missing or
 *         the directory cannot be opened.
 */
int execute(char **args) {

        if (args[1] == NULL) {
                printf("Usage: find [keyword], to find any matching filename "
                       "in this directory or its children\n");
                return 1;
        }

        char *dir_name = args[0];
        char *to_match = args[1];
        char local_dir_name[SHELL_BUFFERSIZE];

        DIR *d;
        /* Open the directory specified by "dir_name". */
        if (strcmp(dir_name, "find") == 0) {
                d = opendir(".");
                // getcwd(local_dir_name, SHELL_BUFFERSIZE);
                sprintf(local_dir_name, ".");
                dir_name = local_dir_name;
        } else {
                d = opendir(dir_name);
        }

        /* Check it was opened. */
        if (!d) {
                fprintf(stderr, "Cannot open directory '%s': %s\n", dir_name,
                        strerror(errno));
                return 1;
        }

        while (1) {
                struct dirent *entry;
                const char *d_name;

                /* "Readdir" gets subsequent entries from "d". */
                entry = readdir(d);
                if (!entry) {
                        /* There are no more entries in this directory, so break
                           out of the while loop. */
                        break;
                }
                d_name = entry->d_name;

                /* Print the name of the file and directory if it matches the
                 * keyword */
                if (strstr(d_name, to_match) != NULL) {
                        printf("%s/%s\n", dir_name, d_name);
                }

                if (entry->d_type & DT_DIR) {

                        /* Check that the directory is not "d" or d's parent. */
                        if (strcmp(d_name, "..") != 0 &&
                            strcmp(d_name, ".") != 0) {
                                int path_length;
                                char path[PATH_MAX];

                                path_length = snprintf(path, 1024, "%s/%s",
                                                       dir_name, d_name);

                                if (path_length >= PATH_MAX) {
                                        fprintf(
                                            stderr,
                                            "Path length has got too long.\n");
                                        exit(EXIT_FAILURE);
                                }
                                /* Recursively call "list_dir" with the new
                                 * path. */
                                args[0] = path;
                                execute(args);
                        }
                }
        }

        /* After going through all the entries, close the directory. */
        if (closedir(d)) {
                fprintf(stderr, "Could not close '%s': %s\n", dir_name,
                        strerror(errno));
        }

        return EXIT_SUCCESS;
}

/**
 * @brief Entry point for the find utility.
 *
 * Passes the argument vector directly to execute(), which treats args[0] as
 * the command name / starting directory and args[1] as the search keyword.
 *
 * @param argc Number of command-line arguments (unused).
 * @param args Array of command-line arguments forwarded to execute().
 * @return The return value of execute().
 */
int main(int argc, char **args) {
        (void)argc;
        return execute(args);
}
