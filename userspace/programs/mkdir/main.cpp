#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char* s_argv0 { nullptr };

static bool create_parents(const char* path, bool verbose)
{
	char buffer[PATH_MAX];
	for (size_t i = 0; path[i];)
	{
		for (; path[i] && path[i] != '/'; i++)
			buffer[i] = path[i];
		for (; path[i] && path[i] == '/'; i++)
			buffer[i] = path[i];
		if (path[i] == '\0')
			break;
		buffer[i] = '\0';

		struct stat st;
		if (stat(buffer, &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
				continue;
			fprintf(stderr, "%s: cannot create '%s': %s\n", s_argv0, buffer, strerror(EEXIST));
			return false;
		}

		if (errno != ENOENT || mkdir(buffer, 0) == -1)
		{
			fprintf(stderr, "%s: cannot create '%s': %s\n", s_argv0, buffer, strerror(errno));
			return false;
		}

		if (verbose)
			printf("%s: created directory '%s'\n", s_argv0, buffer);

		const mode_t filemask = umask(0);
		umask(filemask);
		chmod(buffer, (S_IWUSR | S_IXUSR | ~filemask) & 0777);
	}

	return true;
}

static bool create_directory(const char* path, mode_t mode, bool verbose, bool parents)
{
	if (parents && !create_parents(path, verbose))
		return false;

	struct stat st;
	if (stat(path, &st) == 0)
	{
		if (parents && S_ISDIR(st.st_mode))
			return true;
		fprintf(stderr, "%s: cannot create '%s': %s\n", s_argv0, path, strerror(EEXIST));
		return false;
	}

	if (errno != ENOENT || mkdir(path, mode) == -1)
	{
		fprintf(stderr, "%s: cannot create '%s': %s\n", s_argv0, path, strerror(errno));
		return false;
	}

	if (verbose)
		printf("%s: created directory '%s'\n", s_argv0, path);

	return true;
}

int main(int argc, char* argv[])
{
	s_argv0 = argv[0];

	mode_t mode { S_IRWXU | S_IRWXG | S_IRWXO };
	bool verbose { false };
	bool parents { false };

	for (;;)
	{
		static option long_options[] {
			{ "mode",    required_argument, nullptr, 'm' },
			{ "parents", no_argument,       nullptr, 'p' },
			{ "verbose", no_argument,       nullptr, 'v' },
			{ "help",    no_argument,       nullptr,  0  },
			{}
		};

		int ch = getopt_long(argc, argv, "m:pv", long_options, nullptr);
		if (ch == -1)
			break;

		switch (ch)
		{
			case 'm':
			{
				char* endptr = nullptr;
				mode = strtol(optarg, &endptr, 0);
				if (*endptr != '\0')
				{
					fprintf(stderr, "%s: invalid mode '%s'\n", argv[0], optarg);
					fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
					return 1;
				}
				break;
			}
			case 'p':
				parents = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 0:
				fprintf(stderr, "usage: %s [OPTION]... DIRECTORY...\n", argv[0]);
				fprintf(stderr, "  create DIRECTORYs\n");
				fprintf(stderr, "OPTIONS:\n");
				fprintf(stderr, "  -m, --mode=MODE  set mode to MODE\n");
				fprintf(stderr, "  -p, --parents    create any missing intermediate components\n");
				fprintf(stderr, "  -v, --verbose    print a message for each created directory\n");
				fprintf(stderr, "      --help       show this message and exit\n");
				return 0;
			case ':' : case '?':
				fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
				return 1;
		}
	}

	if (optind >= argc)
	{
		fprintf(stderr, "%s: missing operand\n", argv[0]);
		fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
		return 1;
	}

	int ret = 0;
	for (int i = optind; i < argc; i++)
		if (!create_directory(argv[i], mode, verbose, parents))
			ret = 1;

	return ret;
}
