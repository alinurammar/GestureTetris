#include "game.h"
#include "ps2.h"
#include "interrupts.h"
#include "armtimer.h"
#include "render.h"
#include "strings.h"
#include "piece.h"
#include "gl.h"
#include "malloc.h"
#include "timer.h"

//pieces
piece I = {0xFF00FF00, {I_one, I_two, I_three, I_four}};
piece Z = {0xFF00FF00, {Z_one, Z_two, Z_three, Z_four}};
piece J = {0xFFFF0000, {J_one, J_two, J_three, J_four}};
piece L = {0xFF0000AA, {L_one, L_two, L_three, L_four}};
piece O = {0xFF00FFFF, {O_one, O_two, O_three, O_four}};
piece S = {0xFF00FF00, {S_one, S_two, S_three, S_four}};
piece T = {0xFFFF00FF, {T_one, T_two, T_three, T_four}};

//game state
piece_state cur_piece;
int next_piece;

static int fall_interval = 500000;

void game_init(void) {
  graphics_init();
  piece_map = malloc(7 * 4);
  piece_map[0] = &I;
  piece_map[1] = &Z;
  piece_map[2] = &J;
  piece_map[3] = &L;
  piece_map[4] = &O;
  piece_map[5] = &S;
  piece_map[6] = &T;

  cur_piece.num = random_piece();
  next_piece = random_piece();
  cur_piece.rot = 0;
  cur_piece.x = 5;
  cur_piece.y = 20;
  memset(board, 0, WIDTH * HEIGHT);
  armtimer_init(fall_interval);
  armtimer_enable();
  armtimer_enable_interrupts();
  interrupts_attach_handler(handle_timer, INTERRUPTS_BASIC_ARM_TIMER_IRQ);
}

//ammar
bool is_valid_state(int new_x, int new_y, int new_rot) {
  for(int piece_x = 0; piece_x < 4; piece_x++) {
    for(int piece_y = 0; piece_y < 4; piece_y++) {
      bool piece = piece_map[cur_piece.num]->states[cur_piece.rot][piece_x][piece_y];
      //Check if touches other piece on board
      if(piece && (board[new_x + piece_x][new_y + piece_y])){
        printf("touch %d", board[new_x + piece_x][new_y + piece_y]);
        return false;
      }
      //Check vertical bounds
      if(piece && (new_y + piece_y >= HEIGHT || new_y + piece_y < 0)){
          printf("vert");
          return false;
      }
      //Check horizontal bounds
      if(piece && (new_x + piece_x >= WIDTH || new_x + piece_x < 0)){
          printf("horz");
          return false;
      }
    }
  }
  return true;
}

//bay
bool is_touching(void) {
  for(int piece_x = 0; piece_x < 4; piece_x++) {
    for(int piece_y = 0; piece_y < 4; piece_y++) {
      if(piece_map[cur_piece.num]->states[cur_piece.rot][piece_y][piece_x]
        && (board[cur_piece.y + 1 + piece_y][cur_piece.x + piece_x] || cur_piece.y + piece_y + 1 >= HEIGHT)) {
          return true;
      }
    }
  }
  return false;
}
//ammar
//Checks if a line has been created at the given y value using the board
bool is_line(int y) {
    for(int i = 0; i < WIDTH; i++){
        if(board[y][i] == 0)
            return false;
    }
    return true;
}

//Bakes cur_piece into the board and updates cur_piece to be next_piece and assigns new next_piece
void bake(void) {
  for(int piece_x = 0; piece_x < 4; piece_x++) {
    for(int piece_y = 0; piece_y < 4; piece_y++) {
      if(piece_map[cur_piece.num]->states[cur_piece.rot][piece_y][piece_x]) {
        board[cur_piece.y + piece_y][cur_piece.x + piece_x] = cur_piece.num + 1;
      }
    }
  }
  cur_piece.num = next_piece;
  next_piece = random_piece();
}

//bay
bool handle_timer(unsigned int pc) {
  if(armtimer_check_and_clear_interrupt()) {
    if(is_valid_state(cur_piece.x, cur_piece.y + 1, cur_piece.rot)) {
      printf("moved down\n");
      cur_piece.y++;
      if(is_touching()) {
        //bake shape into board
        if(cur_piece.y < 2) {
          printf("GAME OVER\n");
          while(1);
        }
        bake();
        draw_board();

        cur_piece.y = 0;
        //make next shape into current shape resetting x and y to top
        //randomly choose next shape

        if(is_line(cur_piece.y)) {
          //clear line
        }
      }
    } else {
      bake();
      draw_board();
    }
    draw_piece(cur_piece);

    return true;
  }
  return false;
}

int random_piece(void) {
  //really bad randomness probably
  double rand = timer_get_ticks() & 0b111;
  return (7.0 / 8.0) * rand;
}

//ammar
void handle_input(char key) {
  switch (key) {
    case PS2_KEY_ARROW_LEFT:
      cur_piece.x--;
      break;
    case PS2_KEY_ARROW_RIGHT:
      cur_piece.x++;
      break;
    case PS2_KEY_ARROW_DOWN:
      break;
    case PS2_KEY_ARROW_UP:
      cur_piece.rot = cur_piece.rot + 1 % 4;
      break;
  }
}
