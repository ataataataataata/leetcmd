#include <ncurses.h>
#include <ctype.h>
#include "include/gui.h"
#include "include/html_parser.h"
#include "include/editor.h"
#include "include/leetcode_client.h"
#include "include/config_manager.h"
#include <fstream>

#define LOGO_HEIGHT 6
#define LOGO_SPLIT_COL 34 // column where "leet" and "cmd" parts split (tune to your font)

// --- Color pairs ---
#define PAIR_EASY 1
#define PAIR_MEDIUM 2
#define PAIR_HARD 3
#define PAIR_LOGO_LEET 4
#define PAIR_LOGO_CMD 5
#define PAIR_SUBMIT_OK 6
#define PAIR_SUBMIT_FAIL 7
#define PAIR_CONFIG_BORDER 8
#define PAIR_CONFIG_LABEL 9

std::string selectedQuestionSlug;
std::string selectedQuestionId; // needed by submitCode() as question_id
std::string selectedLangSlug = "cpp";
std::string lastSubmittedCode; // code passed forward into the submit screen

void initColors()
{
    start_color();
    use_default_colors();

    init_pair(PAIR_EASY, COLOR_GREEN, -1);
    init_pair(PAIR_MEDIUM, COLOR_YELLOW, -1);
    init_pair(PAIR_HARD, COLOR_RED, -1);

    // Use an orange-ish color on 256-color terminals, fall back to yellow otherwise.
    if (COLORS >= 256)
        init_pair(PAIR_LOGO_LEET, 208, -1);
    else
        init_pair(PAIR_LOGO_LEET, COLOR_YELLOW, -1);

    init_pair(PAIR_LOGO_CMD, COLOR_CYAN, -1);

    init_pair(PAIR_SUBMIT_OK, COLOR_GREEN, -1);
    init_pair(PAIR_SUBMIT_FAIL, COLOR_RED, -1);

    init_pair(PAIR_CONFIG_BORDER, COLOR_CYAN, -1);
    init_pair(PAIR_CONFIG_LABEL, COLOR_YELLOW, -1);
}

int difficultyColorPair(const std::string &difficulty)
{
    if (difficulty == "Easy")
        return PAIR_EASY;
    if (difficulty == "Medium")
        return PAIR_MEDIUM;
    if (difficulty == "Hard")
        return PAIR_HARD;
    return 0; // no color / default
}

std::vector<std::string> wrapText(const std::string &text, int width)
{
    std::vector<std::string> result;
    if (width <= 0)
        width = 1;

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

    static const char *lines[LOGO_HEIGHT] = {
        "dP                            dP                             dP ",
        "88                            88                             88 ",
        "88        .d8888b. .d8888b. d8888P .d8888b. 88d8b.d8b. .d888b88 ",
        "88        88ooood8 88ooood8   88   88'  `\"\" 88'`88'`88 88'  `88 ",
        "88        88.  ... 88.  ...   88   88.  ... 88  88  88 88.  .88 ",
        "88888888P `88888P' `88888P'   dP   `88888P' dP  dP  dP `88888P8 "};

    for (int i = 0; i < LOGO_HEIGHT; i++)
    {
        std::string line(lines[i]);
        int splitCol = std::min((int)line.size(), LOGO_SPLIT_COL);
        std::string leftPart = line.substr(0, splitCol); // "leet" part
        std::string rightPart = line.substr(splitCol);   // "cmd" part

        attron(COLOR_PAIR(PAIR_LOGO_LEET) | A_BOLD);
        mvprintw(i, start_x, "%s", leftPart.c_str());
        attroff(COLOR_PAIR(PAIR_LOGO_LEET) | A_BOLD);

        attron(COLOR_PAIR(PAIR_LOGO_CMD));
        printw("%s", rightPart.c_str());
        attroff(COLOR_PAIR(PAIR_LOGO_CMD));
    }
}

// Reads a single line of input into a bordered field at the given window
// coordinates, using ncurses' built-in line editing (getnstr). The cursor
// is made visible for the duration of the call and restored afterward.
static std::string promptField(WINDOW *win, int y, int x)
{
    char buf[2048];

    curs_set(1);
    echo();

    wmove(win, y, x);
    wrefresh(win);

    wgetnstr(win, buf, sizeof(buf) - 1);

    noecho();
    curs_set(0);

    return std::string(buf);
}

