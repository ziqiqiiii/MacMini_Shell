#include "minishell.h"

static void load_banner(t_root *sh);

/**
 * @brief Prints the banner.
 *
 * This function prints the banner to the standard output.
 */
void	print_banner(t_root *sh)
{
	printf("\n");
	printf("\033[1;31m");
	load_banner(sh);
	printf("\033[0m");
	printf("\n");
	printf("\033[1;32m");
	printf("                    Welcome to MacMini Shell!\n");
	printf("\033[0m");
	printf("\n");
}

static void load_banner(t_root *sh)
{
	int		fd;
	char	*line;
	char	file[1000];

	snprintf(file, 1000, "%s/banner.txt", sh->current_dir);
	fd = ft_open(file, O_RDONLY, 0666);
	line = get_next_line(fd);
	while (line)
	{
		printf("%s", line);
		free(line);
		line = get_next_line(fd);
	}
	printf("\n");
	free(line);
}

