#include <getopt.h>
#include <libgen.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	bool zero = false;

	for (;;)
	{
		static option long_options[] {
			{ "zero", no_argument, nullptr, 'z' },
			{ "help", no_argument, nullptr, 'h' },
			{}
		};

		int ch = getopt_long(argc, argv, "zh", long_options, nullptr);
		if (ch == -1)
			break;

		switch (ch)
		{
			case 'z':
				zero = true;
				break;
			case 'h':
				fprintf(stderr, "usage: %s [OPTION] NAME...\n", argv[0]);
				fprintf(stderr, "  output the directory containing each NAME\n");
				fprintf(stderr, "OPTIONS:\n");
				fprintf(stderr, "  -z, --zero  end each output with NUL instead of a newline\n");
				fprintf(stderr, "  -h, --help  show this message and exit\n");
				return 0;
			case ':': case '?':
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

	for (int i = optind; i < argc; i++)
	{
		printf("%s", dirname(argv[i]));
		putchar(zero ? '\0' : '\n');
	}

	return 0;
}
