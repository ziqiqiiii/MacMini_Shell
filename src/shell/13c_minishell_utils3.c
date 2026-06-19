#include "minishell.h"

/**
 * @brief Prints the elements of a command array.
 *
 * This function prints the elements of a null-terminated array of strings, 
 * representing command arguments.
 *
 * @param cmd Pointer to the array of command arguments.
 */
void	print_exec_cmd(char **cmd)
{
	int	i;

	i = 0;
	while (cmd[i] != NULL)
	{
		printf("argv[%d]: |%s|\n", i, cmd[i]);
		i++;
	}
}
