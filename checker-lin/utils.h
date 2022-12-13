#ifndef UTILS_H
#define UTILS_H_

#include "errno.h"
#include "stdlib.h"

#include "util/so_scheduler.h"

/* Defensive programming macro */
#define DIE(assertion, call_description)		\
	do {										\
		if (assertion) {						\
			fprintf(stderr, "(%s, %d): ",		\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);						\
		}										\
	} while (0)

#define uint unsigned int

/* Boolean type declaration */
#define bool unsigned char
#define TRUE 1
#define FALSE 0

#define ERROR -1
#define SUCCESS 0

#define EMPTY -1

#define BASIC_PRIO 0

#define NO_TIME 0

#endif
