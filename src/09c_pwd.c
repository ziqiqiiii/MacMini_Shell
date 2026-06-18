#include "minishell.h"

/**
 * @brief Prints the current working directory.
 *
 * This function retrieves and prints the current working directory to
 * the standard output using the getcwd system call.
 *
 * @return EXIT_SUCCESS if successful, or EXIT_FAILURE if an error occurs.
 */
int	pwd(void)
{
	char	cwd[256];

	if (get_pwd(cwd))
		return (EXIT_FAILURE);
	else
		printf("%s\n", cwd);
	return (EXIT_SUCCESS);
}

int	get_pwd(char cwd[256])
{
	if (getcwd(cwd, 256) == NULL)
	{
		perror("pwd: ");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
