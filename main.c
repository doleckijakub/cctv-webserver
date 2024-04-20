#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

const char *argv0;

void error(const char *msg, ...) {
	va_list va;
	va_start(va, msg);

	fprintf(stderr, "Error: ");
	vfprintf(stderr, msg, va);
	fprintf(stderr, "\n");

	va_end(va);
}

void usage(void) {
	fprintf(stderr,
		"Usage: %s "
		"<ip|localhost|anyhost> "
		"<port> "
		"[/path:/dev/videoX...]"
		"\n",
		argv0);
}

#define NEXT_ARG() (assert(argc-- > 0), *argv++)
int main(int argc, const char **argv) {
	argv0 = NEXT_ARG();

	unsigned int s_addr;
	if (argc) {
		const char *arg = NEXT_ARG();
		if (strcmp(arg, "localhost") == 0) {
			s_addr = inet_addr("127.0.0.1");
		} else if (strcmp(arg, "anyhost") == 0) {
			s_addr = INADDR_ANY;
		} else if ((s_addr = inet_addr(arg)) == INADDR_NONE) {
			error("Invalid ip provided: %s", arg);
			usage();
			return 1;
		}
	} else {
		error("Ip, 'localhost' or 'anyhost' expected");
		usage();
		return 1;
	}

	int port;
	if (argc) {
		port = atoi(NEXT_ARG());
	} else {
		error("Port expected");
		usage();
		return 1;
	}

	printf("host: %s:%d\n", inet_ntoa((struct in_addr) { .s_addr = s_addr }), port);

	if (argc) {
		do {
			const char *arg = NEXT_ARG();
			
			const char *path = arg;
			const char *video_device = strchr(arg, ':');
			*(char *) video_device++ = 0;

			printf("path: %s\n", path);
			printf("video_device: %s\n", video_device);
		} while (argc);
	} else {
		error("At least one path:video_device pair expected");
		usage();
		return 1;
	}

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

}
