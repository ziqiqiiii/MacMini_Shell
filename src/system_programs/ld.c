#include "system_program.h"

/**
 * @brief Converts a file mode bitmask into a 10-character permission string.
 *
 * Produces an ls-style string such as "drwxr-xr--". The first character
 * reflects the file type (d, c, b, or -); the remaining nine encode owner,
 * group, and other read/write/execute bits.
 *
 * @param mode File mode bitmask from stat(2).
 * @param str  Output buffer of at least 11 bytes; must be caller-allocated.
 */
void perms_to_string(mode_t mode, char str[11]) {
        strcpy(str, "----------"); // Default to all permissions off

        if (S_ISDIR(mode))
                str[0] = 'd'; // Directory
        if (S_ISCHR(mode))
                str[0] = 'c'; // Character device
        if (S_ISBLK(mode))
                str[0] = 'b'; // Block device

        if (mode & S_IRUSR)
                str[1] = 'r'; // Owner has read permission
        if (mode & S_IWUSR)
                str[2] = 'w'; // Owner has write permission
        if (mode & S_IXUSR)
                str[3] = 'x'; // Owner has execute permission
        if (mode & S_IRGRP)
                str[4] = 'r'; // Group has read permission
        if (mode & S_IWGRP)
                str[5] = 'w'; // Group has write permission
        if (mode & S_IXGRP)
                str[6] = 'x'; // Group has execute permission
        if (mode & S_IROTH)
                str[7] = 'r'; // Others have read permission
        if (mode & S_IWOTH)
                str[8] = 'w'; // Others have write permission
        if (mode & S_IXOTH)
                str[9] = 'x'; // Others have execute permission
}

/**
 * @brief Lists the contents of the current directory with permissions.
 *
 * With no options, prints each non-hidden entry with its permission string
 * color-coded in red (permissions) and green (name). If args[1] is "-r",
 * delegates to the ldr binary for a recursive listing. Any other option
 * prints an error message.
 *
 * @param args Array where args[0] is the command name and args[1] is an
 *             optional flag (e.g., "-r").
 * @return EXIT_SUCCESS on success, 1 if execvp fails or the flag is invalid.
 */
int execute(char **args) {
        if (args[1] != NULL) {
                char *token = strtok(args[1], SHELL_OPT_DELIM);
                // printf("Token is %s\n", token);

                if (token != NULL) {
                        if (strcmp(token, "r") == 0) {
                                // call listdirall,
                                // execvp still need the ./bin because this was
                                // called by a process that was at the ..
                                // directory
                                if (execvp("./bin/ldr", args) == -1) {
                                        perror("Failed to execute, command is "
                                               "invalid.");
                                }
                                return 1;
                        } else {
                                printf("Invalid option. Use -r to display all "
                                       "files within the current directory and "
                                       "its subdirectories.\n");
                                return EXIT_SUCCESS;
                        }
                }
        }

        // print out all the contents of the directory using opendir() function
        DIR *d;
        struct dirent *dir;
        struct stat st;
        char permissions[11];
        d = opendir(".");
        if (d) {
                while ((dir = readdir(d)) != NULL) {
                        // Skip dotfiles
                        if (dir->d_name[0] != '.') {
                                if (stat(dir->d_name, &st) == 0) {
                                        perms_to_string(st.st_mode,
                                                        permissions);
                                        printf(COLOR_RED "%s " COLOR_GREEN
                                                         "%s\n" COLOR_RESET,
                                               permissions, dir->d_name);
                                } else {
                                        perror("stat failed");
                                }
                        }
                }
                closedir(d);
        } else {
                printf("Directory doesn't exist. \n");
        }

        return EXIT_SUCCESS;
}

/**
 * @brief Entry point for the ld utility.
 *
 * Passes the argument vector directly to execute() for directory listing.
 *
 * @param argc Number of command-line arguments (unused).
 * @param args Array of command-line arguments forwarded to execute().
 * @return The return value of execute().
 */
int main(int argc, char **args) {
        (void)argc;
        return execute(args);
}
