#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CLIENTS 1024

const char *argv0;

void stop(int signum) {
	(void) signum;

	kill(getpid(), SIGKILL);
}

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

typedef struct {
	int fd;
	char ip[16];
	int port;
} socket_request;

void client_handler(socket_request *sock_req) {
	const char *method = "GET";
	const char *path = "/";
	const char *http_version = "HTTP/1.1";

	int code = 200;
	size_t bytes_written = 0;

	close(sock_req->fd);
	
	printf("%s %s %s %s %d %zu\n", sock_req->ip, method, path, http_version, code, bytes_written);
}

void *__client_handler(void *data) {
	client_handler((socket_request *) data);

	free(data);

	pthread_exit(NULL);
}

#define NEXT_ARG() (assert(argc-- > 0), *argv++)
int main(int argc, const char **argv) {
	argv0 = NEXT_ARG();

	assert(signal(SIGINT, stop) != SIG_ERR);

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
	if (server_fd < 0) {
		error("Could not start server: %s", strerror(errno));
		return 1;
	}

	struct sockaddr_in server_addr = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_addr.s_addr = s_addr,
		.sin_port = htons(port)
	};

	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		error("Could not start server: %s", strerror(errno));
		return 1;
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) {	
		error("Could not start server: %s", strerror(errno));
		return 1;
	}

	while (1) {
		static struct sockaddr_in client_addr;
		static socklen_t client_len = sizeof(client_addr);
		static int client_fd;

		if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
			error("%s:%d: accept() failed: %s", __FILE__, __LINE__, strerror(errno));
			continue;
		}

		socket_request *sock_req = malloc(sizeof(socket_request));
		sock_req->fd = client_fd;
		strcpy(sock_req->ip, inet_ntoa(client_addr.sin_addr));
		sock_req->port = ntohs(client_addr.sin_port);

		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, __client_handler, sock_req) != 0) {
			error("%s:%d: pthread_create() failed: %s", __FILE__, __LINE__, strerror(errno));
			close(client_fd);
			continue;
		}
	}

}
