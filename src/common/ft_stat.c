#include "common.h"

// -1 -> exits, but it's not dir
// 0 -> exist and it's dir
// 1 -> do not exist 
int	ft_stat(const char *path)
{
	struct stat	st;

	if (stat(path, &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
			return (0);
		ft_putstr_fd("Error: It's not dir\n", 2);
		return (-1);
	}
	return (1);
}
