#include <BAN/String.h>
#include <BAN/Optional.h>
#include <BAN/Vector.h>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/banan-os.h>
#include <sys/random.h>
#include <termios.h>

static void generate_machine_id()
{
	int fd = open("/etc/machine-id", O_WRONLY | O_CREAT | O_EXCL, 0444);
	if (fd == -1)
		return;

	uint8_t machine_id[16];
	if (getrandom(machine_id, sizeof(machine_id), 0) < static_cast<ssize_t>(sizeof(machine_id)))
	{
		unlinkat(fd, nullptr, 0);
		close(fd);
		return;
	}

	for (size_t i = 0; i < sizeof(machine_id); i++)
		dprintf(fd, "%02x", machine_id[i]);
	dprintf(fd, "\n");

	close(fd);
}

static void run_init_scripts()
{
	DIR* dirp = opendir("/etc/init.d");
	if (dirp == nullptr)
		return;

	dirent* dirent;
	while ((dirent = readdir(dirp)))
	{
		if (dirent->d_type != DT_REG)
			continue;

		char script_path[PATH_MAX];
		sprintf(script_path, "/etc/init.d/%s", dirent->d_name);

		const pid_t pid = fork();
		if (pid == -1)
			continue;

		if (pid == 0)
		{
			execl(script_path, script_path, nullptr);
			exit(1);
		}

		int status;
		while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
			continue;

		if (WIFSIGNALED(status))
			dwarnln("'{}' was killed by signal {}", script_path, strsignal(WTERMSIG(status)));
		else if (const int exit_code = WEXITSTATUS(status))
			dwarnln("'{}' exited with {}", script_path, exit_code);
		else
			dprintln("'{}' succeeded", script_path);
	}

	closedir(dirp);
}

int main(int argc, char** argv)
{
	ASSERT(argc == 2);

	const char* tty_name = argv[1];

	if (open(tty_name, O_RDONLY | O_TTY_INIT) != 0) _exit(1);
	if (open(tty_name, O_WRONLY) != 1) _exit(1);
	if (open(tty_name, O_WRONLY) != 2) _exit(1);
	if (open("/dev/debug", O_WRONLY) != 3) _exit(1);

	if (signal(SIGINT, [](int) {}) == SIG_ERR)
		perror("signal");

	if (load_keymap("/usr/share/keymaps/us.keymap") == -1)
		perror("load_keymap");

	setenv("TERM", "ansi", 1);
	setenv("PATH", "/bin:/usr/bin", 1);

	generate_machine_id();
	run_init_scripts();


	if (fork() == 0)
	{
		execl("/usr/bin/dhcp-client", "dhcp-client", NULL);
		exit(1);
	}

	if (fork() == 0)
	{
		execl("/usr/bin/resolver", "resolver", NULL);
		exit(1);
	}

	if (fork() == 0)
	{
		execl("/usr/bin/AudioServer", "AudioServer", NULL);
		exit(1);
	}

	if (fork() == 0)
	{
		execl("/usr/bin/ClipboardServer", "ClipboardServer", NULL);
		exit(1);
	}

	bool first = true;

	termios termios;
	tcgetattr(STDIN_FILENO, &termios);

	while (true)
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &termios);

		char name_buffer[128];

		while (!first)
		{
			printf("username: ");
			fflush(stdout);

			ssize_t nread = read(STDIN_FILENO, name_buffer, sizeof(name_buffer) - 1);
			if (nread == -1)
			{
				perror("read");
				return 1;
			}
			if (nread <= 1 || name_buffer[nread - 1] != '\n')
				continue;
			name_buffer[nread - 1] = '\0';
			break;
		}

		if (first)
		{
			strcpy(name_buffer, "user");
			first = false;
		}

		auto* pwd = getpwnam(name_buffer);
		if (pwd == nullptr)
			continue;

		if (chown(tty_name, pwd->pw_uid, 0) == -1)
			perror("chown");

		pid_t pid = fork();
		if (pid == 0)
		{
			pid_t pgrp = setpgrp();
			if (tcsetpgrp(0, pgrp) == -1)
			{
				perror("tcsetpgrp");
				exit(1);
			}

			printf("Welcome back %s!\n", pwd->pw_name);

			if (initgroups(name_buffer, pwd->pw_gid) == -1)
				perror("initgroups");
			if (setgid(pwd->pw_gid) == -1)
				perror("setgid");
			if (setuid(pwd->pw_uid) == -1)
				perror("setuid");

			setenv("HOME", pwd->pw_dir, 1);
			chdir(pwd->pw_dir);

			setenv("SHELL", pwd->pw_shell, 1);
			char shell_path[PATH_MAX];
			strcpy(shell_path, pwd->pw_shell);

			endpwent();

			execl(shell_path, shell_path, nullptr);
			perror("execl");

			exit(1);
		}

		endpwent();

		if (pid == -1)
		{
			perror("fork");
			break;
		}

		int status;
		waitpid(pid, &status, 0);

		if (tcsetpgrp(0, getpgrp()) == -1)
			perror("tcsetpgrp");

		if (chown(tty_name, 0, 0) == -1)
			perror("chown");
	}

}
