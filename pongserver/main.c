#include <asm-generic/socket.h>
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

#define PORT "3490"
#define BACKLOG 10 // pending connection queue will hold

#include "pongserver.h"
#include "../include/game.h"

// Move ball movement and collisions to server

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

	int p1_sock = -1, p2_sock = -1;
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

		// no need to fork or send if no clients
		if (p1_sock == -1 || p2_sock == -1)
		{
			continue;
		}
		if (!fork())
		{
			close(sockfd);

			// get pos from both players
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

			int numbytes = 0;
			while (numbytes < sizeof(GameState))
			{

				// receive states from both clients, !! SKIPPING SERIALIZATION FOR NOW (inet_ntop, etc.) !!
				// Validate and process states (Scores and collision)
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

				// receive states from both clients, !! SKIPPING SERIALIZATION FOR NOW (inet_ntop, etc.) !!
				// Validate and process states (Scores and collision)
				//
				
				// Send out same validated state to both clients
				int s = send(p2_sock, (void*)&state + numbytes, sizeof(GameState) - numbytes, 0);
				if (s == -1)
				{
					perror("p2send");
					break;
				}
				numbytes += s;
			}
			exit(0);
		}
	}

	close(p1_sock);
	close(p2_sock);

	return 0;
}
