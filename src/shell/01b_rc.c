#include "minishell.h"

void	source_rc(t_root *sh, char **envp)
{
	char	rc_path[PATH_MAX];
	char	home_path[PATH_MAX];
	int		fd;

	get_rc_paths(sh, rc_path, home_path);
	// printf("%s \n %s \n", rc_path, home_path);
	fd = -1;
	if (rc_path[0])
		fd = open(rc_path, O_RDONLY);
	if (fd == -1 && home_path[0])
		fd = open(home_path, O_RDONLY);
	if (fd != -1)
	{
		run_rc(fd, sh, envp);
		close(fd);
		return ;
	}
	if (rc_path[0])
		create_empty_rc(rc_path);
}

void	get_rc_paths(t_root *sh, char *rc_path, char *home_path)
{
	const char	*home;

	rc_path[0] = '\0';
	home_path[0] = '\0';
	sh->current_dir = get_current_directory();
	snprintf(rc_path, PATH_MAX, "%s/.macminishellrc", sh->current_dir);
	home = getenv("HOME");
	if (home)
		snprintf(home_path, PATH_MAX, "%s/.macminishellrc", home);
}

void	run_rc(int fd, t_root *sh, char **envp)
{
	char			*line;
	const char		*value;
	size_t			len;
	t_rc_line_type	type;

	line = get_next_line(fd);
	while (line)
	{
		len = ft_strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		if (len > 1 && line[len - 2] == '\r')
			line[len - 2] = '\0';
		type = classify_rc_line(line, &value);
		if (type == RC_LINE_PATH)
			set_path(sh, value);
		else if (type == RC_LINE_COMMAND)
			exec_rc_cmd(sh, envp, (char *)value);
		free(line);
		line = get_next_line(fd);
	}
}



/**
 * @brief Creates an empty .minishellrc file with a starter comment header.
 *
 * Uses O_EXCL so the file is only created if it does not already exist;
 * silently returns if the file is already present.
 *
 * @param path Absolute file path at which to create the rc file.
 * @see https://github.com/natalieagus/C-Shell-custom/tree/master
 */
void	create_empty_rc(const char *path)
{
	int			fd;
	const char	*header;

	fd = ft_open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
	header = "# MacMini Shell start-up file\n"
		"# Add one command per line; blank lines and # comments"
		" are ignored.\n"
		"\n# Manifesting Mac Mini\n";
	write(fd, header, ft_strlen(header));
	close(fd);
}