// Shown once on startup when no config file exists yet. Collects the
// LeetCode session cookie and CSRF token needed to authenticate requests,
// then persists them via createConfig() so the user isn't asked again.
void configScreen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int boxWidth = std::min(cols - 4, 70);
    int boxHeight = 12;
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *win = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(win, TRUE);

    wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
    box(win, 0, 0);

    std::string title = "leetcmd — First-Time Setup";
    mvwprintw(win, 0, std::max(1, (boxWidth - (int)title.size()) / 2 - 1), " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);

    std::vector<std::string> intro = wrapText(
        "No LeetCode credentials were found. Please provide your session "
        "cookie and CSRF token below (available from your browser's dev tools "
        "while logged in to leetcode.com). These are stored locally and used "
        "only to authenticate API requests.",
        boxWidth - 4);

    int line = 2;
    for (auto &l : intro)
        mvwprintw(win, line++, 2, "%s", l.c_str());

    line += 1;

    wattron(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    mvwprintw(win, line, 2, "LEETCODE_SESSION:");
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    int sessionRow = line;
    line += 2;

    wattron(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    mvwprintw(win, line, 2, "csrftoken:");
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    int csrfRow = line;

    wrefresh(win);

    std::string session = promptField(win, sessionRow, 20);

    wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
    box(win, 0, 0);
    mvwprintw(win, 0, std::max(1, (boxWidth - (int)title.size()) / 2 - 1), " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
    wrefresh(win);

    std::string csrf = promptField(win, csrfRow, 12);

    createConfig(session, csrf);

    wattron(win, COLOR_PAIR(PAIR_SUBMIT_OK) | A_BOLD);
    mvwprintw(win, boxHeight - 2, 2, "Saved. Press any key to continue...");
    wattroff(win, COLOR_PAIR(PAIR_SUBMIT_OK) | A_BOLD);
    wrefresh(win);

    timeout(-1);
    wgetch(win);

    delwin(win);
    clear();
    refresh();
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
            int colorPair = difficultyColorPair(questions[i].difficulty);
            int attrs = colorPair ? COLOR_PAIR(colorPair) : A_NORMAL;
            if (i == highlight)
                attrs |= A_REVERSE;

            attron(attrs);
            mvprintw((i - offset) + LOGO_HEIGHT + 1, list_start_x, "%-70s %-15s %-10s",
                     questions[i].title.c_str(), questions[i].difficulty.c_str(), questions[i].status.c_str());
            attroff(attrs);
        }

        mvprintw(LINES - 1, 1, "Up/Down: Navigate | Enter: Select | Q: Quit");
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
            selectedQuestionId = questions[highlight].id;
            return SCREEN_QUESTION;
        }
        else if (c == 'q' || c == 'Q')
        {
            return SCREEN_EXIT;
        }
    }
}

// Renders the editable code buffer into codePad, auto-scrolling so the
// cursor always stays visible, and draws a reverse-video block for the cursor.
static void renderEditor(WINDOW *pad, Editor &ed, int visibleHeight)
{
    werase(pad);

    // Keep the cursor inside the visible window by adjusting scrollOffset.
    if (ed.cursor.row < ed.scrollOffset)
        ed.scrollOffset = ed.cursor.row;
    else if (ed.cursor.row >= ed.scrollOffset + visibleHeight)
        ed.scrollOffset = ed.cursor.row - visibleHeight + 1;
    if (ed.scrollOffset < 0)
        ed.scrollOffset = 0;

    for (int i = 0; i < (int)ed.lines.size(); i++)
        mvwprintw(pad, i, 0, "%s", ed.lines[i].c_str());

    // Draw the cursor as a reverse-video block over the character beneath it
    // (or a blank space if the cursor sits past the end of the line).
    chtype under = ' ';
    if (ed.cursor.col < (int)ed.lines[ed.cursor.row].size())
        under = (unsigned char)ed.lines[ed.cursor.row][ed.cursor.col];
    mvwaddch(pad, ed.cursor.row, ed.cursor.col, under | A_REVERSE);
}

