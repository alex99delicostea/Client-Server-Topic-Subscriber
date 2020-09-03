#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>


#define MAX_CLIENTS	100
#define TOPIC 		50
#define CONTENT     1500
#define BUFLEN      1600
#define CLIENT_ID   10
#define REQUEST     20
#define EXIT_CODE   60000

struct TCP_Request{
	char request_type[REQUEST];
	char topic[TOPIC + 1];
	char SF;
};

char subscribe[12] = "subscribe";
char unsubscribe[12] = "unsubscribe";
char exit_signal[5] = "exit";

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#endif
