#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* s_argv0 { nullptr };

static bool check_realpath(const char* path, bool only_existing)
{
	if (char* resolved = realpath(path, nullptr))
	{
		printf("%s\n", resolved);
		free(resolved);
		return true;
	}

	if (only_existing)
	{
		fprintf(stderr, "%s: %s: %m\n", s_argv0, path);
		return false;
	}

	char* path_c1 = strdup(path);
	char* path_c2 = strdup(path);
	if (path_c1 == nullptr || path_c2 == nullptr)
	{
		fprintf(stderr, "%s: %s: %m\n", s_argv0, path);
		free(path_c1);
		free(path_c2);
		return false;
	}

	char* resolved = realpath(dirname(path_c1), nullptr);
	if (resolved == nullptr)
		fprintf(stderr, "%s: %s: %m\n", s_argv0, path);
	else
	{
		if (strcmp(resolved, "/") != 0)
			printf("%s", resolved);
		printf("/%s\n", basename(path_c2));
	}

	free(path_c1);
	free(path_c2);
	free(resolved);
	return !!resolved;
}

int main(int argc, char** argv)
{
	s_argv0 = argv[0];

	bool only_existing { false };

	for (;;)
	{
		static option long_options[] {
			{ "canonicalize",          no_argument, nullptr, 'E' },
			{ "canonicalize-existing", no_argument, nullptr, 'e' },
			{ "help",                  no_argument, nullptr,  0  },
			{}
		};

		int ch = getopt_long(argc, argv, "Ee", long_options, nullptr);
		if (ch == -1)
			break;

		switch (ch)
		{
			case 'E':
				only_existing = false;
				break;
			case 'e':
				only_existing = true;
				break;
			case 0:
				fprintf(stderr, "usage: %s [OPTION]... FILE...\n", argv[0]);
				fprintf(stderr, "  print resolved path of FILEs\n");
				fprintf(stderr, "OPTIONS:\n");
				fprintf(stderr, "  -E, --canonicalize           all but last component of FILE must exist\n");
				fprintf(stderr, "  -e, --canonicalize-existing  all components of FILE must exist\n");
				fprintf(stderr, "      --help                   show this message and exit\n");
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
		if (!check_realpath(argv[i], only_existing))
			ret = 1;

	return ret;
}
