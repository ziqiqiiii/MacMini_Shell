#include "system_program.h"
#include <libgen.h>
#include <pthread.h>
#include <sys/file.h>
#define MAX_DAEMONS 64

char project_root[PATH_MAX];
int plant_lock_fd = -1;

typedef struct PlantStats {
	int waterlevel;
	int health;
	int level;
	time_t last_watered;
	time_t last_updated;
} PlantStats;

typedef struct PlantSpace {
	PlantStats stats;
	pthread_mutex_t mu;
	int running;
	char name[64];
} PlantSpace;

/**
 * @brief Resolves the absolute path of the project root directory.
 *
 * Reads the executable's path via /proc/self/exe and navigates one level up
 * if the binary resides in a "bin" subdirectory. Falls back to $HOME or "."
 * if the symlink cannot be read. Result is stored in the global project_root.
 */
void resolve_project_root(void) {
	// printf("resolving project root \n");
	char path[PATH_MAX];
	// should return in the /bin dir, need to back one out
	ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (n == -1) {
		// fallback if unable to get path
		perror("readlink");
		const char *home = getenv("HOME");
		if (home)
			strncpy(project_root, home, sizeof(project_root));
		else
			strcpy(project_root, ".");
		return;
	}

	path[n] = '\0';
	char *dir = dirname(path);
	if (strcmp(basename(dir), "bin") == 0)
		strncpy(project_root, dirname(dir), sizeof(project_root) - 1);
	else
		strncpy(project_root, dir, sizeof(project_root) - 1);

	project_root[sizeof(project_root) - 1] = '\0';
	// printf("project root path: %s\n", project_root);
}

/**
 * @brief Acquires an exclusive advisory lock for a named plant daemon.
 *
 * Creates or opens tmp/plant_<name>.lock and attempts a non-blocking
 * exclusive flock. Stores the open file descriptor in plant_lock_fd so
 * unlock_plant() can release it later.
 *
 * @param name Name of the plant daemon to lock.
 * @return 1 if the lock was acquired, 0 if already held by another process.
 */
int plant_lock(const char *name) {
	size_t len =
		snprintf(NULL, 0, "%s/tmp/plant_%s.lock", project_root, name) + 1;
	char *lock = malloc(len);
	if (!lock) {
		perror("malloc");
		free(lock);
		exit(EXIT_FAILURE);
	}
	snprintf(lock, len, "%s/tmp/plant_%s.lock", project_root, name);

	plant_lock_fd = open(lock, O_CREAT | O_RDWR, 0644);
	if (plant_lock_fd == -1) {
		perror("lock open");
		return 0;
	}

	// check if already locked
	if (flock(plant_lock_fd, LOCK_EX | LOCK_NB) == -1) {
		close(plant_lock_fd);
		return 0;
	}
	free(lock);
	return 1;
}

/**
 * @brief Releases the exclusive plant lock held in plant_lock_fd.
 *
 * Unlocks and closes the file descriptor acquired by plant_lock(). Safe to
 * call even if plant_lock_fd is -1 (no-op in that case).
 */
void unlock_plant(void) {
	if (plant_lock_fd != -1) {
		flock(plant_lock_fd, LOCK_UN);
		close(plant_lock_fd);
	}
}

/**
 * @brief Daemonizes the current process using a double-fork.
 *
 * Performs the standard Unix daemonization sequence: first fork exits the
 * parent, setsid() creates a new session, a second fork prevents the daemon
 * from reacquiring a terminal. Closes all open file descriptors and redirects
 * stdin/stdout/stderr to /dev/null.
 */
void spawn_plant(void) {
	printf("Spawning a plant\n");
	// first fork
	pid_t pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0)
		exit(EXIT_FAILURE);

	// disable signals
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	// second child fork
	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0); // allow daemon perms
	// chdir("/"); //change wd to / (always exists)
	// staying in projects root to write to tmp

	// close all fd
	for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
		close(fd);

	// dump stdin to /dev/null
	int null_fd = open("/dev/null", O_RDWR);
	if (null_fd != -1) {
		dup2(null_fd, STDIN_FILENO);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
		if (null_fd > 2)
			close(null_fd);
	}
}

