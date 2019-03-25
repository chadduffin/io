#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#define COLOR(r,g,b) (16 + 36 * r + 6 * g + b)

struct TERM {
  int rows_;
  int cols_;
#if defined(__APPLE__) || defined(__linux__)
  struct termios termios_;
#elif defined(_WIN32)
  DWORD mode_;
  HANDLE handle_;
#endif
} term;

void window_dimensions(int i) {
  term.rows_ = -1;
  term.cols_ = -1;

#if defined(__APPLE__) || defined(__linux__)
  struct winsize w;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  term.rows_ = w.ws_row;
  term.cols_ = w.ws_col;
#elif defined(_WIN32)
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  term.rows_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  term.cols_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif

  printf("%i, %i\n", term.rows_, term.cols_);
}

char input(void) {
#if defined(__APPLE__) || defined(__linux__)
  char input = 0;

  if (read(1, &input, 1) == -1) {
    return -1;
  }
#elif defined(_WIN32)
  DWORD count = 0;
  TCHAR input = 0;

  ReadConsole(term.handle_, &input, 1, &count, NULL);

  if (count == 0) {
    return -1;
  }
#endif

  return input;
}

void initialize(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("\x1b[?1049h");
  printf("\x1b[2J");
  printf("\x1b[?25l");

#if defined(__APPLE__) || defined(__linux__)
  tcgetattr(1, &term.termios_);

  struct termios termios_tmp = term.termios_;

  termios_tmp.c_lflag = termios_tmp.c_lflag & (~ECHO & ~ICANON);
  tcsetattr(1, TCSANOW, &termios_tmp);
#elif defined(_WIN32)
  term.handle_ = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(term.handle_, &term.mode_);
  SetConsoleMode(term.handle_,
                 term.mode_ & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
#endif

  window_dimensions(-1);
}

void cleanup(void) {
  printf("\x1b[2J");
  printf("\x1b[?1049l");
  printf("\x1b[?25h");

#if defined(__APPLE__) || defined(__linux__)
  tcsetattr(1, TCSANOW, &term.termios_);
#elif defined(_WIN32)
  SetConsoleMode(term.handle_, term.mode_);
#endif
}

void cleanup_and_die(int i) {
  cleanup();
  exit(1);
}

void draw(char c, int x, int y, int fg, int bg) {
  printf("\x1b[%i;%iH", y, x);
  printf("\x1b[38;5;%im", fg);
  printf("\x1b[48;5;%im", bg);
  printf("%c", c);
}

int main(int argc, char** argv) {
  char c = -1;

  atexit(cleanup);
  signal(SIGINT, cleanup_and_die);
  signal(SIGTERM, cleanup_and_die);

#if defined(__APPLE__) || defined(__linux__)
  signal(SIGWINCH, window_dimensions);
#endif

  initialize();

  while ((c = input()) != 'q');

  cleanup();

  return 0;
}

