#include <ncurses.h>
#include "include/gui.h"

#define LOGO_HEIGHT 6

void drawLogo()
{
    int screen_width = COLS; 
    int logo_width = 60;
    int start_x = (screen_width - logo_width) / 2;

    
    if (start_x < 0) start_x = 0;

    // Artık '30' yerine 'start_x' kullanıyoruz
    mvprintw(0, start_x, "dP                            dP                             dP ");
    mvprintw(1, start_x, "88                            88                             88 ");
    mvprintw(2, start_x, "88        .d8888b. .d8888b. d8888P .d8888b. 88d8b.d8b. .d888b88 ");
    mvprintw(3, start_x, "88        88ooood8 88ooood8   88   88'  `\"\" 88'`88'`88 88'  `88 ");
    mvprintw(4, start_x, "88        88.  ... 88.  ...   88   88.  ... 88  88  88 88.  .88 ");
    mvprintw(5, start_x, "88888888P `88888P' `88888P'   dP   `88888P' dP  dP  dP `88888P8 ");
}

Screen mainScreen()
{

    std::vector<questionAtList> questions = getAllQuestions();
    int highlight = 0;
    int n = questions.size();

    int offset = 0;
    int page_size = LINES - LOGO_HEIGHT - 2;
    int list_total_width = 95;

    while (1)
    {

        clear();
        
        int logo_width = 60; 
        int logo_start_x = (COLS - logo_width) / 2;
        if (logo_start_x < 0) logo_start_x = 0;
        int list_start_x = (COLS - list_total_width) / 2;
        if (list_start_x < 0) list_start_x = 0;

        drawLogo();

        for (int i = offset; i < offset + page_size && i < n; i++)
        {


            if (i == highlight) {attron(A_REVERSE);}
            mvprintw((i - offset) + LOGO_HEIGHT + 1, list_start_x, "%-70s %-15s %-10s", questions[i].title.c_str(), questions[i].difficulty.c_str(), questions[i].status.c_str());
            if (i == highlight){attroff(A_REVERSE);}
        }

        refresh();

        int c = getch();
        if (c == KEY_UP)
        {
            highlight--;
            if (highlight < 0)
            {
                highlight = n - 1;
                offset = (n > page_size) ? (n - page_size) : 0; 
            }
            else if (highlight < offset)
            {
                offset--; 
            }
        }
        else if (c == KEY_DOWN)
        {
            highlight++;
            if (highlight >= n)
            {
                highlight = 0;
                offset = 0;
            }
            else if (highlight >= offset + page_size)
            {
                offset++;
            }
        }
    }
}

int run_menu()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);

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