Screen questionScreen()
{
    clear();
    refresh();

    int rows, cols, half, winHeight;
    WINDOW *questionInformation = nullptr, *questionCode = nullptr;
    WINDOW *infoPad = nullptr, *codePad = nullptr;

    std::vector<std::string> infoLines;
    int infoPadHeight = 0;
    int infoScroll = 0;
    int visibleHeight = 0;
    int codeWidth = 0;

    enum FocusedWindow
    {
        FOCUS_INFO,
        FOCUS_CODE
    };
    FocusedWindow focus = FOCUS_INFO;

    questionDetail detail = getQuestionDetail(selectedQuestionSlug);
    const question &q = detail.questionn;

    // Keep question_id in sync in case it wasn't set from the list screen.
    if (selectedQuestionId.empty())
        selectedQuestionId = q.questionId;

    std::string cleanContent = stripHtml(q.content);

    std::string infoText = q.title + "\n\n" + "Difficulty: " + q.difficulty + "\n\n" + cleanContent;

    std::string codeText;
    for (const auto &snippet : q.codeSnippets)
    {
        if (snippet.langSlug == "cpp")
        {
            codeText = snippet.code;
            selectedLangSlug = "cpp";
            break;
        }
    }
    if (codeText.empty() && !q.codeSnippets.empty())
    {
        codeText = q.codeSnippets[0].code;
        selectedLangSlug = q.codeSnippets[0].langSlug;
    }

    Editor ed;
    ed.loadText(codeText);

    auto setupWindows = [&]()
    {
        getmaxyx(stdscr, rows, cols);
        half = cols / 2;
        if (half < 10)
            half = 10;

        winHeight = rows - 1; // bottom line reserved for the shortcut bar

        if (questionInformation)
            delwin(questionInformation);
        if (questionCode)
            delwin(questionCode);
        if (infoPad)
            delwin(infoPad);
        if (codePad)
            delwin(codePad);

        questionInformation = newwin(winHeight, half, 0, 0);
        questionCode = newwin(winHeight, cols - half, 0, half);

        int infoWidth = std::max(half - 4, 1);
        codeWidth = std::max(cols - half - 4, 1);

        infoLines = wrapText(infoText, infoWidth);
        infoPadHeight = std::max((int)infoLines.size(), winHeight);

        infoPad = newpad(infoPadHeight, infoWidth);
        // Code pad height must fit the full editable buffer, not just the
        // wrapped display text, so it can grow as the user types new lines.
        codePad = newpad(std::max((int)ed.lines.size() + 50, winHeight), std::max(codeWidth, 200));

        for (int i = 0; i < (int)infoLines.size(); i++)
            mvwprintw(infoPad, i, 0, "%s", infoLines[i].c_str());

        infoScroll = 0;
        visibleHeight = winHeight - 2;
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

        renderEditor(codePad, ed, visibleHeight);

        prefresh(infoPad, infoScroll, 0, 1, 1, winHeight - 2, half - 2);
        prefresh(codePad, ed.scrollOffset, 0, 1, half + 1, winHeight - 2, cols - 2);

        attron(A_DIM);
        mvprintw(rows - 1, 1, "TAB: Switch Panel | Arrows: Move/Scroll | Enter/Backspace: Edit | S: Submit | Q: Back");
        attroff(A_DIM);
        refresh();
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
        else if (focus == FOCUS_INFO)
        {
            // Description panel is read-only: arrows just scroll it.
            if (c == KEY_UP)
            {
                if (infoScroll > 0)
                    infoScroll--;
            }
            else if (c == KEY_DOWN)
            {
                int maxScroll = std::max(0, infoPadHeight - visibleHeight);
                if (infoScroll < maxScroll)
                    infoScroll++;
            }
            else if (c == 's' || c == 'S')
            {
                lastSubmittedCode = ed.toString();

                delwin(infoPad);
                delwin(codePad);
                delwin(questionInformation);
                delwin(questionCode);

                return SCREEN_SUBMIT;
            }
            else if (c == 'q' || c == 'Q')
            {
                break;
            }
        }
        else // FOCUS_CODE: real text editing
        {
            if (c == KEY_UP)
                ed.moveUp();
            else if (c == KEY_DOWN)
                ed.moveDown();
            else if (c == KEY_LEFT)
                ed.moveLeft();
            else if (c == KEY_RIGHT)
                ed.moveRight();
            else if (c == '\n' || c == KEY_ENTER)
                ed.newline();
            else if (c == KEY_BACKSPACE || c == 127 || c == 8)
                ed.backspace();
            else if (c == 19) // Ctrl+S as a submit shortcut while editing
            {
                lastSubmittedCode = ed.toString();

                delwin(infoPad);
                delwin(codePad);
                delwin(questionInformation);
                delwin(questionCode);

                return SCREEN_SUBMIT;
            }
            else if (c == 17) // Ctrl+Q to leave the editor without submitting
            {
                break;
            }
            else if (isprint(c))
                ed.insertChar((char)c);
        }

        refreshAll();
    }

    delwin(infoPad);
    delwin(codePad);
    delwin(questionInformation);
    delwin(questionCode);

    return SCREEN_MAIN;
}

