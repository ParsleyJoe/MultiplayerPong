#ifndef GAME
#define GAME

#include <raylib.h>
#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define PADDLE_WIDTH 30
#define PADDLE_HEIGHT 100
#define PADDLE1_X 20
#define PADDLE2_X SCR_WIDTH - 40;

struct ball
{
	int pos_x, pos_y;
	float radius;
	int speed_x, speed_y;
};

struct paddle
{
	int pos_x, pos_y;
	int width, height;
	int speed;
};


void draw_ball(struct ball *b);
void draw_paddle(struct paddle *p);
void update_ball(struct ball *b, float dt);
void update_paddle(struct paddle *p, float dt);
void check_paddle_collision(struct paddle *p, struct ball *b);

void draw_game(struct ball *b, struct paddle *p, struct paddle *p2);
void update_game(struct ball *b, struct paddle *p, struct paddle *p2, float dt, int player_id);

#endif