/**
 * @brief Appends a spawn event for the named plant daemon to the log file.
 *
 * Writes a timestamped entry to tmp/dspawn.log recording the daemon name and
 * current PID. Uses an exclusive flock to prevent interleaved writes.
 *
 * @param name Name of the plant daemon that was spawned.
 */
void daemon_spawn_log(const char *name) {
	char log_path[PATH_MAX];
	strncpy(log_path, project_root, sizeof(log_path) - 1);
	strncat(log_path, "/tmp/dspawn.log",
		sizeof(log_path) - strlen(log_path) - 1);

	int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		perror("log open");
		return;
	}

	time_t now = time(NULL);
	flock(fd, LOCK_EX);
	dprintf(fd, "[%s] Spawned dplant daemon %s [PID=%d].\n", name, ctime(&now),
		getpid());
	close(fd);
}

/**
 * @brief Appends an arbitrary message from the daemon to the log file.
 *
 * Writes a timestamped entry to tmp/dspawn.log containing the current PID
 * and the provided message string.
 *
 * @param msg Message string to record in the log.
 */
void daemon_log(const char *msg) {
	char log_path[PATH_MAX];
	strncpy(log_path, project_root, sizeof(log_path) - 1);
	strncat(log_path, "/tmp/dspawn.log",
		sizeof(log_path) - strlen(log_path) - 1);

	int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		perror("log open");
		return;
	}

	time_t now = time(NULL);
	flock(fd, LOCK_EX);
	dprintf(fd, "%sLogging dspawn daemon [%d] message: %s.\n", ctime(&now),
		getpid(), msg);
	close(fd);
}

/**
 * @brief Checks whether a daemon name exists in the active registry.
 *
 * Scans tmp/daemons.reg line by line for an exact name match.
 *
 * @param name Daemon name to look up.
 * @return 1 if the name is found in the registry, 0 otherwise.
 */
