#include <asm-generic/socket.h>
#include <bits/time.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PORT "3490"
#define BACKLOG 10 // pending connection queue will hold

#include "pongserver.h"
#include "../include/game.h"


// Move ball movement and collisions to server
void s_update_ball(struct ball *b, double dt)
{
	b->pos_x += b->speed_x * dt;
	b->pos_y += b->speed_y * dt;

	if (b->pos_x >= SCR_WIDTH - b->radius)
	{
		b->speed_x = -b->speed_x;
		b->pos_x = SCR_WIDTH - b->radius;
	}
	if (b->pos_y >= SCR_HEIGHT - b->radius)
	{
		b->speed_y = -b->speed_y;
		b->pos_y = SCR_HEIGHT - b->radius;
	}
	if (b->pos_x <= 0)
	{
		b->speed_x = -b->speed_x;
		b->pos_x = b->radius;
	}
	if (b->pos_y <= 0)
	{
		b->speed_y = -b->speed_y;
		b->pos_y = b->radius;
	}
}
bool s_check_paddle_collision(struct paddle *p, struct ball *b)
{
	int ballX = b->pos_x - b->radius;
	int ballY = b->pos_y - b->radius;

	bool collisionX = p->pos_x + p->width >= ballX &&
		b->pos_x + b->radius >= p->pos_x;
	bool collisionY = p->pos_y + p->height >= ballY &&
		b->pos_y + b->radius >= p->pos_y;

	return collisionX && collisionY;
}
void s_proccess_collisions(struct paddle* p, struct paddle *p2, struct ball *b)
{
	if (s_check_paddle_collision(p, b))
	{
		b->speed_x = -b->speed_x;
		b->pos_x = (p->pos_x + p->width + 2) + b->radius;
		printf("p1 collided p1 pos: %d\n", p->pos_x);
	}
	else if (s_check_paddle_collision(p2, b))
	{
		b->speed_x = -b->speed_x;
		b->pos_x = (p2->pos_x - 2) - b->radius;
		printf("p2 collided\n");
	}
}

void sigchld_handler(int s)
{
	(void)s;

	int saved_errno = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in*)sa)->sin_addr);
}


int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			exit(1);
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections\n");


	GameState state;
	state.score1 = 69;
	state.player2_y = 300;
	struct ball b;
	b.pos_x = 100;
	b.pos_y = 100;
	b.radius = 20;
	b.speed_x = 300;
	b.speed_y = 300;
	struct paddle pad1, pad2;
	pad1.pos_x = PADDLE1_X;
	pad2.pos_x = PADDLE2_X;
	pad1.height = PADDLE_HEIGHT;
	pad2.height = PADDLE_HEIGHT;
	pad1.width = PADDLE_WIDTH;
	pad2.width = PADDLE_WIDTH;

	int p1_sock = -1, p2_sock = -1;
	const double TICK = 1.0 / 60.0; // using strict tick speed instead of DT

	while(1)
	{
		sin_size = sizeof(their_addr);
	
		if (p1_sock == -1)
		{
			printf("waiting for p1");
			fflush(stdout);
			p1_sock = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (p1_sock == -1)
			{
				perror("accept");
				continue;
			}
			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof(s));
			printf("server: p1 got connection from: %s\n", s);
			fflush(stdout);
			// send player_id
			int id = 1;
			int i = send(p1_sock, &id, sizeof(int), 0);
			if (i == -1)
			{
				perror("send");
			}
		}
		else if (p2_sock == -1)
		{
			printf("waiting for p2");
			fflush(stdout);

			p2_sock = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (p2_sock == -1)
			{
				perror("accept");
				continue;
			}
			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof(s));
			printf("server: p2 got connection from: %s\n", s);
			fflush(stdout);

			int id = 2;
			int i = send(p2_sock, &id, sizeof(int), 0);
			if (i == -1)
			{
				perror("send");
			}

		}
		
		// goes way too fast without sleep
		//sleep(1);
		s_update_ball(&b, TICK); // works better with TICK anyway

		// no need to fork or send if no clients
		if (p1_sock == -1 || p2_sock == -1)
		{
			continue;
		}
		// get pos from both players, ONLY THE Y AXIS IS SENT AND RECEIVED
		int pos1, pos2;
		void* data = malloc(sizeof(int));
		int num = 0;
		while (num < sizeof(int))
		{
			num += recv(p1_sock, data + num, sizeof(int) + num, 0);
		}
		pos1 = *(int*)data;

		num = 0;
		while(num < sizeof(int))
		{
			num += recv(p2_sock, data + num, sizeof(int) + num, 0);
		}
		pos2 = *(int*)data;
		free(data);


		// Check collisions with the player and ball, update score, modify game state
		// and broadcast to clients
		pad1.pos_y = pos1;
		pad2.pos_y = pos2;

		s_proccess_collisions(&pad1, &pad2, &b);
		state.ball_x = b.pos_x;
		state.ball_y = b.pos_y;


		int numbytes = 0;
		while (numbytes < sizeof(GameState))
		{

			state.player_y = pos1;
			state.player2_y = pos2;
			
			// Send out same validated state to both clients
			int s = send(p1_sock, (void*)&state + numbytes, sizeof(GameState) - numbytes, 0);
			if (s == -1)
			{
				perror("p1send");
				break;
			}
			numbytes += s;
		}

		numbytes = 0;
		while (numbytes < sizeof(GameState))
		{

			// Send out same validated state to both clients
			int s = send(p2_sock, (void*)&state + numbytes, sizeof(GameState) - numbytes, 0);
			if (s == -1)
			{
				perror("p2send");
				break;
			}
			numbytes += s;
		}
	}
	close(p1_sock);
	close(p2_sock);
	close(sockfd);

	return 0;
}
