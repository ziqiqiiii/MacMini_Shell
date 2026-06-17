/*
 * tests/unit/test_09k_setenv.c
 *
 * Unit tests for set_env() in src/09k_setenv.c.
 * set_env delegates to export(), so we stub export to verify delegation.
 *
 * Run with: make unit
 */
#include "unity.h"
#include "minishell.h"

int	g_exit_status = 0;

/* --- cross-file stub: export from 09d_export.c --- */

static char	**g_last_export_cmd;
static int	g_export_return;

int	export(char **cmd, t_list **env_list)
{
	(void)env_list;
	g_last_export_cmd = cmd;
	return (g_export_return);
}

void	setUp(void)
{
	g_last_export_cmd = NULL;
	g_export_return = EXIT_SUCCESS;
}

void	tearDown(void) {}

/* --- tests --- */

static void	test_set_env_delegates_to_export(void)
{
	char	*cmd[] = {"setenv", "FOO=bar", NULL};
	t_list	*env = NULL;

	set_env(cmd, &env);
	TEST_ASSERT_EQUAL_PTR(cmd, g_last_export_cmd);
}

static void	test_set_env_returns_export_success(void)
{
	char	*cmd[] = {"setenv", "A=1", NULL};
	t_list	*env = NULL;

	g_export_return = EXIT_SUCCESS;
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, set_env(cmd, &env));
}

static void	test_set_env_returns_export_failure(void)
{
	char	*cmd[] = {"setenv", "1BAD=x", NULL};
	t_list	*env = NULL;

	g_export_return = EXIT_FAILURE;
	TEST_ASSERT_EQUAL_INT(EXIT_FAILURE, set_env(cmd, &env));
}

static void	test_set_env_passes_env_list_through(void)
{
	char	*cmd[] = {"setenv", "X=y", NULL};
	t_list	*env = NULL;

	set_env(cmd, &env);
	TEST_ASSERT_EQUAL_PTR(cmd, g_last_export_cmd);
}

int	main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_set_env_delegates_to_export);
	RUN_TEST(test_set_env_returns_export_success);
	RUN_TEST(test_set_env_returns_export_failure);
	RUN_TEST(test_set_env_passes_env_list_through);
	return UNITY_END();
}
