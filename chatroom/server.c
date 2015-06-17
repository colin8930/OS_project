#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"


struct chatmsg_queue chatmsgQueue;
struct client_queue clientQueue;
pthread_t broadcast_thread;


struct sockaddr_in address;
int port = HOSTPORT;
int sockfd;
struct sockaddr_in their_addr;
socklen_t sin_size;

struct chatmsg_queue *msgQueue = &chatmsgQueue;

sem_t *buf_fill = &chatmsgQueue.fill;
sem_t *buf_empty = &chatmsgQueue.empty;
sem_t *mq_lock = &chatmsgQueue.queue_lock;  //message qeueu lock
sem_t *cq_lock = &clientQueue.queue_lock;  //client queue lock

int main(int argc, char **argv)
{
	/* init */
	int i = 0;
	while (i < MAX_MSG) {
		msgQueue->queue[i] = (char *)malloc(sizeof(char) * MSG_LENGTH);
		i++;
	}

	memset(&clientQueue, 0, sizeof(struct client_queue)); /* init queue */

	/* Socket file descriptor */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* Initialize the socket information */
	address.sin_family = PF_INET;
	address.sin_port = htons(port);
	inet_aton(SERVER_ADDR, &address.sin_addr);
	memset(&(address.sin_zero), '\0', 8);

	if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		perror("bind");
		exit(1);
	}

	printf("Default port: %d\n", port);

	/* init semaphore */
	sem_init(buf_fill, 0, 0);
	sem_init(buf_empty, 0, MAX_MSG);
	sem_init(mq_lock, 0, 1);
	sem_init(cq_lock, 0, 1);

	/* create broadcast thread */
	pthread_create(&(broadcast_thread), NULL, (void *)(*broadcast_func), (void *)(msgQueue));

	while (1) {
		int new_fd;
		char input[20];
		char clientName [NAME_LENGTH];
		if (listen(sockfd, BACKLOG) == -1) {
			printf("listen");
			exit(1);
		}
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			printf("accept");
			exit(1);
		}
		memset(&input, 0, sizeof(input));
		if (recv(new_fd, input, sizeof(input), 0) == -1) {
			printf("recv error");
			exit(1);
		}
		printf("%s login.\n", input);

		/*  first get is username */
		sem_wait(cq_lock);
		if (clientQueue.count >= MAX_CLIENT) {
			sem_post(cq_lock);
			if (send(new_fd, "0 the room is full", sizeof("0 the room is full"), 0) == -1) {
				perror("Server socket sending error");
				exit(1);
			}
			close(new_fd);
			continue;
		}
		sem_post(cq_lock);

		/* check usename */
		int check = 1;
		sem_wait(cq_lock);
		struct chat_client *tmp = clientQueue.head;

		for(; tmp; tmp = tmp -> next){
			if (strcmp(tmp -> client_name , input) == 0) {
				check = 0;
				break;
			}
		}
		sem_post(cq_lock);

		if (check == 0) {
			printf("%s exists\n", input);
			if (send(new_fd, "0 name existed", sizeof("0 name existed"), 0) == -1) {
				perror("Server socket sending error");
				exit(1);
			}
			close(new_fd);
			continue;
		}

		struct chat_client *newClient;
		newClient = (struct chat_client *)malloc(sizeof(struct chat_client));
		memset(newClient, 0, sizeof(struct chat_client));
		newClient -> socketfd = new_fd;
		newClient -> address = their_addr;
		newClient -> color = WHITE;
		strcpy(newClient -> target , "all");
		strcpy(newClient -> client_name, input);
		pthread_create(&(newClient -> client_thread), NULL, (void *)(*client_func), (void *)(newClient));
	}

	return 0;
}

