# leetcmd

A terminal-based LeetCode client written in C++ with [ncurses](https://invisible-island.net/ncurses/). Browse problems, read descriptions, write and edit code in a built-in editor, then run or submit your solution — all without leaving the terminal.

![leetcmd demo](leetcmd_sample.gif)

## Features

- **Problem list** — paginated browsing of the full LeetCode problem set, with difficulty and paid-only indicators, live search, difficulty sorting, and a toggle to hide already-solved problems.
- **Problem view** — HTML problem descriptions are parsed and rendered as readable plain text directly in the terminal.
- **Built-in code editor** — a lightweight, syntax-aware text editor with:
  - Soft-tab indentation and auto-indent after `{`
  - Auto-closing brackets/quotes with typeover support
  - Bracket-match highlighting
  - Word-wise cursor movement (Ctrl+Left/Right style), Home/End, Page Up/Down, and horizontal scrolling for long lines
  - Live, per-language autocomplete with substring matching and inline suggestions (C++, C, Java, Python, and more), including C++ function-signature hints
  - Whole-buffer reindent/format (Ctrl+F) and multi-step undo/redo (Ctrl+U / Ctrl+Y)
- **Run & Submit** — execute your code against sample test cases or submit it directly to LeetCode, with results (runtime, memory, percentile, pass/fail, compile errors) shown in a dedicated results screen.
- **Profile screen** — view your LeetCode stats at a glance: total and per-difficulty solved counts, submission beat-percentages, language breakdown, and skill tags (fundamental/intermediate/advanced).
- **Persistent login** — your LeetCode session cookie and CSRF token are entered once and stored locally.

## Requirements

- A C++17 (or later) compiler
- [ncurses](https://invisible-island.net/ncurses/)
- [libcurl](https://curl.se/libcurl/)
- [nlohmann/json](https://github.com/nlohmann/json)

## Building

```bash
g++ -std=c++17 main.cpp gui.cpp leetcode_client.cpp config_manager.cpp html_parser.cpp http_client.cpp \
    -lncurses -lcurl -o leetcmd
```

> Adjust include/library paths as needed for your system, or wire the sources into your own build system (CMake, Make, etc.).

## Usage

```bash
./leetcmd
```

On first run, leetcmd will ask for your **LEETCODE_SESSION** cookie and **csrftoken**, obtainable from your browser's dev tools while logged in to [leetcode.com](https://leetcode.com). These credentials are stored locally in your home directory (`~/.leetcmd_config` on Linux/macOS, `%APPDATA%\leetcmd\config.txt` on Windows) and are used only to authenticate API requests — they are never sent anywhere else.

### Controls

| Screen | Key | Action |
|---|---|---|
| Main | `↑ / ↓` | Navigate problem list |
| Main | `Enter` | Open selected problem |
| Main | `/` | Search problems |
| Main | `H` | Hide/show solved problems |
| Main | `D` | Cycle difficulty sort (off → easy→hard → hard→easy) |
| Main | `P` | Open profile |
| Main | `q` | Quit |
| Question | `Tab` | Autocomplete (if suggestions available) or indent |
| Question | `Shift+Tab` / `F2` | Switch panel (description ↔ editor) |
| Question | `Ctrl+F` | Reformat/reindent code |
| Question | `Ctrl+U` / `Ctrl+Y` | Undo / redo |
| Question | `r` | Run code against sample tests |
| Question | `s` | Submit code |
| Question | `l` | Change language |
| Question | `q` | Back to main menu |
| Run / Submit | `Esc` / `q` | Back to question |
| Profile | `Esc` / `q` | Back to list |

## Project Structure

```
main.cpp              Entry point
gui.cpp / gui.h        ncurses UI: screens, rendering, input handling
editor.h                Built-in text editor (buffer, cursor, editing operations)
leetcode_client.cpp/.h  LeetCode GraphQL/REST API client (problems, run, submit)
http_client.cpp/.h     Minimal libcurl-based HTTP POST wrapper
html_parser.cpp/.h     Converts LeetCode's HTML problem descriptions to plain text
config_manager.cpp/.h  Reads/writes local credential config file
credits.h               Credits
```

## Disclaimer

This is an unofficial, community-built client. It is not affiliated with or endorsed by LeetCode. Use it in accordance with LeetCode's terms of service.

## License

This project is licensed under the **[MIT License](LICENSE)**.

In short: you're free to use, copy, modify, merge, publish, distribute, and even sell copies of this software, as long as the original copyright notice and license text are included in all copies or substantial portions of it.