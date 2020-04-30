#include "console.h"
#include "printf.h"
#include "strings.h"
#include "gl.h"
#include "malloc.h"
#include "font.h"

#define MAX_OUTPUT_LEN 1024

static unsigned int rows;
static unsigned int cols;
static unsigned int curX;
static unsigned int curY;
static char* text;

static void process_char(char ch);

void console_init(unsigned int nrows, unsigned int ncols)
{
    rows = nrows;
		cols = ncols;
		text = malloc((cols + 1) * rows);
		gl_init(cols * font_get_width(), rows * font_get_height(), GL_DOUBLEBUFFER);
		console_clear();
}

void print(void) {
  //clear old text
	gl_clear(GL_BLACK);
  //write console text 
	char (*txt)[cols + 1] = (char (*)[cols + 1]) text;
	int font_height = font_get_height();
	for(int r = 0; r < rows; r++) {
		gl_draw_string(0, r * font_height, txt[r], GL_WHITE);
	}
	gl_swap_buffer();
}

void console_clear(void)
{
		curX = 0;
		curY = 0;
		char (*txt)[cols + 1] = (char (*)[cols + 1]) text;
		for(int r = 0; r < rows; r++) {
			memset(txt[r], ' ', cols);
			memset(txt[r] + cols, '\0', 1);
		}
		print();
}

int console_printf(const char *format, ...)
{
		va_list args;
		va_start(args, format);
		char temp[MAX_OUTPUT_LEN];
		int size = vsnprintf(temp, MAX_OUTPUT_LEN, format, args);
		for(const char* i = temp; *i != '\0'; i++) {
			process_char(*i);
		}
		print();
		return size;
}

static void fix_cur(void) {
	if(curX == cols) {
		curX = 0;
		curY++;
	}
	if(curX == -1) {
		curX = cols - 1;
		curY--;
	}
}

static void vertical_scroll() {
  char (*txt)[cols + 1] = (char (*)[cols + 1]) text;
  //shift rows up
  memcpy(txt[0], txt[1], (cols + 1) * (rows - 1));
  //clear last row
  memset(txt[rows - 1], ' ', cols);
  memset(txt[rows - 1] + cols, '\0', 1);
  curY--;
}

static void process_char(char ch)
{
		char (*txt)[cols + 1] = (char (*)[cols + 1]) text;
		switch (ch) {
			case '\b':
				curX--;
				break;
			case '\r':
				curX = 0;
				break;
			case '\n':
				curX = 0;
				curY++;
				break;
			case '\f':
				console_clear();
				break;
			default:
				txt[curY][curX] = ch;
				curX++;
				break;
		}
    fix_cur();
    if(curY == rows) {
      vertical_scroll();
    }
}
