#include "render.h"
#include "gl.h"
#include "printf.h"
#include "game.h"

piece_state prev_piece;

void draw_board(void) {
  for(int y = 0; y < 20; y++) {
    printf("|");
    for(int x = 0; x < 10; x++) {
      if(board[y][x]) {
        printf("O");
      } else {
        printf(" ");
      }
    }
    printf("|\n");
  }
  printf(" ----------\n");

  for(int y = 0; y < 20; y++) {
    for(int x = 0; x < 10; x++) {
      if(board[y][x]) {
        gl_draw_rect(x * 50, y * 50, 50,  50, piece_map[(int) board[y][x] - 1]->color);
      }
    }
  }
  gl_swap_buffer();
}

void draw_piece(piece_state piece) {
  for(int piece_x = 0; piece_x < 4; piece_x++) {
    for(int piece_y = 0; piece_y < 4; piece_y++) {
      if(piece_map[piece.num]->states[piece.rot][piece_y][piece_x]) {
        gl_draw_rect((piece.x + piece_x) * 50, (piece.y + piece_y) * 50, 50,  50, piece_map[piece.num]->color);
      }
    }
  }
  gl_swap_buffer();
  //clear previous piece in draw buffer
  for(int piece_x = 0; piece_x < 4; piece_x++) {
    for(int piece_y = 0; piece_y < 4; piece_y++) {
      if(piece_map[prev_piece.num]->states[prev_piece.rot][piece_y][piece_x]) {
        gl_draw_rect((prev_piece.x + piece_x) * 50, (prev_piece.y + piece_y) * 50, 50,  50, GL_BLACK);
      }
    }
  }
  // prev_piece.num = piece.num;
  // prev_piece.x = piece.x;
  // prev_piece.y = piece.y;
  // prev_piece.rot = piece.rot;
  prev_piece = piece;
}


void graphics_init(void) {
  gl_init(WIDTH * 50, HEIGHT * 50, GL_DOUBLEBUFFER);
}
