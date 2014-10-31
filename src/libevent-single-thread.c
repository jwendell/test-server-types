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

#include "utils.h"

static void
signal_cb (evutil_socket_t sig, short events, void *user_data)
{
  struct event_base *base = user_data;
  event_base_loopbreak (base);
}

typedef struct
{
  int fd;
  char buf[BUF_SIZE];
  struct event_base *evbase;
  struct event *read_event;
  struct event *timeout_event;
} Data;

static void
close_connection (Data *data)
{
  printf ("closing %d\n", data->fd);
  close (data->fd);
  event_free (data->read_event);
  event_free (data->timeout_event);
  free (data);
}

static void
read_cb (evutil_socket_t fd, short events, void *arg)
{
  Data *data = arg;
  struct timeval t =
    { 1, 0 };

  int len = read (fd, data->buf, BUF_SIZE - 1);
  if (len > 0)
    {
      data->buf[len] = '\0';
      event_add (data->timeout_event, &t);
    }
  else
    {
      close_connection (data);
    }
}

static void
timeout_cb (evutil_socket_t fd, short events, void *arg)
{
  Data *data = arg;
  int n_written, len = strlen (data->buf);

  event_del (data->timeout_event);
  workout_string (data->buf);

  n_written = write (data->fd, data->buf, len);
  if (n_written < len)
    {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
          fprintf (stderr, "Error writing on fd %d\n", data->fd);
          close_connection (data);
        }
    }
}

static void
accept_cb (int listener, short event, void *arg)
{
  struct event_base *base = arg;
  struct event *read_event;
  Data *data;

  int fd = accept (listener, NULL, NULL);
  if (fd < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;
      err (1, "accept");
    }

  evutil_make_socket_nonblocking (fd);

  data = malloc (sizeof(Data));
  read_event = event_new (base, fd, EV_READ | EV_PERSIST, read_cb, data);
  data->fd = fd;
  data->evbase = base;
  data->read_event = read_event;
  data->timeout_event = evtimer_new(base, timeout_cb, data);

  if (!read_event || event_add (read_event, NULL) < 0)
    err (1, "Error creating a new read_event.");
}

int
main (int argc, char **argv)
{
  struct event_base *base;
  struct event *signal_event, *accept_event;
  int fd, opt, n_processes = 1;

  while ((opt = getopt (argc, argv, "p:")) != -1)
    {
      switch (opt)
        {
        case 'p':
          n_processes = atoi (optarg);
          break;
        default:
          fprintf (stderr, "Usage: %s [-p number_of_processes (default: 1)] \n",
                   argv[0]);
          exit (EXIT_FAILURE);
        }
    }

  setup_limits ();

  base = event_base_new ();
  if (!base)
    err (1, "Could not initialize libevent!");

  fd = create_server_socket ();
  evutil_make_socket_nonblocking (fd);

  signal (SIGPIPE, SIG_IGN);

  accept_event = event_new (base, fd, EV_READ | EV_PERSIST, accept_cb,
                            (void *) base);
  if (!accept_event || event_add (accept_event, NULL) < 0)
    err (1, "Error creating a new event.");

  signal_event = evsignal_new(base, SIGINT, signal_cb, (void * )base);
  if (!signal_event || event_add (signal_event, NULL) < 0)
    err (1, "Could not create/add a signal event!");

  if (n_processes > 1)
    {
      int i;
      pid_t *pids = malloc (n_processes * sizeof(pid_t));

      printf ("Server running with %d processes.\n", n_processes);
      for (i = 0; i < n_processes; i++)
        {
          if ((pids[i] = fork ()) == 0)
            {
              event_reinit (base);
              event_base_dispatch (base);
              goto out;
            }
        }

      close (fd);
      for (i = 0; i < n_processes; i++)
        {
          waitpid (pids[i], NULL, 0);
        }
    }
  else
    {
      printf ("Server running with 1 process.\n");
      event_base_dispatch (base);
    }

  out: event_free (signal_event);
  event_free (accept_event);
  event_base_free (base);

  printf ("Goodbye\n");
  return 0;
}
