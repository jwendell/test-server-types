#ifndef UTILS_H_
#define UTILS_H_

#define BUF_SIZE 100
#define SERVER_PORT 10000
#define CONNECTION_BACKLOG 10

int     create_server_socket (void);
int     setup_limits         (void);
void    workout_string       (char *s);

#endif /* UTILS_H_ */
