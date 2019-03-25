#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

struct {
  int rows_;
  int cols_;
#if defined(__APPLE__) || defined(__linux__)
  struct termios termios_;
#elif defined(_WIN32)
  DWORD mode_;
  HANDLE handle_;
#endif
} term;

void window_dimensions(int) {
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

  read(1, &input, 1);
#elif defined(_WIN32)
  DWORD count = 0;
  TCHAR input = 0;

  ReadConsole(term.handle_, &input, 1, &count, NULL);
#endif

  return input;
}

void initialize(void) {
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
}

void cleanup(void) {
#if defined(__APPLE__) || defined(__linux__)
  tcsetattr(1, TCSANOW, &term.termios_);
#elif defined(_WIN32)
  SetConsoleMode(term.handle_, term.mode_);
#endif
}

void cleanup_and_die(int) {
  cleanup();
  exit(1);
}

int main(int argc, char** argv) {
  atexit(cleanup);
  signal(SIGINT, cleanup_and_die);
  signal(SIGTERM, cleanup_and_die);
  signal(SIGWINCH, window_dimensions);

  initialize();

  while (input() != 'q');

  cleanup();

  return 0;
}

