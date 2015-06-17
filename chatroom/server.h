#define BACKLOG 10
#define HOSTPORT 4500
#define MAX_MSG 20
#define MSG_LENGTH 128
#define MAX_CLIENT 5
#define CMD_COLOR 'c'
#define CMD_EXIT 'e'
#define CMD_SEND 's'
#define WHITE 1
#define BLUE 2
#define RED 3
#define YELLOW 4
#define GREEN 5
#define INST_WHITE "1 "
#define INST_BLUE "2 "
#define INST_RED "3 "
#define INST_YELLOW "4 "
#define INST_GREEN "5 "
#define NAME_LENGTH 20
#define SERVER_ADDR  "127.0.0.1" 

void *broadcast_func(void *);
void *client_func(void *);

struct chat_client {
	struct chat_client *next, *prev;
	int socketfd;
	struct sockaddr_in address;
	int color;
	char target[20];
	char client_name[NAME_LENGTH];
	pthread_t client_thread;
};

struct client_queue {
	int count;
	struct chat_client *head, *tail;
	sem_t queue_lock;
};

struct chatmsg_queue {
	char *queue[MAX_MSG];
	int head;
	int tail;
	sem_t empty;
	sem_t fill;
	sem_t queue_lock;
};