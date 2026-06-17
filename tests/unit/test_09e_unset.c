/*
 * tests/unit/test_09e_unset.c
 *
 * Unit tests for unset() in src/09e_unset.c.
 *
 * Run with: make unit
 */
#include "unity.h"
#include "minishell.h"

int	g_exit_status = 0;

/* --- cross-file stub: del_data from 12_free.c --- */

void	del_data(void *content)
{
	t_env	*data;

	data = (t_env *)content;
	free(data->key);
	free(data->value);
	free(data);
}

/* --- cross-file stubs for env helpers --- */

int	env_link_list(char **envp, t_list **env_list)
{
	int		i;
	t_env	*content;
	t_list	*node;
	char	*eq;

	i = 0;
	while (envp[i])
	{
		content = ft_calloc(1, sizeof(t_env));
		if (!content)
			return (EXIT_FAILURE);
		eq = ft_strchr(envp[i], '=');
		content->key = ft_substr(envp[i], 0, eq - envp[i]);
		content->value = ft_substr(envp[i], eq - envp[i] + 1,
						ft_strlen(envp[i]) - (eq - envp[i]));
		node = ft_lstnew(content);
		ft_lstadd_back(env_list, node);
		i++;
	}
	return (EXIT_SUCCESS);
}

char	*existed_env(char *key, t_list **env_list)
{
	t_list	*tmp;
	t_env	*data;

	if (!key)
		return (NULL);
	tmp = *env_list;
	while (tmp)
	{
		data = (t_env *)tmp->content;
		if (ft_strncmp(data->key, key, ft_strlen(key) + 1) == 0)
			return (data->value);
		tmp = tmp->next;
	}
	return (NULL);
}

/* --- helpers --- */

static t_list	*g_env;

static void	build_env(char **envp)
{
	env_link_list(envp, &g_env);
}

static int	env_count(void)
{
	int		n;
	t_list	*tmp;

	n = 0;
	tmp = g_env;
	while (tmp)
	{
		n++;
		tmp = tmp->next;
	}
	return (n);
}

void	setUp(void)
{
	g_env = NULL;
}

void	tearDown(void)
{
	ft_lstclear(&g_env, del_data);
}

/* --- tests --- */

static void	test_unset_null_key_returns_success(void)
{
	char	*keys[] = {NULL};

	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, unset(keys, &g_env));
}

static void	test_unset_removes_single_var(void)
{
	char	*envp[] = {"A=1", "B=2", "C=3", NULL};
	char	*keys[] = {"B", NULL};

	build_env(envp);
	TEST_ASSERT_EQUAL_INT(3, env_count());
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(2, env_count());
	TEST_ASSERT_NULL(existed_env("B", &g_env));
}

static void	test_unset_removes_first_var(void)
{
	char	*envp[] = {"A=1", "B=2", NULL};
	char	*keys[] = {"A", NULL};

	build_env(envp);
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(1, env_count());
	TEST_ASSERT_NULL(existed_env("A", &g_env));
	TEST_ASSERT_NOT_NULL(existed_env("B", &g_env));
}

static void	test_unset_removes_last_var(void)
{
	char	*envp[] = {"A=1", "B=2", NULL};
	char	*keys[] = {"B", NULL};

	build_env(envp);
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(1, env_count());
	TEST_ASSERT_NOT_NULL(existed_env("A", &g_env));
}

static void	test_unset_nonexistent_key_is_noop(void)
{
	char	*envp[] = {"A=1", NULL};
	char	*keys[] = {"NOEXIST", NULL};

	build_env(envp);
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(1, env_count());
}

static void	test_unset_multiple_keys(void)
{
	char	*envp[] = {"A=1", "B=2", "C=3", NULL};
	char	*keys[] = {"A", "C", NULL};

	build_env(envp);
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(1, env_count());
	TEST_ASSERT_NOT_NULL(existed_env("B", &g_env));
}

static void	test_unset_all_vars_empties_list(void)
{
	char	*envp[] = {"A=1", NULL};
	char	*keys[] = {"A", NULL};

	build_env(envp);
	unset(keys, &g_env);
	TEST_ASSERT_EQUAL_INT(0, env_count());
}

int	main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_unset_null_key_returns_success);
	RUN_TEST(test_unset_removes_single_var);
	RUN_TEST(test_unset_removes_first_var);
	RUN_TEST(test_unset_removes_last_var);
	RUN_TEST(test_unset_nonexistent_key_is_noop);
	RUN_TEST(test_unset_multiple_keys);
	RUN_TEST(test_unset_all_vars_empties_list);
	return UNITY_END();
}
