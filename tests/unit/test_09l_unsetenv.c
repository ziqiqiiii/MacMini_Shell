/*
 * tests/unit/test_09l_unsetenv.c
 *
 * Unit tests for unset_env() in src/09l_unsetenv.c.
 * unset_env delegates to unset(cmd + 1), so we stub unset to verify.
 *
 * Run with: make unit
 */
#include "unity.h"
#include "minishell.h"

int	g_exit_status = 0;

/* --- cross-file stub: unset from 09e_unset.c --- */

static char	**g_last_unset_keys;
static int	g_unset_return;

int	unset(char **key, t_list **env_list)
{
	(void)env_list;
	g_last_unset_keys = key;
	return (g_unset_return);
}

void	setUp(void)
{
	g_last_unset_keys = NULL;
	g_unset_return = EXIT_SUCCESS;
}

void	tearDown(void) {}

/* --- tests --- */

static void	test_unset_env_skips_cmd_name(void)
{
	char	*cmd[] = {"unsetenv", "FOO", NULL};
	t_list	*env = NULL;

	unset_env(cmd, &env);
	TEST_ASSERT_EQUAL_PTR(cmd + 1, g_last_unset_keys);
	TEST_ASSERT_EQUAL_STRING("FOO", g_last_unset_keys[0]);
}

static void	test_unset_env_returns_unset_success(void)
{
	char	*cmd[] = {"unsetenv", "BAR", NULL};
	t_list	*env = NULL;

	g_unset_return = EXIT_SUCCESS;
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, unset_env(cmd, &env));
}

static void	test_unset_env_returns_unset_failure(void)
{
	char	*cmd[] = {"unsetenv", "BAD", NULL};
	t_list	*env = NULL;

	g_unset_return = EXIT_FAILURE;
	TEST_ASSERT_EQUAL_INT(EXIT_FAILURE, unset_env(cmd, &env));
}

static void	test_unset_env_multiple_args(void)
{
	char	*cmd[] = {"unsetenv", "A", "B", NULL};
	t_list	*env = NULL;

	unset_env(cmd, &env);
	TEST_ASSERT_EQUAL_STRING("A", g_last_unset_keys[0]);
	TEST_ASSERT_EQUAL_STRING("B", g_last_unset_keys[1]);
	TEST_ASSERT_NULL(g_last_unset_keys[2]);
}

int	main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_unset_env_skips_cmd_name);
	RUN_TEST(test_unset_env_returns_unset_success);
	RUN_TEST(test_unset_env_returns_unset_failure);
	RUN_TEST(test_unset_env_multiple_args);
	return UNITY_END();
}