Screen submitScreen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Simple "submitting" animation while the real request is in flight.
    const char *frames[] = {".", "..", "...", "...."};
    for (int i = 0; i < 4; i++)
    {
        clear();
        mvprintw(rows / 2 - 1, (cols - 20) / 2, "Submitting%s", frames[i % 4]);
        refresh();
        napms(200);
    }

    // --- Real submit + poll flow using leetcode_client.h ---
    bool accepted = false;
    std::string verdict = "Error";
    std::string runtimeInfo;
    std::string memoryInfo;
    std::string errorInfo;

    try
    {
        submitResponse sr = submitCode(selectedQuestionSlug, lastSubmittedCode,
                                       selectedLangSlug, selectedQuestionId);
        std::ofstream idDbg("submission_id_debug.txt");
        idDbg << sr.submissionId;
        idDbg.close();

        // LeetCode's judge is async: poll getSubmitDetail until a verdict
        // is available, showing the animation while we wait.
        submissionDetail sd;
        const int maxAttempts = 15;
        for (int attempt = 0; attempt < maxAttempts; attempt++)
        {
            clear();
            mvprintw(rows / 2 - 1, (cols - 20) / 2, "Judging%s", frames[attempt % 4]);
            refresh();
            napms(1500);

            sd = getSubmitDetail(sr.submissionId, selectedQuestionSlug);

            // statusCode == 10 means "Accepted" in LeetCode's API; any other
            // non-zero code means judging finished with some other verdict.
            if (sd.statusCode != 0)
                break;
        }

        accepted = (sd.statusCode == 10);

        if (accepted)
        {
            verdict = "Accepted";
        }
        else if (!sd.compileError.empty())
        {
            verdict = "Compile Error";
            errorInfo = sd.compileError;
        }
        else if (!sd.runtimeError.empty())
        {
            verdict = "Runtime Error";
            errorInfo = sd.runtimeError;
        }
        else
        {
            verdict = "Wrong Answer (" + std::to_string(sd.totalCorrect) + "/" + std::to_string(sd.totalTestcases) + ")";
        }

        if (!sd.runtimeDisplay.empty())
        {
            runtimeInfo = "Runtime: " + sd.runtimeDisplay;
            if (sd.runtimePercentile >= 0.0)
                runtimeInfo += "  (faster than " + std::to_string((int)sd.runtimePercentile) + "%)";
        }
        if (!sd.memoryDisplay.empty())
        {
            memoryInfo = "Memory:  " + sd.memoryDisplay;
            if (sd.memoryPercentile >= 0.0)
                memoryInfo += "  (less than " + std::to_string((int)sd.memoryPercentile) + "%)";
        }
    }
    catch (const std::exception &e)
    {
        endwin();
        std::cout << "Exception: " << e.what() << std::endl;
        std::cin.get();
        exit(0);
    }

    clear();

    int boxWidth = std::min(cols - 4, 64);
    int boxHeight = errorInfo.empty() ? 8 : 12;
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *resultWin = newwin(boxHeight, boxWidth, boxY, boxX);
    int colorPair = accepted ? PAIR_SUBMIT_OK : PAIR_SUBMIT_FAIL;

    wattron(resultWin, COLOR_PAIR(colorPair) | A_BOLD);
    box(resultWin, 0, 0);
    mvwprintw(resultWin, 1, std::max(1, (boxWidth - (int)verdict.size()) / 2), "%s", verdict.c_str());
    wattroff(resultWin, COLOR_PAIR(colorPair) | A_BOLD);

    int line = 3;
    if (!runtimeInfo.empty())
        mvwprintw(resultWin, line++, 2, "%s", runtimeInfo.c_str());
    if (!memoryInfo.empty())
        mvwprintw(resultWin, line++, 2, "%s", memoryInfo.c_str());

    if (!errorInfo.empty())
    {
        // Wrap the error text so it doesn't overflow the box width.
        std::vector<std::string> wrapped = wrapText(errorInfo, boxWidth - 4);
        for (size_t i = 0; i < wrapped.size() && line < boxHeight - 2; i++, line++)
            mvwprintw(resultWin, line, 2, "%s", wrapped[i].c_str());
    }

    mvwprintw(resultWin, boxHeight - 2, 2, "Press any key to return...");

    wrefresh(resultWin);
    flushinp();
    timeout(-1);
    getch(); // any key returns to the question screen

    delwin(resultWin);
    return SCREEN_QUESTION;
}

int run_menu()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    curs_set(0);

    initColors();

    if (!checkConfig())
        configScreen();

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

        case SCREEN_SUBMIT:
            current = submitScreen();
            break;

        default:
            current = SCREEN_EXIT;
            break;
        }
    }
    endwin();
    return 0;
}