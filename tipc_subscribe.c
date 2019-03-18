#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif
#include <stdlib.h>
#include "bashansi.h"
#include <stdio.h>
#include <errno.h>

#include "loadables.h"

#include <linux/tipc.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#if !defined (errno)
extern int errno;
#endif

extern char *strerror ();

static int topology_srv = -1;
pthread_t tipc_thread;
static int ofd = STDOUT_FILENO;

#define OUT_MSG(...) dprintf(ofd, __VA_ARGS__)

static int32_t tipc_subscribe(
uint32_t type, uint32_t lower, uint32_t upper,
uint32_t timeout, uint32_t filter, uint8_t handle[8])
{
		struct tipc_subscr sub = {
				.seq.type = htonl(type),
				.seq.lower = htonl(lower),
				.seq.upper = htonl(upper),
				.timeout = htonl(timeout),
				.filter = htonl(filter)
		};
		memcpy(sub.usr_handle, handle, 8);
		if (send(topology_srv, &sub, sizeof(sub), 0) != sizeof(sub)) {
				builtin_error("Failed to subscribe\n");
				return errno;
		}
		return 0;
}
static void tipc_evt(struct tipc_event *e) {
	
	char prefix = '?';
	switch(ntohl(e->event)) {
	case TIPC_PUBLISHED:
		prefix='+';
	break;
	case TIPC_WITHDRAWN:
		prefix='-';
	break;
	case TIPC_SUBSCR_TIMEOUT:
		prefix='*';
	break;
	default:
		prefix='?';
	break;
	}
	OUT_MSG("%c {%u:%u:%u} <%u:%u> [%s]\n",
			prefix,
			ntohl(e->s.seq.type),
			ntohl(e->found_lower),
			ntohl(e->found_upper),
			ntohl(e->port.node),
			ntohl(e->port.ref),
			e->s.usr_handle);
}

static void *tipc_evtloop(void *arg) {

	size_t n;
	struct tipc_event evt = {0};
	while (1) {
		n = recv(topology_srv, &evt, sizeof(evt), 0);
		if (n == sizeof(evt)) {
			tipc_evt(&evt);
		} else {
				break;
		}
	}
}

int tipc_subscribe_builtin (WORD_LIST *list)
{
	int opt;
	uint32_t filter = 0;
	uint32_t timeout = TIPC_WAIT_FOREVER;
	char **args, *sub = NULL, *handle = NULL;
	uint8_t usr_handle[8] = {0};
	uint32_t type, lower, upper;
	char dummy = 0;

	reset_internal_getopt ();
	while ((opt = internal_getopt (list, "u:")) != -1)
	{
		switch (opt) {
		case 't':
			timeout = (uint32_t) strtoul(list_optarg, NULL, 10);
		break;
		case 'u':
			ofd = (int)strtoul(list_optarg, NULL, 10);
		break;
		CASE_HELPOPT;
		default:
			goto usage;
		}
	}
	list = loptend;

	args = strvec_from_word_list (list, 1, 0, (int *)NULL);
	if (!args[0])
		goto usage;
	if ((strncmp(args[0], "cancel", 6) == 0) && args[1]) {
		filter = TIPC_SUB_CANCEL;
		sub = args[1];
		handle = args[2];
	} else if ((strncmp(args[0], "service", 7) == 0) && args[1]) {
		filter = TIPC_SUB_SERVICE;
		sub = args[1];
		handle = args[2];
	} else if ((strncmp(args[0], "port", 4)== 0) && args[1]) {
		filter = TIPC_SUB_PORTS;
		sub = args[1];
		handle = args[2];
	} else {
		/*Assume service subscription*/
		sub = args[0];
		handle = args[1];
	}
	if (handle) {
		if(strlen(handle) > 8) {
			builtin_error("Handle cannot exceed 8 chars\n");
			return EXECUTION_FAILURE;
		}
		strncpy(usr_handle, handle, 8);
	}
	if ((sscanf(sub, "%u:%u:%c", &type, &lower, &dummy) == 2) &&
		!dummy) {
			upper=lower;
	}
	else {
		dummy = 0;
		if ((sscanf(sub, "%u:%u:%u%c", &type, &lower, &upper, &dummy) == 3) &&
		!dummy) {
		}
		else {
			goto usage;
		}
	}
	if (lower > upper) {
		builtin_error("Invalid TIPC service range\n");
		return EXECUTION_FAILURE;
	}

	if (tipc_subscribe(type, lower, upper, timeout, filter, usr_handle) == 0)
		return EXECUTION_SUCCESS;
	return EXECUTION_FAILURE;
usage:
	builtin_usage();
	return EX_USAGE;
}

int tipc_subscribe_builtin_load (char* name)
{
	static const struct sockaddr_tipc sa = {
		.family = AF_TIPC,
		.addrtype = TIPC_ADDR_NAME,
		.addr.name.name.type = 1,
		.addr.name.name.instance = 1
	};
	if ((topology_srv = socket(AF_TIPC, SOCK_SEQPACKET, 0)) < 0) {
		builtin_error("%s\n", strerror(errno));
		return 0;
	}
	if (connect(topology_srv, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
		builtin_error("Failed to connect to TIPC topology server\n");
		return 0;
	}
	if (pthread_create(&tipc_thread, NULL, tipc_evtloop, NULL) != 0) {
		builtin_error("Failed to spawn worker thread\n");
		return 0;
	}
	return (1);
}

/* Called when `template' is disabled. */
void
tipc_subscribe_builtin_unload (char *name)
{
	close(topology_srv);
	pthread_join(tipc_thread, NULL);
}

char *tipc_subscribe_doc[] = {
	"Subscribe to TIPC name table updates",
	"",
	"Options:",
	"  -t <sec>\t Timeout for this subscription",
	"  -u <fd>\t File descriptor that name table updates will be written to",
	"",
	"  <type>, <instance>, <lower> and <upper> are integer values matching the",
	"  service that is to be subscribed to.",
	"",
	"  If handle is specified, this will be provided in the generated event when",
	"  the subscription fires. Handles are limited to 8 chars",
	"",
	"  If port/service is omitted, the default behaviour is to subscribe for",
	"  service availability",
	"",
	"Examples:",
	"  Node up/down events:",
	"  tipc_subscribe 0:0:-1",
	"  Service availability:",
	"  tipc_subscribe 1234:56",
	"  Service range availability:",
	"  tipc_subscribe 1234:56:78"
	"  Cancel a subscription:"
	"  tipc_subscribe cancel 1234:56:78",
	"",
	(char *)NULL
};

struct builtin tipc_subscribe_struct = {
	"tipc_subscribe",			/* builtin name */
	tipc_subscribe_builtin,		/* function implementing the builtin */
	BUILTIN_ENABLED,			/* initial flags for builtin */
	tipc_subscribe_doc,			/* array of long documentation strings. */
	"tipc_subscribe [-t timeout] [-u fd] [port|service|cancel] <type>:<instance>|<lower>:<upper> [handle]",
	0						/* reserved for internal use */
};
