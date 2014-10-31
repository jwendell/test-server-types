
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

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/buffer.h>

#include "utils.h"

#define MESSAGE "Hello world\n"
#define T 10

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
	int *i = ptr;
    if (events & BEV_EVENT_CONNECTED) {
    	struct evbuffer *output = bufferevent_get_output(bev);
    	evbuffer_add(output, "init", 4);
    	printf("connected %d\n", *i);
    } else {
         fprintf(stderr, "couldn't connect %d\n", *i);
         bufferevent_free(bev);
         free(ptr);
    }
}

void readcb(struct bufferevent *bev, void *ptr)
{
	int *i = ptr;
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);
	unsigned char buf[10];

	int len = evbuffer_remove(input, buf, sizeof(buf)-1);
	if (len < 0)
		return;
	buf[len] = '\0';
	evbuffer_add(output, "loop", 4);
}

int main(int argc, char **argv) {
	int listenfd, clientfd;
	struct sockaddr_in addr;
	int i;
	pthread_t t;
	struct event_base *evbase;
	struct bufferevent *be;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <SERVER IP> <NUMBER_OF_CONNECTIONS>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	setup_limits();

	if ((evbase = event_base_new()) == NULL)
		err(1, "can't create event base");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (inet_aton(argv[1], &addr.sin_addr) == 0)
		err(1, "invalid host");
	addr.sin_port = htons(SERVER_PORT);

	for (i = 1; i <= atoi(argv[2]); i++) {
		int *n = malloc(sizeof(int));
		*n = i;
		be = bufferevent_socket_new(evbase, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(be, readcb, NULL, eventcb, n);
		bufferevent_enable(be, EV_READ);
		bufferevent_socket_connect(be, (struct sockaddr *)&addr, sizeof (addr));
	}

	event_base_dispatch(evbase);
	event_base_free(evbase);

	return 0;
}
