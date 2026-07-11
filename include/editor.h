#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

// Cursor pozisyonu: hangi satır (row), satır içinde hangi kolon (col)
struct CursorPos {
    int row = 0;
    int col = 0;
};

// Basit satır tabanlı text editor state'i.
// Her satır ayrı bir std::string olarak tutulur (newline karakteri YOK).
struct Editor {
    static const int TAB_WIDTH = 4;

    std::vector<std::string> lines;
    CursorPos cursor;
    int scrollOffset = 0;  // pad'de en üstte görünen satır index'i (dikey scroll)
    int hScrollOffset = 0; // pad'de en solda görünen kolon index'i (yatay scroll)

    Editor() { lines.push_back(""); }

    // Satırın kaç boşlukla başladığını döndürür (girinti miktarı).
    static int leadingSpaces(const std::string &line)
    {
        int n = 0;
        while (n < (int)line.size() && line[n] == ' ')
            n++;
        return n;
    }

    // Cursor'dan itibaren geriye doğru satırın tamamen boşluklardan mı
    // oluştuğunu kontrol eder (Tab/Backspace'in "girinti modu"na girip
    // girmeyeceğine karar vermek için kullanılır).
    bool onlySpacesBeforeCursor() const
    {
        for (int i = 0; i < cursor.col; i++)
            if (lines[cursor.row][i] != ' ')
                return false;
        return true;
    }

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

    // Tab tuşu: girinti boşluklarındaysak bir sonraki tab durağına kadar
    // boşluk ekler (VSCode/LeetCode editörlerindeki gibi "soft tab").
    void insertTab()
    {
        int spaces = TAB_WIDTH - (cursor.col % TAB_WIDTH);
        lines[cursor.row].insert(cursor.col, spaces, ' ');
        cursor.col += spaces;
    }

    // Açılış parantezi/tırnak yazıldığında kapanışını otomatik ekler ve
    // imleci ortada bırakır (ör. "(" -> "()" imleç ortada). Kapanış
    // karakteri, imlecin hemen sağındaki karakterle aynıysa yeni bir
    // karakter eklemek yerine sadece üzerinden atlar (typeover).
    // Eşleşme yoksa normal insertChar gibi davranır. Bir parantez/tırnak
    // eklenip eklenmediğini true/false olarak döndürür (çağıran taraf
    // otomatik girinti mantığını buna göre atlayabilir).
    void insertCharSmart(char c)
    {
        static const std::string openers = "([{\"'";
        static const std::string closers = ")]}\"'";

        std::string &line = lines[cursor.row];

        // Kapanış karakteri ve tam altında zaten aynısı varsa: üzerinden atla.
        size_t closeIdx = closers.find(c);
        if (closeIdx != std::string::npos && cursor.col < (int)line.size() &&
            line[cursor.col] == c)
        {
            cursor.col++;
            return;
        }

        insertChar(c);

        size_t openIdx = openers.find(c);
        if (openIdx != std::string::npos)
        {
            // Tırnaklarda: hemen önünde aynı tırnak zaten varsa (kapanışın
            // üstüne yazılıyor demektir), çift ekleme yapma.
            bool isQuote = (c == '"' || c == '\'');
            bool nextIsSame = cursor.col < (int)line.size() && line[cursor.col] == c;
            if (isQuote && nextIsSame)
                return;

            char close = closers[openIdx];
            lines[cursor.row].insert(cursor.col, 1, close);
            // Cursor açılışın hemen sonrasında kalır, kapanışın önünde.
        }
    }

    void newline()
    {
        // Cursor'dan sonraki kısmı yeni bir satıra taşı (Enter davranışı)
        std::string rest = lines[cursor.row].substr(cursor.col);
        std::string before = lines[cursor.row].substr(0, cursor.col);
        lines[cursor.row] = before;

        // Otomatik girinti: bir önceki satırın girintisini koru, satır
        // (boşluklar hariç) '{' ile bitiyorsa bir kademe daha içeri gir.
        int indent = leadingSpaces(before);
        std::string trimmed = before;
        while (!trimmed.empty() && trimmed.back() == ' ')
            trimmed.pop_back();
        if (!trimmed.empty() && trimmed.back() == '{')
            indent += TAB_WIDTH;

        std::string newLine(indent, ' ');
        newLine += rest;

        lines.insert(lines.begin() + cursor.row + 1, newLine);
        cursor.row++;
        cursor.col = indent;
    }

    void backspace()
    {
        if (cursor.col > 0)
        {
            // Girinti boşluklarındaysak bir tab durağına kadar birden sil.
            if (onlySpacesBeforeCursor() && cursor.col % TAB_WIDTH == 0)
            {
                int toDelete = TAB_WIDTH;
                lines[cursor.row].erase(cursor.col - toDelete, toDelete);
                cursor.col -= toDelete;
                return;
            }

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

    // Home/End: satır başı / satır sonu (VSCode'daki gibi).
    void moveHome() { cursor.col = 0; }
    void moveEnd() { cursor.col = (int)lines[cursor.row].size(); }

    // Ctrl+Left/Right'ta kullanılan kelime atlama. Önce mevcut boşlukları,
    // sonra bir "kelime"yi (harf/rakam/_) atlar; ne biri ne diğeri varsa
    // (ör. saf noktalama) tek karakter ilerler, böylece asla takılıp kalmaz.
    void moveWordLeft()
    {
        if (cursor.col == 0)
        {
            moveLeft();
            return;
        }
        const std::string &line = lines[cursor.row];
        int c = cursor.col;
        while (c > 0 && isspace((unsigned char)line[c - 1]))
            c--;
        int before = c;
        while (c > 0 && (isalnum((unsigned char)line[c - 1]) || line[c - 1] == '_'))
            c--;
        if (c == before && c > 0)
            c--;
        cursor.col = c;
    }

    void moveWordRight()
    {
        const std::string &line = lines[cursor.row];
        int n = (int)line.size();
        if (cursor.col >= n)
        {
            moveRight();
            return;
        }
        int c = cursor.col;
        while (c < n && isspace((unsigned char)line[c]))
            c++;
        int before = c;
        while (c < n && (isalnum((unsigned char)line[c]) || line[c] == '_'))
            c++;
        if (c == before && c < n)
            c++;
        cursor.col = c;
    }

    // Page Up/Down: `amount` satır yukarı/aşağı zıplar (görünür pencere
    // yüksekliği kadar), dosya sınırlarında güvenle durur.
    void pageUp(int amount)
    {
        cursor.row = std::max(0, cursor.row - amount);
        cursor.col = std::min(cursor.col, (int)lines[cursor.row].size());
    }

    void pageDown(int amount)
    {
        cursor.row = std::min((int)lines.size() - 1, cursor.row + amount);
        cursor.col = std::min(cursor.col, (int)lines[cursor.row].size());
    }
};