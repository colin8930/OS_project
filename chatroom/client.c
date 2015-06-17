/*
* Date: 2015/05/09
* Program: TCP/IP Client Servive
* Version: 1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <curses.h>
#include <pthread.h>

#define MSG_LENGTH 128
#define SERVER_ADDR     "127.0.0.1" 

static char help[] =
"HELP: \n \
send \"/c [x]\"  to change the color of your message. \n\
x = 1, 2.....5 for white, blue, red, yellow and green\n\
send \"/s [user]\"  to change the target of your message. \n\
default is: all.\n\
";

char name[20];
size_t name_len;

int x = 41;
int i = 1;
void* sendMsg(int *socket)
{
	char massage[MSG_LENGTH];
	char ch;
	mvaddstr(0, 0, "Send msg(push ESC to quit, Tab for help):");
	memset(&massage, '\0', sizeof(massage));
	do {
		ch = getch();
		switch (ch) {
		case 127:  /* Backspace */
			if (x > 41) mvaddch(0, --x, ' ');
			move(0, x);
			refresh();
			break;
		case '\n':  /* Enter */
			massage[x - 41] = '\n';
			massage[x - 40 ] = '\0';
			if (send(*socket, massage, sizeof(massage), 0) == -1) {
				perror("client");
				exit(0);
			}
			x = 41;
			mvaddstr(0, 0, "Send msg(push ESC to quit, Tab for help):");
			mvaddstr(0, 41, "\n");
			move(0, 41);
			refresh();
			break;
		case '\t':  /* Tab */
			mvaddstr(i, 0, help);
			refresh();
			i = i + 3;
			if (i >= 23) {
				i = 1;
				clear();
				mvaddstr(0, 0, "Send msg(push ESC to quit, Tab for help):");
				refresh();
			}
			break;
		case 27:  /* ESC */
			if (send(*socket, "/e", sizeof("/e"), 0) == -1) {
				perror("client");
				exit(0);
			}
			endwin();
			printf("Program exit\n");
			exit(1);
			break;
		default:
			mvaddch(0, x, ch);
			refresh();
			massage[x - 41] = ch;
			x++;
			break;
		}
	} while (1);
}


int main(int argc, char* argv[])
{
	int socketfd ;
	int socket_connect ;
	int bufBytes ;
	char msg_buf[MSG_LENGTH] ;
	struct sockaddr_in addr ;
	int sendbytes;

	pthread_t send_msg_thread ;
	initscr();
	noecho();
	start_color();
	/*Socket file descriptor*/
	socketfd = socket(AF_INET, SOCK_STREAM, 0) ;
	if (socketfd == -1) {
		perror("Error!");
		exit(1);
	}

	/*Initialize the socket information*/
	addr.sin_family = AF_INET ;
	addr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &(addr.sin_addr));
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	/*Connect*/
	socket_connect = connect(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) ;
	if (socket_connect == -1) {
		perror("Error");
		exit(1);
	}
	strcpy(name, argv[2]);
	name_len = strlen(name);
	if ((sendbytes = send(socketfd, argv[2], sizeof(argv[2]), 0)) == -1) {
		printf("send\n");
		exit(1);
	}
	/*Send messages*/
	move(0, 0);
	pthread_create(&send_msg_thread, NULL, (void*)&sendMsg, (int*)&socketfd) ;
	/* Color Pair */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_GREEN, COLOR_BLACK);
	/*Receive messages*/
	while (1) {
		int bufBytes = recv(socketfd, msg_buf, MSG_LENGTH, 0);
		if (bufBytes == -1) {
			perror("Error");
			exit(1);
		} else if (bufBytes == 0) {
			endwin();
			printf("Lose connection\n");			
			break;
		} else {
			/* system message for exit */
			if(msg_buf[0] == '0'){
				endwin();
				printf("%s\n", msg_buf+2);
				break;
			}
			msg_buf[bufBytes] = '\0';

			start_color();
			attrset(COLOR_PAIR(msg_buf[0] - 48));
			if(strncmp(msg_buf + 4, name, name_len) == 0) {
				mvaddstr(i, 0, "send form ");
				mvaddstr(i, 10, (msg_buf + 4) + name_len);
				move(0, x);
				attrset(COLOR_PAIR(0));
				refresh();
				i++;
			}
			else if(strncmp(msg_buf + 4, "all", 3) == 0) {
				mvaddstr(i, 0, (msg_buf + 7));
				move(0, x);
				attrset(COLOR_PAIR(0));
				refresh();
				i++;
			} else if(strncmp(msg_buf + 5 + msg_buf[2] - 48, name, name_len) == 0) {
				mvaddstr(i, 0, "send to ");
				char tmp[20];
				strncpy(tmp, msg_buf + 4, (size_t)(msg_buf[2] - 48));
				tmp[msg_buf[2] - 48 ] = '\0';
				mvaddstr(i, 8, tmp);
				mvaddstr(i, 8 + msg_buf[2] - 48, (msg_buf + 5 + name_len + msg_buf[2] - 48));
				move(0, x);
				attrset(COLOR_PAIR(0));
				refresh();
				i++;
			}
			if (i >= 23) {
				i = 1;
				clear();
				mvaddstr(0, 0, "Send msg(push ESC to quit, Tab for help):");
				refresh();
			}
		}
	}
	close(socketfd) ;
	return 0 ;

}

