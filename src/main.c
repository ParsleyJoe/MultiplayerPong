#include <raylib.h>
#include "../include/game.h"

// Unix socket api
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"
#define MAXDATASIZE 100

#include "../pongserver/pongserver.h"


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in*)sa)->sin_addr);
}

int main(int argc, char *argv[])
{
	// game vars
	struct ball b; // fold here
	b.pos_x = 100;
	b.pos_y = 100;
	b.radius = 20;
	b.speed_x = 300;
	b.speed_y = 300;

	struct paddle pad1;
	pad1.height = 100;
	pad1.width = 30;
	pad1.speed = 250;
	pad1.pos_x = 20;
	pad1.pos_y = 20;
	struct paddle pad2 = pad1;
	int player_id = 0; // is the client p1 or p2?
	
	
	// Client server code
	// ------------------
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2)
	{
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	bool connected = false;
	// looping through linked list and connect to first socket we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		inet_ntop(p->ai_addrlen,
			get_in_addr((struct sockaddr*)p->ai_addr),
			s, sizeof(s));
		printf("client: attempting connection to %s\n", s);

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("client: connect");
			close(sockfd);
			continue;
		}
		connected = true;
		break;
	}

	// Return if no server (for now)
	// --------------
	if (!connected)
	{
		fprintf(stderr, "Couldn't connect to a server\n");
		exit(1);
	}
	inet_ntop(p->ai_family,
		get_in_addr((struct sockaddr*)p->ai_addr),
		s, sizeof(s));
	printf("client: connected to %s\n", s);
	// receive player id
	int n = recv(sockfd, &player_id, sizeof(int), 0);
	if (n == -1)
	{
		perror("recv");
		exit(1);
	}

	freeaddrinfo(servinfo);

	// Don't open window until connected
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(SCR_WIDTH, SCR_HEIGHT, "Pong");
	
	pad2.pos_x = SCR_WIDTH - 40;//(pad1.pos_x * 2);
	while (!WindowShouldClose())
	{
		// Send State
		int num = 0;
		int pos;
		if (player_id == 1)
			pos = pad1.pos_y;
		else
			pos = pad2.pos_y;
		while(num < sizeof(int))
		{
			int n = send(sockfd, &pos + num, sizeof(pos) - num, 0);
			if (n == -1)
			{
				perror("send");
				continue;
			}
			num += n;
		}

		// Get game state from server
		void* data = malloc(sizeof(GameState));
		int numbytes = 0;

		while (numbytes < sizeof(GameState))
		{
			if ((numbytes += recv(sockfd, data, sizeof(GameState), 0)) == -1)
			{
				perror("recv");
				// dont just exit out 
				// WARN: !! RUNNING WITH AN ERROR COULD CAUSE WEIRD THINGS !!
			}
		}

		GameState state;
		state = *(GameState*)data;
		free(data); // gotta remember this bro D:
		pad1.pos_y = state.player_y;
		pad2.pos_y = state.player2_y;
		b.pos_x = state.ball_x;
		b.pos_y = state.ball_y;

		// Update game state
		float dt = GetFrameTime();
		update_game(&b, &pad1, &pad2, dt, player_id);
		BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawText(TextFormat("ID: %d", player_id), 20, 20, 40, BLACK);
			draw_game(&b, &pad1, &pad2);
		EndDrawing();

		// send game state (probably only need to send the player movement)
	}
	
	close(sockfd);
	return 0;
}

