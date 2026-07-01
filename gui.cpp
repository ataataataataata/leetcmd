#include <ncurses.h>
#include "include/gui.h"

#define LOGO_HEIGHT 6

void drawLogo()
{
    mvprintw(0, 30, "dP                            dP                             dP ");
    mvprintw(1, 30, "88                            88                             88 ");
    mvprintw(2, 30, "88        .d8888b. .d8888b. d8888P .d8888b. 88d8b.d8b. .d888b88 ");
    mvprintw(3, 30, "88        88ooood8 88ooood8   88   88'  `\"\" 88'`88'`88 88'  `88 ");
    mvprintw(4, 30, "88        88.  ... 88.  ...   88   88.  ... 88  88  88 88.  .88 ");
    mvprintw(5, 30, "88888888P `88888P' `88888P'   dP   `88888P' dP  dP  dP `88888P8 ");
}

Screen mainScreen()
{


    std::vector<questionAtList> questions = getAllQuestions();
    int highlight = 0;
    int n = questions.size();

    while (1)
    {
        clear();
        drawLogo();

        for (int i = 0; i < n; i++)
        {
            if (i == highlight){attron(A_REVERSE);}
                
            mvprintw(i + LOGO_HEIGHT + 1, 0, "%-70s %-15s %-10s", questions[i].title.c_str(),questions[i].difficulty.c_str(), questions[i].status.c_str());
            
            if (i == highlight){attroff(A_REVERSE);}
                
        }
        refresh();

        int c = getch();
        if (c == KEY_UP)
        {
            highlight--;
            if (highlight < 0)
                highlight = n - 1;
        }
        else if (c == KEY_DOWN)
        {
            highlight++;
            if (highlight >= n)
                highlight = 0;
        }
        else if (c == '\n')
        {
            return SCREEN_MAIN;
        }
    }
}

int run_menu()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
 
    Screen current = SCREEN_MAIN;

    while (current != SCREEN_EXIT)
    {
        switch (current)
        {
        case SCREEN_MAIN:
            current = mainScreen();
            break;
        default:
            current = SCREEN_EXIT;
            break;
        }
    }
    endwin();
    return 0;
}