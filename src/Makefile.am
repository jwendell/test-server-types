QUEUE = workqueue.c workqueue.h
COMMON = utils.h utils.c

bin_PROGRAMS = \
	one-thread-connection \
	client \
	pool-threads-libevent \
	libevent-single-thread

one_thread_connection_SOURCES = one-thread-connection.c $(COMMON)
one_thread_connection_LDFLAGS = -pthread

client_LDADD = -levent
client_SOURCES = client.c $(COMMON)

pool_threads_libevent_LDADD   = -levent -levent_pthreads
pool_threads_libevent_LDFLAGS = -pthread
pool_threads_libevent_SOURCES = pool-threads-libevent.c $(QUEUE) $(COMMON)

libevent_single_thread_LDADD  = -levent
libevent_single_thread_SOURCES = libevent-single-thread.c $(COMMON)

-include $(top_srcdir)/git.mk
