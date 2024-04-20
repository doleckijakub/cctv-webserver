cctv-webserver: main.c
	cc -o $@ -Wall -Wextra $^ -lpthread
