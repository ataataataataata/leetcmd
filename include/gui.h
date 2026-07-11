#include <ncurses.h>
#include "leetcode_client.h"

typedef enum {
    SCREEN_MAIN,
    SCREEN_QUESTION,
    SCREEN_SUBMIT,
    SCREEN_RUN,
    SCREEN_EXIT
} Screen;

Screen mainScreen();
void drawLogo();
int run_menu();
Screen questionScreen();
Screen submitScreen();
Screen runScreen();
void initColors();
void configScreen();