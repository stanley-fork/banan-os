#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>


char* g_exe;
char* shift_args(int *argc, char ***argv) {
	return (assert((*argc) > 0), ((*argc)--, *((*argv)++)));
}
void help(FILE* sink) {
	fprintf(sink, "%s <input filename>\n", g_exe);
}
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eprintfln(fmt, ...) eprintf(fmt "\n", ## __VA_ARGS__)
char* read_entire_file(const char* path) {
	char* result = NULL;
	char* head = NULL;
	char* end = NULL;
	size_t buf_size = 0;
	long at = 0;
	FILE *f = fopen(path, "rb");

	if(!f) {
		fprintf(stderr, "ERROR Could not open file %s: %s\n",path,strerror(errno));
		return NULL;
	}
	if(fseek(f, 0, SEEK_END) != 0) {
		fprintf(stderr, "ERROR Could not fseek on file %s: %s\n",path,strerror(errno));
		result = NULL; goto DEFER;
	}
	at = ftell(f);
	if(at == -1L) {
		fprintf(stderr, "ERROR Could not ftell on file %s: %s\n",path,strerror(errno));
		result = NULL; goto DEFER;
	}
	buf_size = at+1;
	rewind(f);
	result = (char*)malloc(buf_size);
	assert(result && "Ran out of memory");
	head = result;
	end = result+buf_size-1;
	while(head != end) {
		head += fread(head, 1, end-head, f);
		if(ferror(f)) {
			fprintf(stderr, "ERROR Could not fread on file %s: %s\n",path,strerror(errno));
			free(result);
			result = NULL; goto DEFER;
		}
	}
	result[buf_size-1] = '\0';
DEFER:
	fclose(f);
	return result;
}

static int banos_install(const void* driver_image) {
	return syscall(SYS_BANOS_INSTALL, driver_image);
}


int main(int argc, char** argv)
{
	g_exe = shift_args(&argc, &argv);
	char* input_filename = NULL;
	while(argc > 0) {
		char* arg = shift_args(&argc, &argv);
		if(!input_filename) input_filename = arg;
		else {
			eprintfln("ERROR: Unexpected argument `%s'", arg);
			help(stderr);
			return 1;
		}
	}
	if(!input_filename) {
		eprintfln("ERROR: Missing input filename!");
		help(stderr);
		return 1;
	}
	char* data = read_entire_file(input_filename);
	if(!data) return 1;
	int id = banos_install(data);
	if(id == -1) {
		eprintfln("ERROR: Failed to install driver `%s': %s", input_filename, strerror(errno));
		return 1;
	}
	printf("%d\n", id);
	return 0;
}