int is_daemon_registered(const char *name) {
	char reg_path[PATH_MAX];
	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	strncat(reg_path, "/tmp/daemons.reg",
		sizeof(reg_path) - strlen(reg_path) - 1);

	FILE *fp = fopen(reg_path, "r");
	if (!fp)
		return 0;

	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		char regname[64], ts[128];
		int pid;

		if (sscanf(line, "%63s %d %127[^\n]", regname, &pid, ts) != 3)
			continue;

		if (strcmp(regname, name) == 0) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

/**
 * @brief Registers a daemon by writing its name, PID, and timestamp to the registry.
 *
 * Appends an entry to tmp/daemons.reg. Returns early without writing if the
 * name is already present in the registry.
 *
 * @param name Name to register for the current daemon process.
 */
void daemon_register(const char *name) {
	if (is_daemon_registered(name)) {
		fprintf(stderr, "daemon name: %s already registered\n", name);
		return;
	}

	char reg_path[PATH_MAX];
	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	strncat(reg_path, "/tmp/daemons.reg",
		sizeof(reg_path) - strlen(reg_path) - 1);

	// add entry
	int fd = open(reg_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		perror("daemon register logs open");
		return;
	}

	time_t now = time(NULL);
	char *ts = ctime(&now);
	if (ts)
		ts[strcspn(ts, "\n")] = 0;

	// flock(fd, LOCK_EX);
	dprintf(fd, "%s %d %s\n", name, getpid(), ts);
	close(fd);
}

/**
 * @brief Initializes a plant with default stats and writes the state file.
 *
 * Sets waterlevel and health to 100, level to 1, and both timestamps to the
 * current time. Writes the PlantStats struct as raw binary to
 * tmp/<name>.state.
 *
 * @param name  Name of the plant (used as the state filename).
 * @param plant Pointer to the PlantStats struct to initialize and persist.
 * @return 0 on success, -1 if the state file cannot be opened.
 */
int plant_init_state(const char *name, PlantStats *plant) {
	plant->waterlevel = 100;
	plant->health = 100;
	plant->level = 1;
	plant->last_watered = time(NULL);
	plant->last_updated = time(NULL);

	char state_path[PATH_MAX];
	strncpy(state_path, project_root, sizeof(state_path) - 1);
	strncat(state_path, "/tmp/",
		sizeof(state_path) - strlen(state_path) - 1);
	strncat(state_path, name, sizeof(state_path) - strlen(state_path) - 1);
	strncat(state_path, ".state",
		sizeof(state_path) - strlen(state_path) - 1);

	int fd = open(state_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		perror("plant init state");
		return -1;
	}
	// no need to format w dpirntf, just write the struct directly
	// can view w xxd i guess if you want to
	flock(fd, LOCK_EX);
	write(fd, plant, sizeof(PlantStats));
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

/**
 * @brief Persists the current plant state to disk.
 *
 * Overwrites tmp/<name>.state with the raw PlantStats struct using an
 * exclusive flock to prevent concurrent writes.
 *
 * @param name  Name of the plant (determines the state file path).
 * @param plant Pointer to the PlantStats struct to write.
 */
void plant_save(const char *name, const PlantStats *plant) {
	size_t len =
		snprintf(NULL, 0, "%s/tmp/%s.state", project_root, name) + 1;
	char *state_path = malloc(len);
	if (!state_path) {
		perror("malloc");
		free(state_path);
		exit(EXIT_FAILURE);
	}
	snprintf(state_path, len, "%s/tmp/%s.state", project_root, name);

	int fd = open(state_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		perror("plant save state");
		return;
	}
	flock(fd, LOCK_EX);
	write(fd, plant, sizeof(PlantStats));
	flock(fd, LOCK_UN);
	close(fd);
	free(state_path);
}

/**
 * @brief Loads a plant's persisted state from disk.
 *
 * Reads the raw PlantStats struct from tmp/<name>.state into the provided
 * pointer.
 *
 * @param name  Name of the plant (determines the state file path).
 * @param plant Pointer to the PlantStats struct to populate.
 * @return 0 if the state was read successfully, -1 otherwise.
 */
int plant_load(const char *name, PlantStats *plant) {
	size_t len =
		snprintf(NULL, 0, "%s/tmp/%s.state", project_root, name) + 1;
	char *state_path = malloc(len);
	if (!state_path) {
		perror("malloc");
		free(state_path);
		exit(EXIT_FAILURE);
	}
	snprintf(state_path, len, "%s/tmp/%s.state", project_root, name);

	int fd = open(state_path, O_RDONLY);
	if (fd != -1) {
		if (read(fd, plant, sizeof(PlantStats)) == sizeof(PlantStats)) {
			close(fd);
			// previously forgot to free the path
			perror("malloc");
			free(state_path);
			return 0;
		}
		close(fd);
	}
	free(state_path);
	return -1;
}

/**
 * @brief Waters the plant, increasing its water level by 20 (capped at 100).
 *
 * Loads the current state, applies the water increment, updates both the
 * last_watered and last_updated timestamps, saves the new state, and prints
 * a confirmation with the resulting water level.
 *
 * @param name Name of the plant to water.
 */
void plant_water(const char *name) {
	size_t len =
		snprintf(NULL, 0, "%s/tmp/%s.state", project_root, name) + 1;
	char *state_path = malloc(len);
	if (!state_path) {
		perror("malloc");
		free(state_path);
		exit(EXIT_FAILURE);
	}
	snprintf(state_path, len, "%s/tmp/%s.state", project_root, name);

	int fd = open(state_path, O_RDONLY);
	if (fd == -1) {
		perror("open plant state");
		return;
	}

	PlantStats plant;
	if (read(fd, &plant, sizeof(PlantStats)) != sizeof(PlantStats)) {
		perror("read plant state");
		close(fd);
		return;
	}
	close(fd);

	time_t now = time(NULL);
	plant.waterlevel =
		(plant.waterlevel + 20 > 99) ? 100 : plant.waterlevel + 20;
	plant.last_watered = now;
	plant.last_updated = now;

	// save the new state
	plant_save(name, &plant);

	printf("Successfully watered %s successfully!\nWater state at %d!\n",
	       name, plant.waterlevel);
	free(state_path);
}

/**
 * @brief Checks whether a named plant daemon is currently alive.
 *
 * Looks up the name in tmp/daemons.reg and verifies the stored PID still
 * exists in /proc.
 *
 * @param plantname Name of the plant daemon to check.
 * @return 1 if an active process is found, 0 otherwise.
 */
int is_plant_running(const char *plantname) {
	char reg_path[PATH_MAX];
	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	strncat(reg_path, "/tmp/daemons.reg",
		sizeof(reg_path) - strlen(reg_path) - 1);

	FILE *fp = fopen(reg_path, "r");
	if (!fp) {
		perror("plant running reg open");
		return 0;
	}

	// check if there is a match
	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		char name[64], ts[128];
		int pid;

		if (sscanf(line, "%63s %d %127[^\n]", name, &pid, ts) != 3)
			continue;

		if (strcmp(name, plantname) == 0) {
			char proc_path[128];
			snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
			if (access(proc_path, F_OK) == 0) {
				fclose(fp);
				return 1;
			}
		}
	}

	// default there is no active daemon with this name
	fclose(fp);
	return 0;
}

typedef struct {
	char name[64];
	pid_t pid;
	char timestamp[128];
} DaemonInfo;

/**
 * @brief Records a terminated daemon in the graveyard registry.
 *
 * Appends an entry to tmp/cematary.reg. If an entry with the same name
 * already exists, a numeric suffix (.1, .2, …) is appended to prevent
 * collisions. Uses the PID stored in rip rather than the calling PID.
 *
 * @param rip Struct containing the name, PID, and original timestamp of the daemon.
 */
void add_to_graveyard(DaemonInfo rip) {
	char reg_path[PATH_MAX];
	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	// just like in pet cematary, try not to respawn it lol
	strncat(reg_path, "/tmp/cematary.reg",
		sizeof(reg_path) - strlen(reg_path) - 1);

	FILE *fp = fopen(reg_path, "r");
	int count = 0;

	if (fp) {
		char line[1024];
		while (fgets(line, sizeof(line), fp)) {
			if (strncmp(line, rip.name, strlen(rip.name)) == 0 &&
			    (line[strlen(rip.name)] == ' ' ||
			     line[strlen(rip.name)] == '.'))
				count++;
		}
		fclose(fp);
	}

	printf("adding to graveyard\n");

	char modified_name[128];
	if (count == 0)
		snprintf(modified_name, sizeof(modified_name), "%s", rip.name);
	else
		snprintf(modified_name, sizeof(modified_name), "%s.%d",
			 rip.name, count);

	// add entry
	int fd = open(reg_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		perror("daemon cematary register logs open");
		return;
	}

	time_t now = time(NULL);
	char *ts = ctime(&now);
	if (ts)
		ts[strcspn(ts, "\n")] = 0;

	// dprintf(fd, "%s %d %s\n", modified_name, getpid(), ts);
	dprintf(fd, "%s %d %s\n", modified_name, rip.pid, ts);
	close(fd);
}

/**
 * @brief Automatically sends SIGTERM to all active daemons matching the given name.
 *
 * Scans tmp/daemons.reg for live entries whose name matches the argument,
 * sends SIGTERM to each, and moves them to the graveyard. Intended for
 * programmatic use (e.g., when a plant dies), not interactive selection.
 *
 * @param name Name of the daemon(s) to terminate.
 */
void dkill_auto(const char *name) {
	char reg_path[PATH_MAX];
	strncpy(reg_path, project_root, sizeof(reg_path) - 1);
	strncat(reg_path, "/tmp/daemons.reg",
		sizeof(reg_path) - strlen(reg_path) - 1);

	// check for daemons created with the same name and add a number to
	// differentiate it
	FILE *fd = fopen(reg_path, "r");

	if (!fd) {
		perror("dkill: failed to open daemon reg");
		return;
	}

	int count = 0;
	char line[1024];
	DaemonInfo daemons[MAX_DAEMONS];

	while (fgets(line, sizeof(line), fd) && count < MAX_DAEMONS) {
		char name[64], timestamp[128];
		int pid;

		if (sscanf(line, "%63s %d %127[^\n]", name, &pid, timestamp) !=
		    3)
			continue;

		char proc_path[128];
		snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
		// check all the pids just in case
		if (access(proc_path, F_OK) == 0) {
			strncpy(daemons[count].name, name,
				sizeof(daemons[count].name));
			daemons[count].pid = pid;
			strncpy(daemons[count].timestamp, timestamp,
				sizeof(daemons[count].timestamp));
			++count;
		}
	}
	fclose(fd);

	if (count == 0) {
		printf("No active daemons found\n");
		return;
	}

	for (int i = 0; i < count; ++i) {
		// compare string instead of address
		if (strcmp(daemons[i].name, name) != 0)
			continue;

		if (kill(daemons[i].pid, SIGTERM) == 0) {
			printf("Killed %-12s (PID %d)\n", daemons[i].name,
			       daemons[i].pid);
			DaemonInfo toinsert = daemons[i];
			add_to_graveyard(toinsert);
		} else {
			perror("kill");
		}
	}
}

/**
 * @brief Handles plant death: releases resources and terminates the daemon.
 *
 * Logs the death event, releases and removes the plant lock file, calls
 * dkill_auto() to deregister the daemon, then exits the process.
 *
 * @param name Name of the plant that has died.
 */
void plant_died(const char *name) {
	daemon_log("HP has hit 0, plant has died");
	daemon_log("Natural plant death, died from hitting HP 0");

	if (plant_lock_fd != -1) {
		flock(plant_lock_fd, LOCK_UN);
		close(plant_lock_fd);

		size_t len = snprintf(NULL, 0, "%s/tmp/plant_%s.lock",
				      project_root, name);
		char *lock_path = malloc(len);
		if (!lock_path) {
			perror("malloc");
			free(lock_path);
			return;
		}
		snprintf(lock_path, len, "%s/tmp/plant_%s.lock", project_root,
			 name);
		unlink(lock_path);
	}

	printf("running dkill_auto");
	dkill_auto(name);

	exit(EXIT_SUCCESS);
}

/**
 * @brief Main event loop of the plant daemon.
 *
 * Loads or initializes the plant state, then runs an infinite loop that
 * wakes every 10 seconds. Each time at least 60 seconds have elapsed since
 * last_updated, the loop drains the water level, adjusts health (and
 * potentially levels up), saves state, and checks for death. Calls
 * plant_died() if health reaches 0.
 *
 * @param name Name of the plant this daemon manages.
 */
void daemon_work(const char *name) {
	PlantStats plant;
	if (plant_load(name, &plant) != 0) {
		daemon_log("Initializing new plant state");
		if (plant_init_state(name, &plant) != 0) {
			daemon_log("Initializing new plant state");
			exit(EXIT_FAILURE);
		}
	}

	while (1) {
		time_t now = time(NULL);
		int seconds_passed = (int)difftime(now, plant.last_updated);

		// reduce every minute
		if (seconds_passed >= 60) {
			// plant.waterlevel -= seconds_passed / 60;
			plant.waterlevel -= 101;
			if (plant.waterlevel < 0)
				plant.waterlevel = 0;

			// if (plant.waterlevel == 0)
			//         plant.health -= 5;
			if (plant.waterlevel == 0)
				plant.health -= 100;
			else if (plant.waterlevel > 80 && plant.health < 100)
				plant.health += 1;

			if (plant.health > 95 && plant.waterlevel > 95)
				plant.level++;

			plant.last_updated = now;
			plant_save(name, &plant);
			daemon_log("one plant cycle, state updated");
		}

		if (plant.health <= 0) {
			plant_died(name);
		}

		sleep(10);
	}
}

/**
 * @brief Loads and prints the current statistics for a plant.
 *
 * Reads the plant's state file and displays its name, water level, health,
 * level, last watered time, and last updated time to stdout.
 *
 * @param plant_name  Name of the plant to query.
 * @param statplant   Output pointer populated with the loaded PlantStats.
 * @return 0 if stats were loaded and printed successfully, 1 otherwise.
 */
int plant_stats(const char *plant_name, PlantStats *statplant) {
	if (plant_load(plant_name, statplant) == 0) {
		printf("Plant:                     %s\n", plant_name);
		printf("Water Level:               %d\n",
		       statplant->waterlevel);
		printf("Health:                    %d\n", statplant->health);
		printf("Level:                     %d\n", statplant->level);
		printf("Last Watered:              %s",
		       ctime(&(statplant->last_watered)));
		printf("Last Updated:              %s",
		       ctime(&(statplant->last_updated)));
		return 0;
	}
	return 1;
}

/**
 * @brief Respawns a previously registered but no-longer-running plant daemon.
 *
 * Verifies the daemon is not already alive and is present in the registry,
 * then daemonizes the process, re-registers it, logs the spawn, and resumes
 * the daemon work loop with the existing state.
 *
 * @param name Name of the plant daemon to respawn.
 */
void respawn_plant(const char *name) {
	if (is_plant_running(name)) {
		fprintf(stderr, "plant daemon is still alive\n");
		return;
	}

	if (!is_daemon_registered(name)) {
		fprintf(stderr, "plant daemon not registered\n");
		return;
	}

	printf("respawning dplant daemon: %s\n", name);
	spawn_plant();
	daemon_register(name);
	daemon_spawn_log(name);
	daemon_log("respawned dplant");
	daemon_work(name);

	unlock_plant();
}

/**
 * @brief Entry point for the dplant daemon manager.
 *
 * Dispatches based on argument count and subcommand:
 *   dplant <name>           — spawn a new plant daemon.
 *   dplant <name> water     — water the running plant.
 *   dplant <name> stats     — print the plant's current stats.
 *   dplant <name> respawn   — respawn a stopped but registered plant.
 *
 * @param argc Number of command-line arguments (2 or 3).
 * @param argv Array of command-line arguments.
 * @return 0 on success, 1 on invalid usage or error.
 */
int main(int argc, char **argv) {

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: dplant <plant_name>\n");
		return 1;
	}

	resolve_project_root();
	const char *plant_name = argv[1];

	// wtf if i dkill and run dplant plant water it creates a new daemon lol
	// if(argc == 3 && is_plant_running(plant_name)){
	// stop spawning D:
	if (argc == 3 && !is_plant_running(plant_name) &&
	    strcmp("respawn", argv[2]) != 0) {
		fprintf(stderr, "No running daemon for '%s']n", plant_name);
		fflush(stderr);
		return 1;
	} else if (argc == 3 && strcmp("respawn", argv[2]) == 0) {
		respawn_plant(plant_name);
		return 0;
	}

	if (argc == 3 && is_plant_running(plant_name)) {
		if (strcmp(argv[2], "water") == 0) {
			plant_water(plant_name);
			return 0;
		} else if (strcmp(argv[2], "stats") == 0) {
			PlantStats statplant;
			if (plant_stats(plant_name, &statplant) != 0) {
				fprintf(stderr,
					"unable to load state for: %s\n",
					plant_name);
			}
			return 0;
		} else {
			fprintf(stderr,
				"unknown command for running daemon: %s\n",
				argv[2]);
			return 1;
		}
	}

	if (is_plant_running(plant_name)) {
		fprintf(stderr, "dplant daemon <%s> is already active\n",
			plant_name);
		return 1;
	}

	if (!plant_lock(plant_name)) {
		fprintf(stderr, "could not acquire lock for daemon: <%s>\n",
			plant_name);
		return 1;
	}

	if (argc == 2 && !is_daemon_registered(plant_name)) {
		spawn_plant();
		daemon_register(plant_name);
		daemon_spawn_log(plant_name);
		daemon_log("dplant daemon started");
		daemon_work(plant_name);

		unlock_plant();
		return 0;
	}

	return 1;
}
