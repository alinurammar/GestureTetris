#ifndef GAME_H
#define GAME_H
#include "piece.h"
#include "gl.h"
#include <stdbool.h>

#define WIDTH 10
#define HEIGHT 20

piece** piece_map;

char board[HEIGHT][WIDTH];

void game_init(void);

bool is_valid_state(int new_x, int new_y, int new_rot);

bool is_touching(void);

bool is_line(int y);

bool handle_timer(unsigned int pc);

void handle_input(char key);

int random_piece(void);

#endif
