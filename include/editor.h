#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

// Cursor pozisyonu: hangi satır (row), satır içinde hangi kolon (col)
struct CursorPos {
    int row = 0;
    int col = 0;
};

// Basit satır tabanlı text editor state'i.
// Her satır ayrı bir std::string olarak tutulur (newline karakteri YOK).
struct Editor {
    std::vector<std::string> lines;
    CursorPos cursor;
    int scrollOffset = 0; // pad'de en üstte görünen satır index'i

    Editor() { lines.push_back(""); }

    // Dışarıdan gelen (örn. LeetCode code snippet) text'i satırlara böl
    void loadText(const std::string &text)
    {
        lines.clear();
        std::stringstream ss(text);
        std::string line;
        while (std::getline(ss, line, '\n'))
            lines.push_back(line);
        if (lines.empty())
            lines.push_back("");
        cursor = {0, 0};
        scrollOffset = 0;
    }

    // Editor içeriğini tek bir string olarak geri döndür (submit için)
    std::string toString() const
    {
        std::string result;
        for (size_t i = 0; i < lines.size(); i++)
        {
            result += lines[i];
            if (i + 1 < lines.size())
                result += "\n";
        }
        return result;
    }

    void insertChar(char c)
    {
        lines[cursor.row].insert(cursor.col, 1, c);
        cursor.col++;
    }

    void newline()
    {
        // Cursor'dan sonraki kısmı yeni bir satıra taşı (Enter davranışı)
        std::string rest = lines[cursor.row].substr(cursor.col);
        lines[cursor.row] = lines[cursor.row].substr(0, cursor.col);
        lines.insert(lines.begin() + cursor.row + 1, rest);
        cursor.row++;
        cursor.col = 0;
    }

    void backspace()
    {
        if (cursor.col > 0)
        {
            lines[cursor.row].erase(cursor.col - 1, 1);
            cursor.col--;
        }
        else if (cursor.row > 0)
        {
            // Satır başındaysak bir önceki satırla birleştir
            int prevLen = (int)lines[cursor.row - 1].size();
            lines[cursor.row - 1] += lines[cursor.row];
            lines.erase(lines.begin() + cursor.row);
            cursor.row--;
            cursor.col = prevLen;
        }
    }

    void moveLeft()
    {
        if (cursor.col > 0)
            cursor.col--;
        else if (cursor.row > 0)
        {
            cursor.row--;
            cursor.col = (int)lines[cursor.row].size();
        }
    }

    void moveRight()
    {
        if (cursor.col < (int)lines[cursor.row].size())
            cursor.col++;
        else if (cursor.row + 1 < (int)lines.size())
        {
            cursor.row++;
            cursor.col = 0;
        }
    }

    void moveUp()
    {
        if (cursor.row > 0)
        {
            cursor.row--;
            cursor.col = std::min(cursor.col, (int)lines[cursor.row].size());
        }
    }

    void moveDown()
    {
        if (cursor.row + 1 < (int)lines.size())
        {
            cursor.row++;
            cursor.col = std::min(cursor.col, (int)lines[cursor.row].size());
        }
    }
};