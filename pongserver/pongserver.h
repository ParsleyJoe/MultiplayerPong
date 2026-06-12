#ifndef PONGSERVER
#define PONGSERVER

struct GameState {
	int player_y, player2_y; // don't need X in the server packet
	int score1, score2;
	int ball_x, ball_y;
} typedef GameState;


#endif
