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

    // Config screen now themed with the same leet(orange)/cmd(cyan) palette
    // used by the logo, instead of its own separate colors.
    if (COLORS >= 256)
        init_pair(PAIR_CONFIG_BORDER, 208, -1);
    else
        init_pair(PAIR_CONFIG_BORDER, COLOR_YELLOW, -1);
    init_pair(PAIR_CONFIG_LABEL, COLOR_CYAN, -1);
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
// coordinates. Unlike the old getnstr()-based version, this scrolls the
// visible text horizontally so tokens longer than the field width (LeetCode
// session cookies routinely run 100+ chars) never overflow past the box
// border. Left/Right arrows move the cursor, Backspace deletes, Enter
// confirms. The cursor is made visible for the duration of the call and
// restored afterward.
static std::string promptFieldScrolling(WINDOW *win, int y, int x, int fieldWidth)
{
    if (fieldWidth < 1)
        fieldWidth = 1;

    std::string input;
    int pos = 0;       // cursor position within input
    int scrollOff = 0; // index of first visible character

    curs_set(1);
    keypad(win, TRUE);

    auto redraw = [&]()
    {
        if (pos < scrollOff)
            scrollOff = pos;
        else if (pos - scrollOff >= fieldWidth)
            scrollOff = pos - fieldWidth + 1;
        if (scrollOff < 0)
            scrollOff = 0;

        // Clear the field area first so shorter re-renders don't leave
        // stray characters from a previous, longer render.
        mvwprintw(win, y, x, "%*s", fieldWidth, "");

        int visibleLen = std::min((int)input.size() - scrollOff, fieldWidth);
        if (visibleLen < 0)
            visibleLen = 0;
        std::string visible = input.substr(scrollOff, visibleLen);

        mvwprintw(win, y, x, "%s", visible.c_str());
        wmove(win, y, x + (pos - scrollOff));
        wrefresh(win);
    };

    redraw();

    int ch;
    while ((ch = wgetch(win)) != '\n' && ch != KEY_ENTER)
    {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (pos > 0)
            {
                input.erase(pos - 1, 1);
                pos--;
            }
        }
        else if (ch == KEY_DC) // Delete key
        {
            if (pos < (int)input.size())
                input.erase(pos, 1);
        }
        else if (ch == KEY_LEFT)
        {
            if (pos > 0)
                pos--;
        }
        else if (ch == KEY_RIGHT)
        {
            if (pos < (int)input.size())
                pos++;
        }
        else if (ch == KEY_HOME)
        {
            pos = 0;
        }
        else if (ch == KEY_END)
        {
            pos = (int)input.size();
        }
        else if (isprint(ch))
        {
            input.insert(input.begin() + pos, (char)ch);
            pos++;
        }

        redraw();
    }

    noecho();
    curs_set(0);
    return input;
}

