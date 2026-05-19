#include <BAN/String.h>
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>

static const char* s_argv0 { nullptr };

static char parse_char(const char*& array)
{
	assert(array[0]);

	if (array[0] != '\\')
		return *array++;

	switch (array[1])
	{
		case '\\': array += 2; return '\\';
		case 'a':  array += 2; return '\a';
		case 'b':  array += 2; return '\b';
		case 'f':  array += 2; return '\f';
		case 'n':  array += 2; return '\n';
		case 'r':  array += 2; return '\r';
		case 't':  array += 2; return '\t';
		case 'v':  array += 2; return '\v';
	}

	size_t octal_count = 0;
	while (octal_count < 3 && '0' <= array[1 + octal_count] && array[1 + octal_count] <= '7')
		octal_count++;

	if (octal_count == 0)
		return *array++;

	int value = 0;
	for (size_t i = 1; i <= octal_count; i++)
		value = (value * 8) + (array[i] - '0');

	array += 1 + octal_count;
	return value;
}

static BAN::String expand_array(const char* array, size_t expand_len)
{
	BAN::String result;
	while (*array)
	{
		if (array[0] == '[')
		{
			const char* end = strchr(array + 1, ']');
			if (end == nullptr)
				goto normal_character;

			const size_t len = end - array - 1;
			if (len < 2)
				goto normal_character;

			if (array[1] == ':' && end[-1] == ':')
			{
				int (*class_test)(int) = nullptr;

#define CHECK_CHAR_CLASS(name) \
				else if (len == sizeof(#name) + 1 && memcmp(array + 2, #name, len - 2) == 0) \
					class_test = is##name
				if (false);
				CHECK_CHAR_CLASS(alnum);
				CHECK_CHAR_CLASS(alpha);
				CHECK_CHAR_CLASS(blank);
				CHECK_CHAR_CLASS(cntrl);
				CHECK_CHAR_CLASS(digit);
				CHECK_CHAR_CLASS(graph);
				CHECK_CHAR_CLASS(lower);
				CHECK_CHAR_CLASS(print);
				CHECK_CHAR_CLASS(punct);
				CHECK_CHAR_CLASS(space);
				CHECK_CHAR_CLASS(upper);
				CHECK_CHAR_CLASS(xdigit);
#undef CHECK_CHAR_CLASS

				if (class_test == nullptr)
				{
					fprintf(stderr, "%s: invalid character class '%.*s'\n", s_argv0, (int)len - 2, array + 2);
					exit(1);
				}

				for (int ch = 0; ch < 0x100; ch++)
					if (class_test(ch))
						MUST(result.push_back(ch));
			}
			else if (array[1] == '=' && end[-1] == '=')
			{
				if (len - 2 != 1)
				{
					fprintf(stderr, "%s: %.*s: equivalence class must be a single character\n", s_argv0, (int)len - 2, array + 2);
					exit(1);
				}

				// TODO: actually support collating elements
				MUST(result.push_back(array[2]));
			}
			else
			{
				const char* temp = array + 1;
				const char ch = parse_char(temp);

				if (temp[0] != '*')
					goto normal_character;
				temp++;

				const int base = (temp[0] == '0') ? 8 : 10;

				bool valid_count = true;
				for (size_t i = 0; valid_count && temp[i] != ']'; i++)
					valid_count = ('0' <= temp[i] && temp[i] <= '0' + base - 1);
				if (!valid_count)
				{
					fprintf(stderr, "%s: invalid repeat count '%.*s'\n", s_argv0, (int)len - 2, array + 2);
					exit(1);
				}

				size_t count = 0;
				for (size_t i = 0; temp[i] != ']'; i++)
					count = (count * base) + (temp[i] - '0');
				if (count == 0 && result.size() < expand_len)
					count = expand_len - result.size();

				for (size_t i = 0; i < count; i++)
					MUST(result.push_back(ch));
			}

			array = end + 1;
			continue;
		}

	normal_character:
		const char ch1 = parse_char(array);

		if (array[0] == '-' && array[1])
		{
			array++;
			const char ch2 = parse_char(array);
			for (int ch = ch1; ch <= ch2; ch++)
				MUST(result.push_back(ch));
			continue;
		}

		MUST(result.push_back(ch1));
	}

	return result;
}

int main(int argc, char* argv[])
{
	s_argv0 = argv[0];

	bool complement { false };
	bool delete_    { false };
	bool squeeze    { false };
	bool truncate   { false };

	for (;;)
	{
		static option long_options[] {
			{ "complement",      no_argument, nullptr, 'c' },
			{ "delete",          no_argument, nullptr, 'd' },
			{ "squeeze-repeats", no_argument, nullptr, 's' },
			{ "truncate-set1",   no_argument, nullptr, 't' },
			{ "help",            no_argument, nullptr,  0  },
			{}
		};

		int ch = getopt_long(argc, argv, "cCdst", long_options, nullptr);
		if (ch == -1)
			break;

		switch (ch)
		{
			case 'c': case 'C':
				complement = true;
				break;
			case 'd':
				delete_ = true;
				break;
			case 's':
				squeeze = true;
				break;
			case 't':
				truncate = true;
				break;
			case 0:
				fprintf(stderr, "usage: %s [OPTION]... STRING1 [STRING2]\n", argv[0]);
				fprintf(stderr, "  translate and/or delete characters from standard input\n");
				fprintf(stderr, "OPTIONS:\n");
				fprintf(stderr, "  -c, -C, --complement   do not ignore entries starting with .\n");
				fprintf(stderr, "  -d, --delete           do not list . and ..\n");
				fprintf(stderr, "  -s, --squeeze-repeats  list directories and not their contents\n");
				fprintf(stderr, "  -t, --truncate-set1    print sizes in human readable form\n");
				fprintf(stderr, "      --help             show this message and exit\n");
				return 0;
			case ':' : case '?':
				fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
				return 1;
		}
	}

	const int needed_args = (delete_ == squeeze) ? 2 : 1;
	if (optind + needed_args > argc)
	{
		fprintf(stderr, "%s: missing operand\n", argv[0]);
		fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
		return 1;
	}

	if (optind + 2 < argc)
	{
		fprintf(stderr, "%s: extra operand '%s'\n", argv[0], argv[optind + 2]);
		fprintf(stderr, "see '%s --help' for usage\n", argv[0]);
		return 1;
	}

	BAN::String array1 = expand_array(argv[optind], 0);
	if (complement)
	{
		bool contains[0x100] {};
		for (int ch : array1)
			contains[ch] = true;
		array1.clear();
		for (int ch = 0; ch < 0x100; ch++)
			if (!contains[ch])
				MUST(array1.push_back(ch));
	}

	BAN::Optional<BAN::String> array2;
	if (optind + 1 < argc)
	{
		array2 = expand_array(argv[optind + 1], array1.size());;
		if (truncate && array1.size() > array2->size())
			MUST(array1.resize(array2->size()));
		if (!array1.empty() && array2->empty())
		{
			fprintf(stderr, "%s: STRING2 must not be empty\n", argv[0]);
			return 1;
		}
		while (array2->size() < array1.size())
			MUST(array2->push_back(array2->back()));
	}

	char translate_map[0x100] {};
	for (int ch = 0; ch < 0x100; ch++)
		translate_map[ch] = ch;
	if (!delete_ && !squeeze)
		for (size_t i = 0; i < array1.size(); i++)
			translate_map[static_cast<int>(array1[i])] = array2.value()[i];

	bool delete_set[0x100] {};
	if (delete_)
	{
		for (int ch : array1)
			delete_set[ch] = true;
	}

	bool squeeze_set[0x100] {};
	if (squeeze)
	{
		const auto& array = array2.has_value() ? array2.value() : array1;
		for (int ch : array)
			squeeze_set[ch] = true;
	}

	int prev_char = -1;
	for (;;)
	{
		int ch = getchar();
		if (ch == EOF)
			break;
		ch = translate_map[ch];
		if (delete_set[ch])
			continue;
		if (squeeze_set[ch] && prev_char == ch)
			continue;
		prev_char = ch;
		putchar(ch);
	}

	return 0;
}
