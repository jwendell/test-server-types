#include <stdio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "utils.h"

int
setup_limits (void)
{
  struct rlimit rlim;
  int result, err;

  rlim.rlim_cur = rlim.rlim_max = 100000;
  if ((result = setrlimit (RLIMIT_NOFILE, &rlim)) < 0)
    {
      getrlimit (RLIMIT_NOFILE, &rlim);
      fprintf (
          stderr,
          "Could not raise the limit of open files. Using system default of %lu.\n",
          rlim.rlim_cur);
    }

  rlim.rlim_cur = rlim.rlim_max = 1024 * 1024;
  if ((result = setrlimit (RLIMIT_STACK, &rlim)) < 0)
    {
      getrlimit (RLIMIT_STACK, &rlim);
      fprintf (stderr,
               "Could not raise the stack size. Using system default of %lu.\n",
               rlim.rlim_cur);
    }

  return result;
}

static void
reverse_string (char *s)
{
  int length = strlen (s);
  int c, i, j;

  for (i = 0, j = length - 1; i < j; i++, j--)
    {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
    }
}

static char
rot13_char (char c)
{
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
    return c + 13;
  else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
    return c - 13;
  else
    return c;
}

static void
rot13_string (char *s)
{
  int len = strlen (s);
  int i;

  for (i = 0; i < len; i++)
    s[i] = rot13_char (s[i]);
}

void
workout_string (char *s)
{
  int i;
  for (i = 0; i < 100; i++)
    {
      reverse_string (s);
      rot13_string (s);
    }
}

int
create_server_socket (void)
{
  int fd, yes = 1;
  struct sockaddr_in listen_addr;

  fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    {
      err (1, "socket() failed");
    }

  memset (&listen_addr, 0, sizeof(listen_addr));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  listen_addr.sin_port = htons (SERVER_PORT);

  if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {
      err (1, "setsockopt failed");
    }

  if (bind (fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) < 0)
    {
      err (1, "bind failed");
    }

  if (listen (fd, CONNECTION_BACKLOG) < 0)
    {
      err (1, "listen failed");
    }

  return fd;
}