// Shown once on startup when no config file exists yet. Collects the
// LeetCode session cookie and CSRF token needed to authenticate requests,
// then persists them via createConfig() so the user isn't asked again.
// Styled with the same leet(orange)/cmd(cyan) palette as the main logo,
// and uses promptFieldScrolling so long tokens never wrap past the border.
void configScreen()
{
    clear();
    refresh();
    noecho();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int boxWidth = std::min(cols - 4, 76);
    int boxHeight = 13;
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *win = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(win, TRUE);

    auto drawChrome = [&]()
    {
        wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);

        std::string leet = "leet";
        std::string cmd = "cmd";
        std::string suffix = " — First-Time Setup ";
        int totalLen = (int)(leet.size() + cmd.size() + suffix.size());
        int titleX = std::max(1, (boxWidth - totalLen) / 2);

        wattron(win, COLOR_PAIR(PAIR_LOGO_LEET) | A_BOLD);
        mvwprintw(win, 0, titleX, "%s", leet.c_str());
        wattroff(win, COLOR_PAIR(PAIR_LOGO_LEET) | A_BOLD);

        wattron(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
        wprintw(win, "%s", cmd.c_str());
        wattroff(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);

        wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
        wprintw(win, "%s", suffix.c_str());
        wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
    };

    drawChrome();

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

    int labelWidth = 18; // "LEETCODE_SESSION:" is the longer of the two labels
    int fieldX = 2 + labelWidth + 1;
    int fieldWidth = std::max(boxWidth - fieldX - 2, 10);

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

    std::string session = promptFieldScrolling(win, sessionRow, fieldX, fieldWidth);

    drawChrome();
    for (auto &l : intro)
        ; // intro text is static; no need to redraw between fields other than chrome
    // Redraw both labels since drawChrome() only repaints the border/title.
    wattron(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    mvwprintw(win, sessionRow, 2, "LEETCODE_SESSION:");
    mvwprintw(win, csrfRow, 2, "csrftoken:");
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    // Re-print the already-entered session value (truncated/scrolled view)
    // so it stays visible while the user fills in the csrf field.
    {
        std::string visible = session.size() > (size_t)fieldWidth
                                  ? session.substr(session.size() - fieldWidth)
                                  : session;
        mvwprintw(win, sessionRow, fieldX, "%s", visible.c_str());
    }
    wrefresh(win);

    std::string csrf = promptFieldScrolling(win, csrfRow, fieldX, fieldWidth);

    createConfig(session, csrf);

    wattron(win, COLOR_PAIR(PAIR_SUBMIT_OK) | A_BOLD);
    mvwprintw(win, boxHeight - 2, 2, "Saved. Press any key to continue...");
    wattroff(win, COLOR_PAIR(PAIR_SUBMIT_OK) | A_BOLD);
    wrefresh(win);

    flushinp();
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

// Shows a small popup listing every language LeetCode provided a starter
// snippet for on this question (mirrors the language dropdown on
// leetcode.com), lets the user browse it with Up/Down, and returns the
// index of the chosen entry in `snippets` (or -1 if the user cancelled
// with Q/Esc). `currentIndex` is marked with a "> " so it's obvious which
// language is currently loaded in the editor.
static int selectLanguage(const std::vector<codeSnippet> &snippets, int currentIndex)
{
    if (snippets.empty())
        return -1;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int n = (int)snippets.size();

    int boxWidth = 30;
    for (auto &s : snippets)
        boxWidth = std::max(boxWidth, (int)s.lang.size() + 8);
    boxWidth = std::min(boxWidth, std::max(cols - 4, 20));

    int listHeight = std::min(n, std::max(rows - 8, 3));
    int boxHeight = std::min(listHeight + 4, rows - 2);
    listHeight = boxHeight - 4;

    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *win = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(win, TRUE);

    int highlight = (currentIndex >= 0 && currentIndex < n) ? currentIndex : 0;
    int offset = 0;

    auto draw = [&]()
    {
        werase(win);
        wattron(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
        box(win, 0, 0);
        std::string title = " Select Language ";
        mvwprintw(win, 0, std::max(1, (boxWidth - (int)title.size()) / 2), "%s", title.c_str());
        wattroff(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);

        if (highlight < offset)
            offset = highlight;
        else if (highlight >= offset + listHeight)
            offset = highlight - listHeight + 1;

        for (int i = offset; i < offset + listHeight && i < n; i++)
        {
            int y = 1 + (i - offset);
            bool isCurrent = (i == currentIndex);
            std::string label = (isCurrent ? "> " : "  ") + snippets[i].lang;
            if ((int)label.size() > boxWidth - 3)
                label = label.substr(0, boxWidth - 3);

            int attrs = (i == highlight) ? A_REVERSE : A_NORMAL;
            if (isCurrent)
                attrs |= A_BOLD;

            wattron(win, attrs);
            mvwprintw(win, y, 2, "%-*s", boxWidth - 3, label.c_str());
            wattroff(win, attrs);
        }

        wattron(win, A_DIM);
        mvwprintw(win, boxHeight - 2, 2, "%-*.*s", boxWidth - 4, boxWidth - 4,
                  "Up/Down  Enter: Select  Q: Cancel");
        wattroff(win, A_DIM);

        wrefresh(win);
    };

    draw();
    flushinp();
    timeout(-1);

    int chosen = -1;
    while (true)
    {
        int c = wgetch(win);
        if (c == KEY_UP)
            highlight = (highlight - 1 + n) % n;
        else if (c == KEY_DOWN)
            highlight = (highlight + 1) % n;
        else if (c == '\n' || c == KEY_ENTER)
        {
            chosen = highlight;
            break;
        }
        else if (c == 'q' || c == 'Q' || c == 27) // Esc
        {
            chosen = -1;
            break;
        }
        draw();
    }

    delwin(win);
    return chosen;
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

    // Tracks which entry of q.codeSnippets is currently loaded in the editor,
    // so the language picker (L) can highlight/mark it.
    int langIndex = 0;
    for (size_t i = 0; i < q.codeSnippets.size(); i++)
    {
        if (q.codeSnippets[i].langSlug == selectedLangSlug)
        {
            langIndex = (int)i;
            break;
        }
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
        mvprintw(rows - 1, 1, "TAB: Switch Panel | Arrows: Move/Scroll | Enter/Backspace: Edit | S: Submit | L: Language | Q: Back");
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
            else if (c == 'l' || c == 'L')
            {
                int choice = selectLanguage(q.codeSnippets, langIndex);
                if (choice != -1 && choice != langIndex)
                {
                    langIndex = choice;
                    const codeSnippet &snip = q.codeSnippets[langIndex];
                    selectedLangSlug = snip.langSlug;
                    ed.loadText(snip.code);
                    setupWindows();
                }
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
            else if (c == 12) // Ctrl+L to switch language while editing
            {
                int choice = selectLanguage(q.codeSnippets, langIndex);
                if (choice != -1 && choice != langIndex)
                {
                    langIndex = choice;
                    const codeSnippet &snip = q.codeSnippets[langIndex];
                    selectedLangSlug = snip.langSlug;
                    ed.loadText(snip.code);
                    setupWindows();
                }
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

// Maps a LeetCode submission statusCode to a short, human-readable verdict.
// 10 Accepted, 11 Wrong Answer, 12 Memory Limit Exceeded, 13 Output Limit
// Exceeded, 14 Time Limit Exceeded, 15 Runtime Error, 16 Internal Error,
// 20 Compile Error - anything else falls back to "Unknown Error".
static std::string verdictTitle(int statusCode)
{
    switch (statusCode)
    {
    case 10:
        return "Accepted";
    case 11:
        return "Wrong Answer";
    case 12:
        return "Memory Limit Exceeded";
    case 13:
        return "Output Limit Exceeded";
    case 14:
        return "Time Limit Exceeded";
    case 15:
        return "Runtime Error";
    case 16:
        return "Internal Error";
    case 20:
        return "Compile Error";
    default:
        return "Unknown Error";
    }
}

// Green for success, yellow for "resource limit" style failures (still ran,
// just too slow/too much memory), red for everything else that failed outright.
static int verdictColorPair(int statusCode)
{
    if (statusCode == 10)
        return PAIR_SUBMIT_OK;
    if (statusCode == 12 || statusCode == 13 || statusCode == 14)
        return PAIR_MEDIUM;
    return PAIR_SUBMIT_FAIL;
}

// Small ASCII progress bar used to visualize runtime/memory percentiles,
// e.g. "[#############-----] 72%".
static std::string percentileBar(double percentile, int width)
{
    if (width < 8)
        width = 8;
    int inner = width - 2;
    int filled = (int)((percentile / 100.0) * inner + 0.5);
    filled = std::max(0, std::min(inner, filled));

    std::string bar = "[";
    bar += std::string(filled, '#');
    bar += std::string(inner - filled, '-');
    bar += "] " + std::to_string((int)percentile) + "%";
    return bar;
}

// Draws a small bordered status box with a spinner + message in the center
// of the screen. Used both while the solution is uploading and while the
// judge is running the test cases, so the user always sees clear feedback
// instead of a frozen terminal.
static void drawStatusBox(int rows, int cols, const std::string &message, int frame)
{
    static const char spinnerFrames[] = {'|', '/', '-', '\\'};

    int boxWidth = std::min(cols - 4, 50);
    int boxHeight = 5;
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    clear();

    WINDOW *win = newwin(boxHeight, boxWidth, boxY, boxX);
    wattron(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
    box(win, 0, 0);
    mvwprintw(win, 0, std::max(1, (boxWidth - 9) / 2), " leetcmd ");
    wattroff(win, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);

    std::string line;
    line += spinnerFrames[frame % 4];
    line += "  " + message;
    mvwprintw(win, boxHeight / 2, std::max(1, (boxWidth - (int)line.size()) / 2), "%s", line.c_str());

    wrefresh(win);
    delwin(win);
}

Screen submitScreen()
{
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int frame = 0;

    // --- Phase 1: uploading the solution ---
    for (int i = 0; i < 5; i++)
    {
        drawStatusBox(rows, cols, "Uploading your solution...", frame++);
        napms(120);
    }

    // --- Real submit + poll flow using leetcode_client.h ---
    bool accepted = false;
    int statusCode = -1;
    std::string verdict = "Unknown Error";
    std::string runtimeInfo, memoryInfo;
    double runtimePercentile = -1.0, memoryPercentile = -1.0;
    std::vector<std::string> detailLines; // pre-wrapped extra info (errors, WA input/output/expected)

    try
    {
        submitResponse sr = submitCode(selectedQuestionSlug, lastSubmittedCode,
                                       selectedLangSlug, selectedQuestionId);

        // Any key the user mashes while waiting for the network/judge would
        // otherwise sit in the input queue and get consumed the instant the
        // result screen calls getch(), making it look like the box flashes
        // and disappears immediately. Flush before the wait starts.
        flushinp();

        // LeetCode's judge is asynchronous: poll getSubmitDetail until a
        // verdict is available (statusCode != 0 means judging has finished),
        // showing an animated spinner while we wait instead of freezing.
        submissionDetail sd;
        const int maxAttempts = 20; // Increase this if you experience timeouts
        for (int attempt = 0; attempt < maxAttempts; attempt++)
        {
            std::string msg = "Running against test cases";
            for (int d = 0; d <= attempt % 3; d++)
                msg += ".";
            drawStatusBox(rows, cols, msg, frame++);

            sd = getSubmitDetail(sr.submissionId, selectedQuestionSlug);
            if (sd.statusCode != 0)
                break;

            napms(1000); // Wait 1 second before polling again
            flushinp();
        }

        statusCode = sd.statusCode;
        accepted = (statusCode == 10);
        verdict = verdictTitle(statusCode);

        int wrapWidth = std::min(cols - 8, 70);

        if (statusCode == 11) // Wrong Answer: show the same Input/Output/Expected
        {                     // breakdown LeetCode.com itself shows.
            verdict += "  (" + std::to_string(sd.totalCorrect) + "/" +
                       std::to_string(sd.totalTestcases) + " testcases passed)";

            auto addSection = [&](const std::string &label, const std::string &content)
            {
                if (content.empty())
                    return;
                if (!detailLines.empty())
                    detailLines.push_back("");
                detailLines.push_back(label);
                for (auto &l : wrapText(content, wrapWidth))
                    detailLines.push_back("  " + l);
            };
            addSection("Input:", sd.lastTestcase);
            addSection("Output:", sd.codeOutput);
            addSection("Expected:", sd.expectedOutput);
        }
        else if (!sd.compileError.empty())
        {
            detailLines.push_back("Compiler Output:");
            for (auto &l : wrapText(sd.compileError, wrapWidth))
                detailLines.push_back("  " + l);
        }
        else if (!sd.runtimeError.empty())
        {
            detailLines.push_back("Runtime Output:");
            for (auto &l : wrapText(sd.runtimeError, wrapWidth))
                detailLines.push_back("  " + l);
            if (!sd.lastTestcase.empty())
            {
                detailLines.push_back("");
                detailLines.push_back("Last Input:");
                for (auto &l : wrapText(sd.lastTestcase, wrapWidth))
                    detailLines.push_back("  " + l);
            }
        }

        if (accepted)
        {
            if (!sd.runtimeDisplay.empty())
            {
                runtimeInfo = "Runtime: " + sd.runtimeDisplay;
                runtimePercentile = sd.runtimePercentile;
            }
            if (!sd.memoryDisplay.empty())
            {
                memoryInfo = "Memory:  " + sd.memoryDisplay;
                memoryPercentile = sd.memoryPercentile;
            }
        }
    }
    catch (const std::exception &e)
    {
        endwin();
        std::cout << "Exception: " << e.what() << std::endl;
        std::cin.get();
        exit(0);
    }

    // --- Result screen ---
    clear();
    refresh();
    getmaxyx(stdscr, rows, cols);

    int colorPair = verdictColorPair(statusCode);

    int boxWidth = std::min(cols - 4, 78);
    int boxHeight = std::max(10, std::min(rows - 2, 22));
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *resultWin = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(resultWin, TRUE);

    // Every string printed into resultWin goes through this helper so a long
    // verdict/runtime/percentile line can never overflow the window's right
    // border. Unclipped mvwprintw calls previously wrapped onto the next
    // physical row inside the window and visually collided with whatever
    // was printed there next (e.g. the memory line eating into the runtime
    // percentile line).
    auto printClipped = [&](int y, int x, const std::string &text)
    {
        int maxLen = std::max(0, boxWidth - x - 1);
        std::string clipped = (int)text.size() > maxLen ? text.substr(0, maxLen) : text;
        mvwprintw(resultWin, y, x, "%s", clipped.c_str());
    };

    wattron(resultWin, COLOR_PAIR(colorPair) | A_BOLD);
    box(resultWin, 0, 0);
    printClipped(1, std::max(1, (boxWidth - (int)verdict.size()) / 2), verdict);
    wattroff(resultWin, COLOR_PAIR(colorPair) | A_BOLD);

    int line = 3;
    if (accepted)
    {
        if (!runtimeInfo.empty())
        {
            printClipped(line++, 2, runtimeInfo);
            if (runtimePercentile >= 0.0)
            {
                std::string suffix = " faster";
                int barWidth = std::max(8, boxWidth - 4 - 2 - (int)suffix.size());
                wattron(resultWin, A_DIM);
                printClipped(line++, 2, "  " + percentileBar(runtimePercentile, barWidth) + suffix);
                wattroff(resultWin, A_DIM);
            }
        }
        if (!memoryInfo.empty())
        {
            printClipped(line++, 2, memoryInfo);
            if (memoryPercentile >= 0.0)
            {
                std::string suffix = " less mem";
                int barWidth = std::max(8, boxWidth - 4 - 2 - (int)suffix.size());
                wattron(resultWin, A_DIM);
                printClipped(line++, 2, "  " + percentileBar(memoryPercentile, barWidth) + suffix);
                wattroff(resultWin, A_DIM);
            }
        }
        line++;
    }

    // Scrollable detail area (compile/runtime output, or WA input/output/expected)
    // so long payloads never get clipped -- they're just scrolled with arrows.
    int detailAreaTop = line;
    int detailAreaHeight = std::max(1, boxHeight - detailAreaTop - 2);
    int detailPadHeight = std::max((int)detailLines.size(), 1);

    WINDOW *detailPad = newpad(detailPadHeight, std::max(boxWidth - 4, 1));
    for (int i = 0; i < (int)detailLines.size(); i++)
        mvwprintw(detailPad, i, 0, "%s", detailLines[i].c_str());

    int detailScroll = 0;
    bool scrollable = (int)detailLines.size() > detailAreaHeight;
    std::string footerText = scrollable
                                  ? "Up/Down: Scroll  |  Any other key: Return"
                                  : "Press any key to return...";

    auto renderResult = [&]()
    {
        mvwprintw(resultWin, boxHeight - 2, 2, "%*s", boxWidth - 4, "");
        wattron(resultWin, A_DIM);
        mvwprintw(resultWin, boxHeight - 2, 2, "%s", footerText.c_str());
        wattroff(resultWin, A_DIM);
        wrefresh(resultWin);

        pnoutrefresh(detailPad, detailScroll, 0,
                     boxY + detailAreaTop, boxX + 2,
                     boxY + detailAreaTop + detailAreaHeight - 1, boxX + boxWidth - 3);
        doupdate();
    };

    renderResult();

    // Guarantee the result box actually stays on screen until the user
    // deliberately presses something. flushinp() drops anything that was
    // queued up during the submit/judging wait, and the loop below ignores
    // ERR (no key ready) and KEY_RESIZE so a stray terminal resize event
    // can't be mistaken for "return to question" input.
    flushinp();
    timeout(-1);

    while (true)
    {
        int key = wgetch(resultWin);
        if (key == ERR || key == KEY_RESIZE)
            continue;

        if (key == KEY_UP && detailScroll > 0)
        {
            detailScroll--;
            renderResult();
            continue;
        }
        if (key == KEY_DOWN)
        {
            int maxScroll = std::max(0, detailPadHeight - detailAreaHeight);
            if (detailScroll < maxScroll)
            {
                detailScroll++;
                renderResult();
                continue;
            }
        }
        break;
    }

    delwin(detailPad);
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