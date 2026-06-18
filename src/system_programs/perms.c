#include "perms.h"
#include <string.h>

void	perms_to_string(mode_t mode, char str[11])
{
	strcpy(str, "----------");
	if (S_ISDIR(mode))
		str[0] = 'd';
	if (S_ISCHR(mode))
		str[0] = 'c';
	if (S_ISBLK(mode))
		str[0] = 'b';
	if (mode & S_IRUSR)
		str[1] = 'r';
	if (mode & S_IWUSR)
		str[2] = 'w';
	if (mode & S_IXUSR)
		str[3] = 'x';
	if (mode & S_IRGRP)
		str[4] = 'r';
	if (mode & S_IWGRP)
		str[5] = 'w';
	if (mode & S_IXGRP)
		str[6] = 'x';
	if (mode & S_IROTH)
		str[7] = 'r';
	if (mode & S_IWOTH)
		str[8] = 'w';
	if (mode & S_IXOTH)
		str[9] = 'x';
}
