#include "../include/game.h"

void update_game(struct ball *b, struct paddle *p, struct paddle *p2, float dt, int player_id)
{
	update_ball(b, dt);
	if (player_id == 1)
		update_paddle(p, dt);
	else
		update_paddle(p2, dt);
	check_paddle_collision(p, b);
}

void draw_game(struct ball *b, struct paddle *p, struct paddle *p2)
{
	draw_ball(b);

	draw_paddle(p);
	draw_paddle(p2);
}

void draw_ball(struct ball *b)
{
	DrawCircle(b->pos_x, b->pos_y, b->radius, (Color){25, 230, 100, 255});
}

void draw_paddle(struct paddle *p)
{
	DrawRectangle(p->pos_x, p->pos_y, p->width, p->height, (Color){25, 230, 223, 255});
}

void update_ball(struct ball *b, float dt)
{
	b->pos_x += b->speed_x * dt;
	b->pos_y += b->speed_y * dt;

	if (b->pos_x >= GetScreenWidth() - (b->radius / 2.0f))
	{
		b->speed_x = -b->speed_x;
	}
	if (b->pos_y >= GetScreenHeight() - (b->radius / 2.0f))
	{
		b->speed_y = -b->speed_y;
	}
	if (b->pos_x <= 0)
		b->speed_x = -b->speed_x;
	if (b->pos_y <= 0)
		b->speed_y = -b->speed_y;
}

void update_paddle(struct paddle *p, float dt)
{

	if (IsKeyDown(KEY_UP))
	{
		p->pos_y += -p->speed * dt;
	}
	if (IsKeyDown(KEY_DOWN))
	{
		p->pos_y += p->speed * dt;
	}

	if (p->pos_y < 0)
	{
		p->pos_y = 0;
	}
	if (p->pos_y + p->height >= GetScreenHeight())
	{
		p->pos_y = GetScreenHeight() - p->height;
	}
}

void check_paddle_collision(struct paddle *p, struct ball *b)
{
	if (CheckCollisionCircleRec((Vector2){(float)b->pos_x, (float)b->pos_y}, b->radius, (Rectangle){(float)p->pos_x, (float)p->pos_y, (float)p->width, (float)p->height}))
	{
		b->speed_x = -b->speed_x;
	}
}


