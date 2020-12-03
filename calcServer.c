// Authors:
// Jack Zhang (jzhan237)
// Tony Pan (jpan26)
#include <stdio.h>      /* for snprintf */
#include "csapp.h"
#include "calc.h"

/* buffer size for reading lines of input from user */
#define LINEBUF_SIZE 1024

void chat_with_client(struct Calc *calc, int infd, int outfd);

/*
 * fatal error - terminate program
 */
void fatal() {
	fprintf(stderr, "Error\n");
	exit(1);
}

int main(int argc, char **argv) {
	if (argc != 2) fatal(); // takes one command line argument
	const char *port = argv[1];

	int serverfd = open_listenfd((char*) port); // create server socket
	if (serverfd < 0) fatal();

	struct Calc *calc = calc_create();

	while (1) {
		int clientfd = Accept(serverfd, NULL, NULL); // wait for client to connect
		if (clientfd < 0) {
			calc_destroy(calc);
			fatal();
		}
		/* chat with client using standard input and standard output */
		chat_with_client(calc, clientfd, clientfd);
		close(clientfd);
	}

	calc_destroy(calc);
	return 0;
}

void chat_with_client(struct Calc *calc, int infd, int outfd) {
	rio_t in;
	char linebuf[LINEBUF_SIZE];

	/* wrap input */
	rio_readinitb(&in, infd);

	/*
	 * Read lines of input, evaluate them as calculator expressions,
	 * and (if evaluation was successful) print the result of each
	 * expression.
	 * 
	 * quit - terminate the client
	 * shutdown - terminate both the client and the server
	 */
	int done = 0;
	while (!done) {
		ssize_t n = rio_readlineb(&in, linebuf, LINEBUF_SIZE);
		if (n <= 0) {
			/* error or end of input */
			done = 1;
		} else if (strcmp(linebuf, "shutdown\n") == 0 || strcmp(linebuf, "shutdown\r\n") == 0) {
			/* shutdown */
			calc_destroy(calc);
			exit(0);
		} else if (strcmp(linebuf, "quit\n") == 0 || strcmp(linebuf, "quit\r\n") == 0) {
			/* quit command */
			done = 1;
		} else {
			/* process input line */
			int result;
			if (calc_eval(calc, linebuf, &result) == 0) {
				/* expression couldn't be evaluated */
				rio_writen(outfd, "Error\n", 6);
			} else {
				/* output result */
				int len = snprintf(linebuf, LINEBUF_SIZE, "%d\n", result);
				if (len < LINEBUF_SIZE) {
					rio_writen(outfd, linebuf, len);
				}
			}
		}
	}
}