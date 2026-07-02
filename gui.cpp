#include <ncurses.h>
#include "include/gui.h"
#include "include/html_parser.h"

#define LOGO_HEIGHT 6


std::string selectedQuestionSlug;


std::vector<std::string> wrapText(const std::string &text, int width)
{
    std::vector<std::string> result;
    if (width <= 0) width = 1;

   
    std::stringstream ss(text);
    std::string paragraph;

    while (std::getline(ss, paragraph, '\n'))
    {
        if (paragraph.empty())
        {
            result.push_back(""); 
            continue;
        }

     

        std::stringstream words(paragraph);
        std::string word;
        std::string currentLine;

        while (words >> word)
        {
            
            while ((int)word.size() > width)
            {
                if (!currentLine.empty())
                {
                    result.push_back(currentLine);
                    currentLine.clear();
                }
                result.push_back(word.substr(0, width));
                word = word.substr(width);
            }

            if (currentLine.empty())
            {
                currentLine = word;
            }
            else if ((int)(currentLine.size() + 1 + word.size()) <= width)
            {
                currentLine += " " + word;
            }
            else
            {
                result.push_back(currentLine);
                currentLine = word;
            }
        }

        if (!currentLine.empty())
            result.push_back(currentLine);
    }

    return result;
}

void drawLogo()
{
    int screen_width = COLS;
    int logo_width = 60;
    int start_x = (screen_width - logo_width) / 2;

    if (start_x < 0)
        start_x = 0;

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
        if (logo_start_x < 0)
            logo_start_x = 0;
        int list_start_x = (COLS - list_total_width) / 2;
        if (list_start_x < 0)
            list_start_x = 0;

        drawLogo();

        for (int i = offset; i < offset + page_size && i < n; i++)
        {

            if (i == highlight)
            {
                attron(A_REVERSE);
            }
            mvprintw((i - offset) + LOGO_HEIGHT + 1, list_start_x, "%-70s %-15s %-10s", questions[i].title.c_str(), questions[i].difficulty.c_str(), questions[i].status.c_str());
            if (i == highlight)
            {
                attroff(A_REVERSE);
            }
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
        else if (c == '\n' || c == KEY_ENTER)
        {
            selectedQuestionSlug = questions[highlight].titleSlug;
            return SCREEN_QUESTION;
        }
        else if (c == 'q' || c == 'Q')
        {
            return SCREEN_EXIT;
        }
    }
}

Screen questionScreen()
{
    clear();
    refresh();

    int rows, cols, half;
    WINDOW *questionInformation = nullptr, *questionCode = nullptr;
    WINDOW *infoPad = nullptr, *codePad = nullptr;

    std::vector<std::string> infoLines, codeLines;
    int infoPadHeight = 0, codePadHeight = 0;
    int infoScroll = 0, codeScroll = 0;
    int visibleHeight = 0;

    enum FocusedWindow { FOCUS_INFO, FOCUS_CODE };
    FocusedWindow focus = FOCUS_INFO;



    questionDetail detail = getQuestionDetail(selectedQuestionSlug);
    const question &q = detail.questionn;

    std::string cleanContent = stripHtml(q.content);

    std::string infoText = q.title + "\n\n"
                          + "Difficulty: " + q.difficulty + "\n\n"
                          + cleanContent;

    
    std::string codeText;
    for (const auto &snippet : q.codeSnippets)
    {
        if (snippet.langSlug == "cpp")
        {
            codeText = snippet.code;
            break;
        }
    }
    if (codeText.empty() && !q.codeSnippets.empty())
        codeText = q.codeSnippets[0].code;

    
    auto setupWindows = [&]()
    {
        getmaxyx(stdscr, rows, cols);
        half = cols / 2;
        if (half < 10) half = 10;

        if (questionInformation) delwin(questionInformation);
        if (questionCode) delwin(questionCode);
        if (infoPad) delwin(infoPad);
        if (codePad) delwin(codePad);

        questionInformation = newwin(rows, half, 0, 0);
        questionCode = newwin(rows, cols - half, 0, half);

        int infoWidth = std::max(half - 4, 1);
        int codeWidth = std::max(cols - half - 4, 1);

        infoLines = wrapText(infoText, infoWidth);
        codeLines = wrapText(codeText, codeWidth);

        infoPadHeight = std::max((int)infoLines.size(), rows);
        codePadHeight = std::max((int)codeLines.size(), rows);

        infoPad = newpad(infoPadHeight, infoWidth);
        codePad = newpad(codePadHeight, codeWidth);

        for (int i = 0; i < (int)infoLines.size(); i++)
            mvwprintw(infoPad, i, 0, "%s", infoLines[i].c_str());
        for (int i = 0; i < (int)codeLines.size(); i++)
            mvwprintw(codePad, i, 0, "%s", codeLines[i].c_str());

        infoScroll = 0;
        codeScroll = 0;
        visibleHeight = rows - 2;
    };

    auto refreshAll = [&]()
    {
        wattron(questionInformation, focus == FOCUS_INFO ? A_BOLD : A_NORMAL);
        box(questionInformation, 0, 0);
        wattroff(questionInformation, A_BOLD);

        wattron(questionCode, focus == FOCUS_CODE ? A_BOLD : A_NORMAL);
        box(questionCode, 0, 0);
        wattroff(questionCode, A_BOLD);

        wrefresh(questionInformation);
        wrefresh(questionCode);

        prefresh(infoPad, infoScroll, 0, 1, 1, rows - 2, half - 2);
        prefresh(codePad, codeScroll, 0, 1, half + 1, rows - 2, cols - 2);
    };

    setupWindows();
    refreshAll();

    while (1)
    {
        int c = getch();

        if (c == KEY_RESIZE)
        {
            endwin();
            refresh();
            setupWindows();
        }
        else if (c == '\t')
        {
            focus = (focus == FOCUS_INFO) ? FOCUS_CODE : FOCUS_INFO;
        }
        else if (c == KEY_UP)
        {
            int &scroll = (focus == FOCUS_INFO) ? infoScroll : codeScroll;
            if (scroll > 0) scroll--;
        }
        else if (c == KEY_DOWN)
        {
            int &scroll = (focus == FOCUS_INFO) ? infoScroll : codeScroll;
            int maxLines = (focus == FOCUS_INFO) ? infoPadHeight : codePadHeight;
            int maxScroll = std::max(0, maxLines - visibleHeight);
            if (scroll < maxScroll) scroll++;
        }
        else if (c == 'q' || c == 'Q')
        {
            break;
        }

        refreshAll();
    }

    delwin(infoPad);
    delwin(codePad);
    delwin(questionInformation);
    delwin(questionCode);

    return SCREEN_MAIN;
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

        case SCREEN_QUESTION:
            current = questionScreen();
            break;

        default:
            current = SCREEN_EXIT;
            break;
        }
    }
    endwin();
    return 0;
}