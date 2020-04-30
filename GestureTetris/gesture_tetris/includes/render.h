#ifndef RENDER_H
#define RENDER_H

#include "piece.h"
#include "gl.h"
#include "game.h"

void render_init(void);

void draw_board(void);

void draw_piece(piece_state piece);

void graphics_init(void);

#endif
