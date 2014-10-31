#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <err.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "workqueue.h"
#include "utils.h"

static workqueue_t workqueue;

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	event_base_loopbreak(base);
}

typedef struct {
	int fd;
	char buf[BUF_SIZE];
	struct event_base *evbase;
	struct event *read_event;
	struct event *timeout_event;
	struct job *read_job;
	struct job *timeout_job;
} Data;

static void
close_connection(Data *data) {
	event_free(data->timeout_event);
	event_free(data->read_event);
	free(data->read_job);
	free(data->timeout_job);
	printf("closing %d\n", data->fd);
	close(data->fd);
	free(data);
}

static void
actually_read(job_t *job) {
	Data *data = job->user_data;
	struct timeval t = {1, 0};

	int len = read(data->fd, data->buf, BUF_SIZE-1);
	if (len > 0) {
		data->buf[len] = '\0';
		event_add(data->timeout_event, &t);
	} else {
		close_connection(data);
	}
}

static void
read_cb(evutil_socket_t fd, short events, void *arg) {
	Data *data = arg;
	workqueue_add_job(&workqueue, data->read_job);
}

static void
actually_timeout(job_t *job) {
	Data *data = job->user_data;
	int len = strlen(data->buf);

	event_del(data->timeout_event);
	workout_string(data->buf);

	if (write(data->fd, data->buf, len) < len) {
		close_connection(data);
	} else {
		event_add(data->read_event, NULL);
	}
}

static void
timeout_cb(evutil_socket_t fd, short events, void *arg) {
	Data *data = arg;
	workqueue_add_job(&workqueue, data->timeout_job);
}

static void
accept_cb(int listener, short event, void *arg) {
	struct event_base *base = arg;
	struct event *read_event;
	Data *data;

	int fd = accept(listener, NULL, NULL);
	if (fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		err(1, "accept");
	}

	evutil_make_socket_nonblocking(fd);

	data = malloc(sizeof(Data));
	read_event = event_new(base, fd, EV_READ, read_cb, data);
	data->fd = fd;
	data->evbase = base;
	data->read_event = read_event;
	data->timeout_event = evtimer_new(base, timeout_cb, data);
	data->read_job = malloc(sizeof(job_t));
	data->read_job->job_function = actually_read;
	data->read_job->user_data = data;
	data->timeout_job = malloc(sizeof(job_t));
	data->timeout_job->job_function = actually_timeout;
	data->timeout_job->user_data = data;

	if (!read_event || event_add(read_event, NULL) < 0)
		err(1, "Error creating a new read_event.");
}

int
main(int argc, char **argv)
{
	struct event_base *base;
	struct event *signal_event, *accept_event;
	int fd;

	setup_limits();

	if (evthread_use_pthreads() < 0)
		err(1, "no threads for libevent.");

	base = event_base_new();
	if (!base)
		err(1, "Could not initialize libevent!");

	signal(SIGPIPE, SIG_IGN);

	fd = create_server_socket();
	evutil_make_socket_nonblocking(fd);

	if (workqueue_init(&workqueue, 100))
		err(1, "Failed to create work queue");

	accept_event = event_new(base, fd, EV_READ|EV_PERSIST, accept_cb, (void *)base);
	if (!accept_event || event_add(accept_event, NULL) < 0)
		err(1, "Error creating a new event.");

	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL) < 0)
		err(1, "Could not create/add a signal event!");

	printf("Server ready\n");
	event_base_dispatch(base);

	event_free(signal_event);
	event_free(accept_event);
	event_base_free(base);

	printf("Goodbye\n");
	return 0;
}
