/*
 * one-thread-connection.c
 *
 *  Created on: 24/10/2014
 *      Author: jwendell
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <pthread.h>
#include <signal.h>

#include "utils.h"

pthread_mutex_t n_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
static int n_threads = 0;

static int
inc_n_threads(void) {
	int result;

	pthread_mutex_lock(&n_threads_mutex);
	result = ++n_threads;
	pthread_mutex_unlock(&n_threads_mutex);

	return result;
}

static int
dec_n_threads(void) {
	int result;

	pthread_mutex_lock(&n_threads_mutex);
	result = --n_threads;
	pthread_mutex_unlock(&n_threads_mutex);

	return result;
}

typedef struct {
	int fd;
} Info;

static void *
handle_connection(void *arg) {
	Info *info = arg;
	int len;
	char buf[BUF_SIZE];

	pthread_detach(pthread_self());
	inc_n_threads();

	while (1) {
		len = read(info->fd, buf, BUF_SIZE-1);
		if (len <= 0)
			break;
		buf[len] = '\0';

		workout_string(buf);

		if (write(info->fd, buf, len) < len) {
			break;
		}
		sleep(1);
	}

	printf("closing %d\n", info->fd);
	close(info->fd);
	free(arg);
	dec_n_threads();
	return NULL;
}

static void
stats(int s) {
	printf("%d threads\n", n_threads);
}

static void
main_loop(int listenfd) {
	pthread_t t;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	/* Set only 512K for stack, to reach max number of threads */
	if (pthread_attr_setstacksize(&attr, 524288) != 0)
		perror("pthread_attr_setstacksize");

	while (1) {
		Info *info = malloc(sizeof(Info));
		if (!info)
			err(1, "no memory, exiting");

		info->fd = accept(listenfd, NULL, NULL);
		if (pthread_create(&t, &attr, handle_connection, (void *) info) != 0) {
			fprintf(stderr, "can't create more threads (current number: %d), ignoring this connection attempt (%s)\n", n_threads, strerror(errno));
			close(info->fd);
			free(info);
		}
	}
}

int main(int argc, char **argv) {
	int listenfd;
	int opt, n_processes = 1;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			n_processes = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-p number_of_processes (default: 1)] \n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	setup_limits();
	listenfd = create_server_socket();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, stats);

	if (n_processes > 1) {
		int i;
		pid_t *pids = malloc(n_processes * sizeof(pid_t));

		printf("Server running with %d processes.\n", n_processes);
		for (i = 0; i < n_processes; i++) {
			if ((pids[i] = fork()) == 0) {
				main_loop(listenfd);
				exit(0);
			}
		}

		close(listenfd);
		for (i = 0; i < n_processes; i++) {
			waitpid(pids[i], NULL, 0);
		}
	} else {
		printf("Server running with 1 process.\n");
		main_loop(listenfd);
	}

	printf("exiting main app\n");
	return 0;
}
