#include "minishell.h"




t_rc_line_type	classify_rc_line(const char *line, const char **value_out)
{
	const char	*s;

	s = line;
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s == '\0' || *s == '#')
	{
		*value_out = NULL;
		return (RC_LINE_EMPTY);
	}
	if (ft_strncmp(s, "PATH=", 5) == 0)
	{
		*value_out = s + 5;
		return (RC_LINE_PATH);
	}
	*value_out = s;
	return (RC_LINE_COMMAND);
}

void	set_path(t_root *sh, const char *value)
{
	char		new_path[PATH_MAX * 2];
	char		arg[PATH_MAX * 2 + 6];
	const char	*old;
	char		*cmd[3];

	old = getenv("PATH");
	if (old)
		snprintf(new_path, sizeof(new_path), "%s:%s", value, old);
	else
		snprintf(new_path, sizeof(new_path), "%s", value);
	setenv("PATH", new_path, 1);
	snprintf(arg, sizeof(arg), "PATH=%s", new_path);
	cmd[0] = "export";
	cmd[1] = arg;
	cmd[2] = NULL;
	export(cmd, &sh->env_list);
}

void	exec_rc_cmd(t_root *sh, char **envp, char *line)
{
	char	*cmd;
	t_list	*cmd_lexer;
	t_tree	*head;

	cmd = expand(line, &sh->env_list);
	cmd_lexer = lexer(cmd);
	if (cmd_lexer == NULL)
	{
		free(cmd);
		return ;
	}
	head = parser(cmd_lexer, ft_lstsize(cmd_lexer), sh);
	ft_tcsetattr(STDIN_FILENO, TCSANOW, &sh->previous);
	recurse_bst(head, envp, sh);
	reset_data(sh, &cmd_lexer, &head);
	free(cmd);
}
