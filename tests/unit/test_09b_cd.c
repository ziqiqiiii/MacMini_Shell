/*
 * tests/unit/test_09b_cd.c
 *
 * Unit tests for cd() in src/09b_cd.c.
 *
 * Run with: make unit
 */
#include "unity.h"
#include "minishell.h"

int	g_exit_status = 0;

/* --- cross-file stub: existed_env from 09f_env.c --- */

static char	*g_stub_home;

char	*existed_env(char *key, t_list **env_list)
{
	(void)env_list;
	if (key && ft_strncmp(key, "HOME", 5) == 0)
		return (g_stub_home);
	return (NULL);
}

/* --- helpers --- */

static char	g_orig_dir[PATH_MAX];

void	setUp(void)
{
	getcwd(g_orig_dir, sizeof(g_orig_dir));
	g_stub_home = getenv("HOME");
}

void	tearDown(void)
{
	chdir(g_orig_dir);
}

static int	capture_stderr_cd(char **cmd, char *buf, size_t size,
	t_list **env)
{
	int		pipefd[2];
	int		saved;
	ssize_t	n;

	if (pipe(pipefd) != 0)
		return (-1);
	saved = dup(STDERR_FILENO);
	dup2(pipefd[1], STDERR_FILENO);
	close(pipefd[1]);
	cd(cmd, env);
	dup2(saved, STDERR_FILENO);
	close(saved);
	n = read(pipefd[0], buf, size - 1);
	close(pipefd[0]);
	if (n < 0)
		return (-1);
	buf[n] = '\0';
	return ((int)n);
}

/* --- tests --- */

static void	test_cd_no_args_goes_home(void)
{
	char	*cmd[] = {"cd", NULL};
	t_list	*env = NULL;
	char	cwd[PATH_MAX];

	if (!g_stub_home)
		TEST_IGNORE_MESSAGE("HOME not set");
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, cd(cmd, &env));
	getcwd(cwd, sizeof(cwd));
	TEST_ASSERT_EQUAL_STRING(g_stub_home, cwd);
}

static void	test_cd_no_args_home_unset_fails(void)
{
	char	*cmd[] = {"cd", NULL};
	t_list	*env = NULL;
	char	buf[256];
	int		pipefd[2];
	int		saved;
	int		ret;

	g_stub_home = NULL;
	pipe(pipefd);
	saved = dup(STDERR_FILENO);
	dup2(pipefd[1], STDERR_FILENO);
	close(pipefd[1]);
	ret = cd(cmd, &env);
	dup2(saved, STDERR_FILENO);
	close(saved);
	read(pipefd[0], buf, sizeof(buf));
	close(pipefd[0]);
	TEST_ASSERT_EQUAL_INT(EXIT_FAILURE, ret);
}

static void	test_cd_valid_path(void)
{
	char	*cmd[] = {"cd", "/tmp", NULL};
	t_list	*env = NULL;
	char	cwd[PATH_MAX];

	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, cd(cmd, &env));
	getcwd(cwd, sizeof(cwd));
	TEST_ASSERT_EQUAL_STRING("/tmp", cwd);
}

static void	test_cd_invalid_path_fails(void)
{
	char	*cmd[] = {"cd", "/nonexistent_dir_12345", NULL};
	t_list	*env = NULL;
	char	buf[256];

	capture_stderr_cd(cmd, buf, sizeof(buf), &env);
	/* perror prints "cd: : No such file or directory" */
}

static void	test_cd_invalid_path_returns_failure(void)
{
	char	*cmd[] = {"cd", "/nonexistent_dir_12345", NULL};
	t_list	*env = NULL;
	int		pipefd[2];
	int		saved;
	int		ret;

	pipe(pipefd);
	saved = dup(STDERR_FILENO);
	dup2(pipefd[1], STDERR_FILENO);
	close(pipefd[1]);
	ret = cd(cmd, &env);
	dup2(saved, STDERR_FILENO);
	close(saved);
	close(pipefd[0]);
	TEST_ASSERT_EQUAL_INT(EXIT_FAILURE, ret);
}

static void	test_cd_tilde_expands_to_home(void)
{
	char	*tilde;
	char	*cmd[3];
	t_list	*env = NULL;
	char	cwd[PATH_MAX];

	if (!g_stub_home)
		TEST_IGNORE_MESSAGE("HOME not set");
	tilde = ft_strdup("~");
	cmd[0] = "cd";
	cmd[1] = tilde;
	cmd[2] = NULL;
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, cd(cmd, &env));
	getcwd(cwd, sizeof(cwd));
	TEST_ASSERT_EQUAL_STRING(g_stub_home, cwd);
	/* tilde was freed/replaced by tilda_helper — don't free again */
}

int	main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_cd_no_args_goes_home);
	RUN_TEST(test_cd_no_args_home_unset_fails);
	RUN_TEST(test_cd_valid_path);
	RUN_TEST(test_cd_invalid_path_fails);
	RUN_TEST(test_cd_invalid_path_returns_failure);
	RUN_TEST(test_cd_tilde_expands_to_home);
	return UNITY_END();
}