void *client_func(void *arg)
{
	struct chat_client *client;
	client = arg;

	/* add the client */
	sem_wait(cq_lock);
	if (clientQueue.tail != NULL) {
		clientQueue.tail -> next = client;
		client -> prev = clientQueue.tail;
		clientQueue.tail = client;
	} else {
		clientQueue.head = client;
		clientQueue.tail = client;
		clientQueue.head -> next = NULL;
		clientQueue.tail -> prev = NULL;
	}
	clientQueue.count ++;
	sem_post(cq_lock);

	char mbuf[MSG_LENGTH];	/* for received*/
	char content[MSG_LENGTH];      /* for send */
	int new_fd = client -> socketfd;

	while (1) {

		/* receive from client */
		memset(&mbuf, 0, sizeof(mbuf));
		if (recv(new_fd, &mbuf, MSG_LENGTH, 0) == -1) {
			perror("recv error occurs, exit");
			exit(1);
		}
		mbuf[MSG_LENGTH - 1] = 0;
		int color;
		char target[20];
		char tmp[20];
		/* command handler*/
		if (mbuf[0] == '/') {
			if (mbuf[1] == CMD_COLOR) {
				/* set color*/
				printf("%s color change: %s",client -> client_name , mbuf);
				if (sscanf(mbuf, "%s %d", tmp, &color) != 2) break;
				client -> color = color;
			} else if (mbuf[1] == CMD_SEND) {
				/* set target*/
				printf("%s target change: %s",client -> client_name , mbuf);
				if (sscanf(mbuf, "%s %s", tmp, target) != 2) break;
				strcpy(client -> target, target);

			} else if (mbuf[1] == CMD_EXIT) {
				/* exit */

				memset(&content, '\0', sizeof(content));
				strcat(content, "1 3 all client ");
				strcat(content, client -> client_name);
				strcat(content, " exit");
				close(new_fd);

				/* remove the client in the queue*/
				sem_wait(cq_lock); /* lock*/
				if ((client -> prev) && (client -> next)) {
					client -> next -> prev = client -> prev;
					client -> prev -> next = client -> next;
				} else if ((!client -> next) && (client -> prev)) {
					client -> prev -> next = NULL;
					clientQueue.tail = client -> prev;
				} else if ((!client -> prev) && (client -> next)) {
					client -> next -> prev = NULL;
					clientQueue.head = client -> next;
				} else {
					clientQueue.head = NULL;
					clientQueue.tail = NULL;
				}
				sem_post(cq_lock);  /* release lock */

				sem_wait(buf_empty);
				sem_wait(mq_lock);
				strcpy(msgQueue->queue[msgQueue->tail], content);	/* put to the queue */
				msgQueue->tail = (msgQueue->tail + 1) % MAX_MSG;
				sem_post(mq_lock);
				sem_post(buf_fill);

				printf("client [%s] exit \n", client -> client_name);
				pthread_detach(pthread_self());
				free(client);
				pthread_exit(0);
			}
		} else {
			/* send */
			memset(&content, '\0', sizeof(content));
			switch (client -> color) {
			case WHITE:
				strcat(content, INST_WHITE);
				break;
			case BLUE:
				strcat(content, INST_BLUE);
				break;
			case RED:
				strcat(content, INST_RED);
				break;
			case YELLOW:
				strcat(content, INST_YELLOW);
				break;
			case GREEN:
				strcat(content, INST_GREEN);
				break;
			default:
				break;
			}
			char len[2] = "  ";
			len[0] = strlen(client -> target) + 48;
			strcat(content, len);
			strcat(content, client -> target);
			strcat(content, " ");
			strcat(content, client -> client_name);  /* put the username */
			strcat(content, " : ");
			strcat(content, mbuf); /* put the message */
			printf("%s send : %s",client -> client_name , content);
			/* If there are no empty, wait */
			sem_wait(buf_empty);
			sem_wait(mq_lock);
			strcpy(msgQueue->queue[msgQueue->tail], content);	/* put on the queue */
			msgQueue->tail = (msgQueue->tail + 1) % MAX_MSG;
			sem_post(mq_lock);
			/* Increment the number of filled sem */
			sem_post(buf_fill);
		}
	}
}


void *broadcast_func(void *arg)
{

	while (1) {
		char content[MSG_LENGTH];
		memset(&content, '\0', sizeof(content));

		/* If there are no element filled, wait */
		sem_wait(buf_fill);
		sem_wait(mq_lock);
		strcpy(content, msgQueue->queue[msgQueue->head]);

		sem_wait(cq_lock);
		struct chat_client *tmp = clientQueue.head;

		/* send to all client */
		for(; tmp; tmp = tmp -> next){
			if (send(tmp -> socketfd, &content, sizeof(content), 0) == -1) {
				perror("Server socket sending error");
				exit(0);
			}
		}
		sem_post(cq_lock);
		msgQueue->head = (msgQueue->head + 1) % MAX_MSG;
		sem_post(mq_lock);
		/* Increment the number of empty */
		sem_post(buf_empty);
	}
}