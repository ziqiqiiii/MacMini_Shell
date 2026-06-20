#include "system_program.h"

static void     daemon_register(const char *project_root, const char *name);
static void     daemon_spawn_log(const char *project_root);
static void     daemon_work(const char *project_root);

int main(int argc, char **argv)
{
	(void)  argc;
	(void)  argv;

	char *project_root = resolve_project_root();

	ensure_daemon_files(project_root);
	daemon_spawn();
	daemon_register(project_root, "deamon_eskimo");
	daemon_spawn_log(project_root);
	daemon_log(project_root, "start of new deamon before deamon work");
	daemon_work(project_root);

	free(project_root);

	return 0;
}

static void daemon_spawn_log(const char *project_root)
{
	char	log_path[PATH_MAX];
	int 	fd;
	time_t	now;

	strncpy(log_path, project_root, sizeof(log_path) - 1);
	strncat(log_path, "/tmp/dspawn.log", sizeof(log_path) - strlen(log_path) - 1);

	fd = ft_open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	now = time(NULL);
	dprintf(fd, "%sStarted dspawn daemon [%d].\n", ctime(&now), getpid());
	
	close(fd);
}

static void daemon_register(const char *project_root, const char *name)
{
	char	reg_path[PATH_MAX];
	FILE	*fp ;
	int		count = 0;
	char	modified_name[64];
	int		fd;
	time_t	now;
	char	*ts;

	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	strncat(reg_path, "/tmp/daemons.reg", sizeof(reg_path) - strlen(reg_path) - 1);

	// check for daemons created with the same name and add a number to
	// differentiate it
	fp = ft_fopen(reg_path, "r");

	if (fp)
	{
		char line[1024];
		while (fgets(line, sizeof(line), fp))
		{
			if (strncmp(line, name, strlen(name)) == 0 && (line[strlen(name)] == ' ' || line[strlen(name)] == '.'))
				count++;
		}
		fclose(fp);
	}

	if (count == 0)
		snprintf(modified_name, sizeof(modified_name), "%s", name);
	else
		snprintf(modified_name, sizeof(modified_name), "%s.%d", name, count);

	// add entry
	fd = ft_open(reg_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	now = time(NULL);
	ts = ctime(&now);
	if (ts)
		ts[strcspn(ts, "\n")] = 0;
	dprintf(fd, "%s %d %s\n", modified_name, getpid(), ts);
	close(fd);
}

static void daemon_work(const char *project_root)
{
	while (1)
	{
		daemon_log(project_root, "deamon one work cycle");
		sleep(10);
	}
}
