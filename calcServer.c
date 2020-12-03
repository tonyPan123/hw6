// Authors:
// Jack Zhang (jzhan237)
// Tony Pan (jpan26)
#include <stdio.h>      /* for snprintf */
#include "csapp.h"
#include "calc.h"

/* buffer size for reading lines of input from user */
#define LINEBUF_SIZE 1024

volatile int shut_down = 0;

struct ConnInfo {
  int clientfd;
  struct Calc *record;
  sem_t *pthread_num;
  sem_t *stop;
};


void *worker(void *arg);

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

  int max_iterms = 99999;
  
  sem_t max_pthread;
 
  sem_init(&max_pthread,0 ,max_iterms);
 
  
  struct Calc *calc = calc_create();
  
  while (!shut_down) {
    int clientfd = Accept(serverfd, NULL, NULL);
    if (clientfd < 0) {
      fatal("Error accepting client connection");
    }

   
    struct ConnInfo *info = malloc(sizeof(struct ConnInfo));
    info->clientfd = clientfd;
    info->record = calc;
    info->pthread_num = &max_pthread;
      
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, worker, info) != 0) {
      fatal("pthread_create failed");
    }
  }
  int sval;
  sem_getvalue(&max_pthread, &sval);
  while (sval != max_iterms) {
    sem_getvalue(&max_pthread, &sval);
  }
  close(serverfd);
  calc_destroy(calc);
  return 0;
}

void *worker(void *arg) {
  struct ConnInfo *info = arg;
  pthread_detach(pthread_self());

  sem_wait(info->pthread_num);
 
  chat_with_client(info->record, info->clientfd, info->clientfd);
  close(info->clientfd);
  free(info);

  sem_post(info->pthread_num);
  
  return NULL;
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
      done = 1;
      shut_down = 1;
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
