#include "minishell.h"


/**
 * @brief Calculates the length of a 2D array of strings.
 *
 * @param str Pointer to the first element of a null-terminated array of strings.
 * @return The number of strings in the array.
 */
int	array2d_len(char **str)
{
	int	i;

	i = 0;
	while (str[i])
		i++;
	return (i);
}

/**
 * @brief Converts a string to lowercase in-place.
 *
 * This function modifies the given string by converting all uppercase 
 * characters to lowercase.
 *
 * @param str Pointer to the string to be modified.
 */
void	str_to_lower(char **str)
{
	int		i;

	i = 0;
	while ((*str)[i])
	{
		if ((*str)[i] >= 'A' && (*str)[i] <= 'Z')
			(*str)[i] += 32;
		i++;
	}
}

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

char		*get_current_directory(void)
{
	char		temp1[PATH_MAX];
	char		*temp2;
	char		*path;
	ssize_t		n;

	n = readlink("/proc/self/exe", temp1, sizeof(temp1) - 1);
	if (n == -1)
		return (NULL);
	temp1[n] = '\0';
	temp2 = dirname(temp1);
	n = strlen(temp2) + 1;
	path = malloc(sizeof(char) * n);
	if (!path)
		return (NULL);
	ft_strlcpy(path, temp2, n);
	return (path);
}
