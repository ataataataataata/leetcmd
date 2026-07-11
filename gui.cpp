#include <ncurses.h>
#include <ctype.h>
#include "include/gui.h"
#include "include/html_parser.h"
#include "include/editor.h"

static int codeGutterWidth(const Editor &ed);
#include "include/leetcode_client.h"
#include "include/config_manager.h"
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <functional>

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
#define PAIR_BRACKET_MATCH 10

std::string selectedQuestionSlug;
std::string selectedQuestionId; // needed by submitCode() as question_id
std::string selectedLangSlug = "cpp";
std::string lastSubmittedCode; // code passed forward into the submit screen
std::string lastRunTestcases;  // sample testcases (joined) passed forward into the run screen
std::vector<std::string> lastRunTestcaseList; // same testcases, unjoined, for Input/Output pairing in the result view

// Which question/language lastSubmittedCode actually belongs to. Run and
// Submit both leave questionScreen() and come back to it, and without this
// questionScreen() has no way to tell "the user edited their code" apart
// from "this is a fresh visit to the question" -- it would just reload the
// pristine template from the API every time, silently discarding edits.
std::string lastEditedSlug;
std::string lastEditedLangSlug;

// --- Autocomplete keyword tables ---------------------------------------
// One entry per LeetCode langSlug, covering language keywords plus the
// standard-library types/functions a competitive programmer reaches for
// most often while solving a problem. Kept intentionally curated (not an
// exhaustive dump) so the suggestion list stays short and relevant.
static const std::unordered_map<std::string, std::vector<std::string>> languageKeywords = {
    {"cpp", {
        "include", "define", "namespace", "using", "std", "class", "struct",
        "public", "private", "protected", "virtual", "override", "template",
        "typename", "const", "static", "auto", "return", "if", "else", "for",
        "while", "do", "switch", "case", "break", "continue", "true", "false",
        "nullptr", "new", "delete", "sizeof", "void", "int", "long", "double",
        "float", "bool", "char", "string", "vector", "unordered_map",
        "unordered_set", "map", "set", "multimap", "multiset", "bitset",
        "pair", "tuple", "queue", "stack", "priority_queue", "deque", "array",
        "push_back", "pop_back", "emplace_back", "push", "pop", "top", "front",
        "back", "clear", "at", "first", "second", "begin", "end", "rbegin",
        "rend", "size", "empty", "resize", "reserve", "insert", "erase",
        "emplace", "find", "count", "contains", "sort", "reverse", "unique",
        "rotate", "fill", "iota", "max", "min", "abs", "gcd", "lcm", "swap",
        "make_pair", "make_tuple", "get", "lower_bound", "upper_bound",
        "binary_search", "accumulate", "count_if", "all_of", "any_of",
        "none_of", "nth_element", "partial_sort", "memset", "distance",
        "next", "prev", "greater", "less", "INT_MAX", "INT_MIN", "LONG_MAX",
        "LONG_MIN", "LLONG_MAX", "LLONG_MIN", "substr", "length", "append",
        "to_string", "stoi", "stol", "stoll", "stod", "isalpha", "isdigit",
        "tolower", "toupper", "numeric_limits", "cout", "cin", "endl"
    }},
    {"c", {
        "include", "define", "int", "long", "double", "float", "char", "void",
        "struct", "typedef", "enum", "union", "const", "static", "return",
        "if", "else", "for", "while", "do", "switch", "case", "break",
        "continue", "malloc", "calloc", "realloc", "free", "sizeof", "printf",
        "scanf", "strlen", "strcpy", "strcat", "strcmp", "memset", "memcpy",
        "qsort", "abs", "NULL", "INT_MAX", "INT_MIN", "true", "false"
    }},
    {"java", {
        "public", "private", "protected", "static", "final", "abstract",
        "class", "interface", "extends", "implements", "void", "int", "long",
        "double", "float", "boolean", "char", "String", "new", "return", "if",
        "else", "for", "while", "do", "switch", "case", "break", "continue",
        "try", "catch", "finally", "throw", "throws", "import", "package",
        "this", "super", "null", "true", "false", "instanceof",
        "ArrayList", "LinkedList", "HashMap", "TreeMap", "HashSet", "TreeSet",
        "PriorityQueue", "ArrayDeque", "List", "Map", "Set", "Queue", "Deque",
        "Stack", "Arrays", "Collections", "Comparator", "Optional", "Stream",
        "Collectors", "add", "remove", "get", "put", "size", "isEmpty",
        "contains", "containsKey", "containsValue", "keySet", "values",
        "entrySet", "poll", "offer", "peek", "push", "pop", "sort",
        "compareTo", "equals", "hashCode", "toString", "toCharArray",
        "charAt", "length", "substring", "split", "stream", "collect",
        "Math.max", "Math.min", "Math.abs", "Integer.MAX_VALUE",
        "Integer.MIN_VALUE", "StringBuilder", "System.out.println"
    }},
    {"python3", {
        "def", "class", "self", "return", "if", "elif", "else", "for", "while",
        "in", "not", "and", "or", "is", "import", "from", "as", "with", "try",
        "except", "finally", "raise", "lambda", "yield", "pass", "break",
        "continue", "global", "nonlocal", "None", "True", "False", "print",
        "len", "range", "list", "dict", "set", "tuple", "str", "int", "float",
        "sorted", "reversed", "enumerate", "zip", "map", "filter", "any",
        "all", "sum", "max", "min", "abs", "round", "isinstance", "super",
        "staticmethod", "classmethod", "property", "append", "extend", "pop",
        "insert", "remove", "sort", "join", "split", "strip", "replace",
        "format", "startswith", "endswith", "isdigit", "isalpha",
        "collections", "defaultdict", "OrderedDict", "Counter", "deque",
        "heapq", "heappush", "heappop", "bisect", "bisect_left",
        "bisect_right", "functools", "lru_cache", "reduce", "itertools",
        "permutations", "combinations", "product", "math", "floor", "ceil",
        "sqrt", "gcd", "inf"
    }},
    {"python", {
        "def", "class", "self", "return", "if", "elif", "else", "for", "while",
        "in", "not", "and", "or", "is", "import", "from", "as", "try",
        "except", "finally", "raise", "lambda", "yield", "pass", "print",
        "len", "range", "list", "dict", "set", "tuple", "str", "int", "float",
        "sorted", "reversed", "enumerate", "zip", "map", "filter", "any",
        "all", "sum", "max", "min", "abs", "append", "extend", "pop",
        "insert", "remove", "sort", "join", "split", "strip", "replace",
        "collections", "defaultdict", "Counter", "deque", "heapq", "True",
        "False", "None"
    }},
    {"javascript", {
        "function", "const", "let", "var", "return", "if", "else", "for",
        "while", "do", "switch", "case", "break", "continue", "class",
        "extends", "constructor", "this", "new", "typeof", "instanceof",
        "null", "undefined", "true", "false", "import", "export", "default",
        "async", "await", "try", "catch", "finally", "throw", "console.log",
        "Array", "Map", "Set", "Object", "Number", "String", "Boolean",
        "Infinity", "NaN", "JSON.stringify", "JSON.parse", "parseInt",
        "parseFloat", "push", "pop", "shift", "unshift", "slice", "splice",
        "concat", "join", "length", "map", "filter", "reduce", "forEach",
        "find", "findIndex", "includes", "indexOf", "some", "every", "sort",
        "reverse", "keys", "values", "entries", "Math.max", "Math.min",
        "Math.abs", "Math.floor", "Math.ceil", "Math.sqrt"
    }},
    {"typescript", {
        "function", "const", "let", "var", "return", "if", "else", "for",
        "while", "do", "switch", "case", "break", "continue", "class",
        "interface", "extends", "implements", "constructor", "this", "new",
        "typeof", "instanceof", "null", "undefined", "true", "false",
        "import", "export", "default", "async", "await", "try", "catch",
        "finally", "throw", "console.log", "Array", "Map", "Set", "Object",
        "number", "string", "boolean", "void", "any", "unknown", "never",
        "readonly", "push", "pop", "shift", "unshift", "slice", "splice",
        "concat", "join", "length", "map", "filter", "reduce", "forEach",
        "find", "findIndex", "includes", "indexOf", "some", "every", "sort",
        "reverse", "keys", "values", "entries", "Math.max", "Math.min",
        "Math.abs", "Math.floor", "Math.ceil", "Math.sqrt"
    }},
    {"csharp", {
        "using", "namespace", "public", "private", "protected", "static",
        "final", "abstract", "class", "interface", "int", "long", "double",
        "float", "bool", "char", "string", "var", "new", "return", "if",
        "else", "for", "foreach", "while", "do", "switch", "case", "break",
        "continue", "try", "catch", "finally", "throw", "null", "true",
        "false", "this", "base", "List", "Dictionary", "HashSet", "SortedSet",
        "SortedDictionary", "Queue", "Stack", "PriorityQueue", "Array",
        "Add", "Remove", "Contains", "ContainsKey", "TryGetValue", "Count",
        "Sort", "Select", "Where", "OrderBy", "ToList", "ToArray", "Push",
        "Pop", "Peek", "Enqueue", "Dequeue", "Math.Max", "Math.Min",
        "Math.Abs", "int.Parse", "Convert.ToInt32", "StringBuilder",
        "Console.WriteLine"
    }},
    {"golang", {
        "func", "package", "import", "var", "const", "type", "struct",
        "interface", "return", "if", "else", "for", "range", "switch",
        "case", "break", "continue", "go", "chan", "defer", "select",
        "make", "new", "len", "cap", "copy", "append", "delete", "nil",
        "true", "false", "map", "sort.Ints", "sort.Slice", "sort.Strings",
        "strings.Split", "strings.Join", "strings.Contains", "strconv.Itoa",
        "strconv.Atoi", "math.Max", "math.Min", "math.Abs", "math.MaxInt64",
        "fmt.Println", "fmt.Printf", "fmt.Sprintf"
    }},
    {"ruby", {
        "def", "end", "class", "module", "if", "elsif", "else", "unless",
        "while", "until", "for", "in", "do", "return", "yield", "nil",
        "true", "false", "self", "puts", "print", "require", "attr_accessor",
        "attr_reader", "attr_writer", "each", "each_with_index", "map",
        "map!", "select", "reject", "reduce", "inject", "sort", "sort_by",
        "min", "max", "sum", "include?", "push", "pop", "shift", "unshift",
        "join", "split", "gsub", "sub", "to_s", "to_i", "to_a", "length",
        "Hash.new", "Array.new"
    }},
    {"swift", {
        "func", "var", "let", "class", "struct", "enum", "protocol",
        "extension", "if", "else", "guard", "for", "in", "while", "switch",
        "case", "break", "continue", "return", "import", "true", "false",
        "nil", "self", "print", "map", "filter", "reduce", "sorted",
        "append", "removeLast", "removeFirst", "insert", "remove", "count",
        "isEmpty", "contains", "Set", "Array", "Dictionary", "Int.max",
        "Int.min", "guard let", "if let"
    }},
    {"kotlin", {
        "fun", "val", "var", "class", "object", "interface", "data",
        "if", "else", "when", "for", "in", "while", "do", "return", "true",
        "false", "null", "this", "super", "println", "listOf",
        "mutableListOf", "arrayListOf", "mapOf", "mutableMapOf", "setOf",
        "mutableSetOf", "filter", "map", "reduce", "fold", "sortedBy",
        "forEach", "also", "apply", "let", "run", "with", "Int.MAX_VALUE",
        "Int.MIN_VALUE"
    }},
    {"rust", {
        "fn", "let", "mut", "struct", "enum", "impl", "trait", "if", "else",
        "match", "for", "in", "while", "loop", "return", "true", "false",
        "self", "None", "Some", "Ok", "Err", "Option", "Result", "Box", "Rc",
        "RefCell", "println!", "vec!", "Vec", "HashMap", "HashSet",
        "BTreeMap", "BTreeSet", "VecDeque", "String", "push", "pop", "iter",
        "into_iter", "collect", "map", "filter", "fold", "sort", "sort_by",
        "unwrap", "i32::MAX", "i32::MIN"
    }},
    {"php", {
        "function", "class", "public", "private", "protected", "static",
        "if", "else", "elseif", "for", "foreach", "while", "do", "switch",
        "case", "break", "continue", "return", "echo", "print", "true",
        "false", "null", "array", "new", "this", "array_push", "array_pop",
        "array_shift", "array_unshift", "array_map", "array_filter",
        "array_reduce", "array_merge", "count", "sort", "usort", "in_array",
        "isset", "strlen", "substr", "str_split", "implode", "explode"
    }},
    {"scala", {
        "def", "val", "var", "class", "object", "trait", "case", "if",
        "else", "for", "while", "do", "match", "return", "true", "false",
        "this", "None", "Some", "List", "Map", "Set", "Array", "Vector",
        "println", "map", "filter", "reduce", "fold", "sortBy", "foreach",
        "mutable.ListBuffer", "mutable.Map"
    }},
};

// Returns the identifier ([A-Za-z0-9_]) immediately to the left of the
// cursor on the current line, i.e. the partial word the user is typing.
// Empty when the cursor isn't right after an identifier character.
static std::string wordPrefixAtCursor(const Editor &ed)
{
    const std::string &line = ed.lines[ed.cursor.row];
    int start = ed.cursor.col;
    while (start > 0 && (isalnum((unsigned char)line[start - 1]) || line[start - 1] == '_'))
        start--;
    return line.substr(start, ed.cursor.col - start);
}

// Kullanıcının kodda daha önce yazdığı, en az 3 karakterlik tanımlayıcıları
// (değişken/fonksiyon adları) toplar. LeetCode çözümlerinde sürekli
// tekrarlanan "result", "dp", "left", "right" gibi kendi isimlerini bir
// kere yazdıktan sonra autocomplete'in hatırlayıp önermesi için.
static std::vector<std::string> collectBufferIdentifiers(const Editor &ed)
{
    std::vector<std::string> ids;
    std::string current;
    for (const auto &line : ed.lines)
    {
        current.clear();
        for (char ch : line)
        {
            if (isalnum((unsigned char)ch) || ch == '_')
            {
                current += ch;
                continue;
            }
            if (current.size() >= 3 && !isdigit((unsigned char)current[0]))
                ids.push_back(current);
            current.clear();
        }
        if (current.size() >= 3 && !isdigit((unsigned char)current[0]))
            ids.push_back(current);
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

// Filters the keyword table for the given language, plus identifiers
// already used in `ed`'s buffer, down to entries that could complete
// `prefix`. Ranking: (1) dil anahtar kelimesi prefix eşleşmesi, (2) kendi
// yazdığı isimlerden prefix eşleşmesi, (3) dil anahtar kelimesi substring
// eşleşmesi (ör. "back" -> push_back/pop_back). Her grup alfabetik
// sıralı, popup kompakt kalsın diye toplam 8 ile sınırlı.
static std::vector<std::string> autocompleteSuggestions(const std::string &prefix, const std::string &langSlug, const Editor &ed)
{
    std::vector<std::string> prefixMatches, bufferMatches, substringMatches;
    if (prefix.empty())
        return prefixMatches;

    auto it = languageKeywords.find(langSlug);
    if (it != languageKeywords.end())
    {
        for (const auto &kw : it->second)
        {
            if (kw.size() <= prefix.size())
                continue;

            if (kw.compare(0, prefix.size(), prefix) == 0)
                prefixMatches.push_back(kw);
            else if (kw.find(prefix) != std::string::npos)
                substringMatches.push_back(kw);
        }
    }

    for (const auto &id : collectBufferIdentifiers(ed))
    {
        if (id.size() <= prefix.size())
            continue;
        if (id.compare(0, prefix.size(), prefix) != 0)
            continue;
        if (std::find(prefixMatches.begin(), prefixMatches.end(), id) != prefixMatches.end())
            continue; // zaten dil anahtar kelimesi olarak listede
        bufferMatches.push_back(id);
    }

    std::sort(prefixMatches.begin(), prefixMatches.end());
    std::sort(bufferMatches.begin(), bufferMatches.end());
    std::sort(substringMatches.begin(), substringMatches.end());

    std::vector<std::string> results = std::move(prefixMatches);
    for (auto &id : bufferMatches)
    {
        if (results.size() >= 8)
            break;
        results.push_back(id);
    }
    for (auto &kw : substringMatches)
    {
        if (results.size() >= 8)
            break;
        if (std::find(results.begin(), results.end(), kw) == results.end())
            results.push_back(kw);
    }

    if (results.size() > 8)
        results.resize(8);
    return results;
}

// Short signature/description shown next to a suggestion in the popup.
// Currently curated for C++ (the editor's default language and the one
// most of these problems get solved in); other languages just show the
// bare keyword with no hint. Extend this table -- or add sibling ones
// keyed by langSlug -- to cover more languages as needed.
static const std::unordered_map<std::string, std::string> cppFunctionHints = {
    {"push_back", "void push_back(const T&)"},
    {"pop_back", "void pop_back()"},
    {"emplace_back", "T& emplace_back(Args&&...)"},
    {"size", "size_t size() const"},
    {"empty", "bool empty() const"},
    {"clear", "void clear()"},
    {"resize", "void resize(size_t)"},
    {"reserve", "void reserve(size_t)"},
    {"begin", "iterator begin()"},
    {"end", "iterator end()"},
    {"rbegin", "reverse_iterator rbegin()"},
    {"rend", "reverse_iterator rend()"},
    {"at", "T& at(size_t)"},
    {"front", "T& front()"},
    {"back", "T& back()"},
    {"insert", "iterator insert(pos, val)"},
    {"erase", "iterator erase(pos)"},
    {"find", "iterator find(key)"},
    {"count", "size_t count(key)"},
    {"contains", "bool contains(key)"},
    {"push", "void push(const T&)"},
    {"pop", "void pop()"},
    {"top", "T& top()"},
    {"sort", "void sort(first, last)"},
    {"reverse", "void reverse(first, last)"},
    {"unique", "iterator unique(first, last)"},
    {"rotate", "void rotate(first, mid, last)"},
    {"fill", "void fill(first, last, val)"},
    {"max", "T max(a, b)"},
    {"min", "T min(a, b)"},
    {"abs", "T abs(T)"},
    {"gcd", "T gcd(a, b)"},
    {"swap", "void swap(a, b)"},
    {"make_pair", "pair<T1,T2> make_pair(a, b)"},
    {"make_tuple", "tuple<...> make_tuple(...)"},
    {"get", "T& get<I>(tuple/pair)"},
    {"lower_bound", "it lower_bound(first,last,val)"},
    {"upper_bound", "it upper_bound(first,last,val)"},
    {"binary_search", "bool binary_search(f,l,val)"},
    {"accumulate", "T accumulate(first,last,init)"},
    {"count_if", "size_t count_if(f,l,pred)"},
    {"all_of", "bool all_of(f,l,pred)"},
    {"any_of", "bool any_of(f,l,pred)"},
    {"none_of", "bool none_of(f,l,pred)"},
    {"nth_element", "void nth_element(f,nth,l)"},
    {"partial_sort", "void partial_sort(f,mid,l)"},
    {"memset", "void memset(ptr,val,n)"},
    {"distance", "diff_t distance(first,last)"},
    {"substr", "string substr(pos, len)"},
    {"length", "size_t length() const"},
    {"append", "string& append(str)"},
    {"to_string", "string to_string(val)"},
    {"stoi", "int stoi(const string&)"},
    {"stol", "long stol(const string&)"},
    {"stoll", "long long stoll(const string&)"},
    {"stod", "double stod(const string&)"},
    {"isalpha", "bool isalpha(int)"},
    {"isdigit", "bool isdigit(int)"},
    {"tolower", "int tolower(int)"},
    {"toupper", "int toupper(int)"},
};

// Joins a question's example testcases into the single newline-separated
// payload LeetCode's interpret_solution ("Run") endpoint expects as data_input.
static std::string joinTestcases(const std::vector<std::string> &testcases)
{
    std::string result;
    for (size_t i = 0; i < testcases.size(); i++)
    {
        result += testcases[i];
        if (i + 1 < testcases.size())
            result += "\n";
    }
    return result;
}

void initColors()
{
    start_color();
    use_default_colors();

    // Easy artık yeşil değil, turkuaz -- 256 renkli terminallerde gerçek bir
    // turkuaz tonu (44), desteklenmeyen terminallerde en yakın standart renk
    // olan cyan'a düşer.
    if (COLORS >= 256)
        init_pair(PAIR_EASY, 44, -1);
    else
        init_pair(PAIR_EASY, COLOR_CYAN, -1);
    init_pair(PAIR_MEDIUM, COLOR_YELLOW, -1);
    init_pair(PAIR_HARD, COLOR_RED, -1);

    // Use an orange-ish color on 256-color terminals, fall back to yellow otherwise.
    if (COLORS >= 256)
        init_pair(PAIR_LOGO_LEET, 208, -1);
    else
        init_pair(PAIR_LOGO_LEET, COLOR_YELLOW, -1);

    // "cmd" ve onunla aynı aksan rengini paylaşan tüm arayüz öğeleri
    // (autocomplete/dil seçici popup kenarlıkları, aktif satır numarası,
    // run/submit durum kutuları) artık düz beyaz -- eskiden cyan'dı, çok
    // fazla renk bir arada "gürültülü" duruyordu. Beyaz + turuncu (leet) +
    // zorluk renkleri (yeşil/sarı/kırmızı) ile palet daha sade.
    init_pair(PAIR_LOGO_CMD, COLOR_WHITE, -1);

    init_pair(PAIR_SUBMIT_OK, COLOR_GREEN, -1);
    init_pair(PAIR_SUBMIT_FAIL, COLOR_RED, -1);

    // Config screen now themed with the same leet(orange)/cmd(cyan) palette
    // used by the logo, instead of its own separate colors.
    if (COLORS >= 256)
        init_pair(PAIR_CONFIG_BORDER, 208, -1);
    else
        init_pair(PAIR_CONFIG_BORDER, COLOR_YELLOW, -1);
    init_pair(PAIR_CONFIG_LABEL, COLOR_CYAN, -1);

    // Highlight box for matching bracket pairs in the code editor
    // (VSCode-style): black text on a bright background so both halves of
    // a (), [] or {} pop out from the surrounding code.
    if (COLORS >= 256)
        init_pair(PAIR_BRACKET_MATCH, COLOR_BLACK, 214);
    else
        init_pair(PAIR_BRACKET_MATCH, COLOR_BLACK, COLOR_YELLOW);
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

// LeetCode'un problemsetQuestionListV2 API'si "status" alanında SOLVED /
// ATTEMPTED / TO_DO gibi değerler döndürüyor. Çözülmüş sorular yeşil,
// denenmiş-ama-çözülmemişler sarı; TO_DO (veya bilinmeyen bir değer) renksiz
// kalıyor -- zaten satırın normal rengiyle "henüz dokunulmamış" hissi veriyor.
int statusColorPair(const std::string &status)
{
    if (status == "SOLVED" || status == "ac")
        return PAIR_SUBMIT_OK;
    if (status == "ATTEMPTED" || status == "notac")
        return PAIR_MEDIUM;
    return 0; // TO_DO / bilinmiyor: renksiz
}

// Alt/üst kısayol çubuğunu terminal genişliğine göre kırparak basar.
// mvprintw taşan metni olduğu gibi terminale yazar; terminal de kendi
// içinde "satır kaydırma" yaparak metni bir alt satıra taşar -- eğer bu
// zaten ekranın en alt satırıysa, terminal tüm içeriği yukarı kaydırır ve
// ncurses'in çizdiği pencereler ekrandan "kaybolmuş" gibi görünür. Bunu
// önlemek için metni asla `cols - 2` karakterden uzun basmıyoruz.
static void drawHintBar(int row, int cols, const std::string &text)
{
    int maxWidth = std::max(0, cols - 2);
    std::string clipped = ((int)text.size() > maxWidth) ? text.substr(0, maxWidth) : text;

    attron(A_DIM);
    mvprintw(row, 1, "%s", clipped.c_str());
    attroff(A_DIM);
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

// '/' ile açılan tek satırlık arama kutusu. Logonun hemen altında, config
// kutusuyla aynı leet(border)/cmd(label) paletini kullanan küçük bir
// popup olarak çizilir. promptFieldScrolling'den farkı: burada ESC ile
// iptal edilebilir, çünkü aramada "vazgeçip mevcut listeye dön" mantıklı
// bir davranış -- config ekranında ise kimlik bilgisi girmeden çıkış
// yoktu zaten. `initial` ile kutu önceki arama terimiyle önceden dolu
// açılır, böylece kullanıcı terimi düzeltmek için baştan yazmak zorunda
// kalmaz. `cancelled` true dönerse çağıran taraf hiçbir state değiştirmez.
static std::string searchPromptBox(const std::string &initial, bool &cancelled)
{
    cancelled = false;

    int boxWidth = std::min(COLS - 4, 50);
    int boxHeight = 3;
    int boxY = LOGO_HEIGHT;
    int boxX = std::max(0, (COLS - boxWidth) / 2);

    WINDOW *win = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(win, TRUE);

    wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);

    wattron(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
    mvwprintw(win, 0, 2, " Search ");
    wattroff(win, COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);

    int fieldX = 1;
    int fieldWidth = boxWidth - 2;

    std::string input = initial;
    int pos = (int)input.size();
    int scrollOff = 0;

    curs_set(1);

    auto redraw = [&]()
    {
        if (pos < scrollOff)
            scrollOff = pos;
        else if (pos - scrollOff >= fieldWidth)
            scrollOff = pos - fieldWidth + 1;
        if (scrollOff < 0)
            scrollOff = 0;

        mvwprintw(win, 1, fieldX, "%*s", fieldWidth, "");

        int visibleLen = std::min((int)input.size() - scrollOff, fieldWidth);
        if (visibleLen < 0)
            visibleLen = 0;
        std::string visible = input.substr(scrollOff, visibleLen);

        mvwprintw(win, 1, fieldX, "%s", visible.c_str());
        wmove(win, 1, fieldX + (pos - scrollOff));
        wrefresh(win);
    };

    redraw();

    int ch;
    while (true)
    {
        ch = wgetch(win);

        if (ch == '\n' || ch == KEY_ENTER)
            break;

        if (ch == 27) // ESC: vazgeç, mevcut aramaya/listeye dokunma
        {
            cancelled = true;
            break;
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (pos > 0)
            {
                input.erase(pos - 1, 1);
                pos--;
            }
        }
        else if (ch == KEY_DC)
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

    curs_set(0);
    delwin(win);
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
        int totalLen = (int)(leet.size() + cmd.size());
        int titleX = std::max(1, (boxWidth - totalLen) / 2);

        wattron(win,A_BOLD);
        mvwprintw(win, 0, titleX, "%s", leet.c_str());
        wattroff(win,A_BOLD);

        wattron(win,A_BOLD);
        wprintw(win, "%s", cmd.c_str());
        wattroff(win,A_BOLD);


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

    wattron(win,A_BOLD);
    mvwprintw(win, line, 2, "LEETCODE_SESSION:");
    wattroff(win,A_BOLD);
    int sessionRow = line;
    line += 2;

    wattron(win,A_BOLD);
    mvwprintw(win, line, 2, "csrftoken:");
    wattroff(win,A_BOLD);
    int csrfRow = line;

    wrefresh(win);

    std::string session = promptFieldScrolling(win, sessionRow, fieldX, fieldWidth);

    drawChrome();
    for (auto &l : intro)
        ; // intro text is static; no need to redraw between fields other than chrome
    // Redraw both labels since drawChrome() only repaints the border/title.
    wattron(win,A_BOLD);
    mvwprintw(win, sessionRow, 2, "LEETCODE_SESSION:");
    mvwprintw(win, csrfRow, 2, "csrftoken:");
    wattroff(win,A_BOLD);
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
    // getAllQuestions() yerine sayfalı getQuestions(skip, limit) kullanılıyor.
    // Sorular tek seferde değil, kullanıcı listenin sonuna yaklaştıkça
    // sunucudan API_PAGE_SIZE'lık parçalar halinde çekiliyor (lazy load).
    static const int API_PAGE_SIZE = 100;

    std::vector<questionAtList> questions;
    int apiSkip = 0;
    bool hasMore = true;

    // Arama modundayken loadMore() sayfalamayı searchQuestions() üzerinden
    // yapar, değilse normal getQuestions() üzerinden -- ikisi de aynı
    // skip/limit/hasMore mantığını paylaştığı için tek bir lambda'ya
    // sığdırmak, listScreen'in geri kalanının (yukarı/aşağı, lazy-load
    // sınırına yaklaşınca çekme vs.) arama açıkken de değişmeden
    // çalışmasını sağlıyor.
    std::string searchTerm;
    bool searching = false;

    auto loadMore = [&]()
    {
        if (!hasMore)
            return;
        QuestionPage page = searching
                                 ? searchQuestions(apiSkip, API_PAGE_SIZE, searchTerm)
                                 : getQuestions(apiSkip, API_PAGE_SIZE);
        questions.insert(questions.end(), page.questions.begin(), page.questions.end());
        apiSkip += (int)page.questions.size();
        hasMore = page.hasMore;
    };

    loadMore(); // ilk sayfa

    int highlight = 0;
    int offset = 0;
    int list_total_width = 95;

    while (1)
    {
        int n = (int)questions.size();

        // page_size her karede yeniden hesaplanır: pencere yeniden
        // boyutlandırıldığında (KEY_RESIZE) bir kez hesaplanıp bir daha
        // güncellenmeyen eski (stale) bir değer kullanılırsa, liste ya
        // ekran sınırlarının dışına taşar ya da hiç görünmez.
        int page_size = std::max(1, LINES - LOGO_HEIGHT - 2);

        // Küçülen pencerede highlight/offset artık geçersiz
        // sınırların dışında kalmasın.
        if (highlight >= n && n > 0)
            highlight = n - 1;
        if (highlight < 0)
            highlight = 0;
        if (offset > highlight)
            offset = highlight;
        if (offset + page_size <= highlight)
            offset = highlight - page_size + 1;
        if (offset < 0)
            offset = 0;

        clear();

        int logo_width = 60;
        int logo_start_x = (COLS - logo_width) / 2;
        if (logo_start_x < 0)
            logo_start_x = 0;
        int list_start_x = (COLS - list_total_width) / 2;
        if (list_start_x < 0)
            list_start_x = 0;

        drawLogo();

        if (searching)
        {
            std::string status = "Search: \"" + searchTerm + "\"  (" +
                                  std::to_string(n) + (hasMore ? "+" : "") +
                                  (n == 1 ? " result" : " results") + ")";
            int maxWidth = std::max(0, COLS - list_start_x - 1);
            if ((int)status.size() > maxWidth)
                status = status.substr(0, maxWidth);

            attron(COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
            mvprintw(LOGO_HEIGHT, list_start_x, "%s", status.c_str());
            attroff(COLOR_PAIR(PAIR_CONFIG_LABEL) | A_BOLD);
        }

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

        drawHintBar(LINES - 1, COLS, "Up/Down: Navigate | Enter: Select | /: Search | P: Profile | Q: Quit");
        refresh();

        int c = getch();
        if (c == KEY_UP)
        {
            highlight--;
            if (highlight < 0)
            {
                // Listenin başından sona sarılıyor: gerçek sonu bilmek için
                // sunucuda kalan tüm sayfalar (varsa) çekilir.
                while (hasMore)
                    loadMore();
                n = (int)questions.size();
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

            // Görünür pencerenin sonuna yaklaşıldığında, sunucuda daha
            // fazla soru varsa bir sonraki sayfayı arka planda çek.
            if (highlight >= n - 1 && hasMore)
                loadMore();

            n = (int)questions.size();

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
        else if (c == '/')
        {
            bool cancelled = false;
            std::string term = searchPromptBox(searchTerm, cancelled);

            if (!cancelled)
            {
                // Baştaki/sondaki boşlukları at, sadece boşluktan oluşan bir
                // terim yanlışlıkla "boş değil" sayılıp gereksiz bir API
                // isteği tetiklemesin.
                size_t start = term.find_first_not_of(' ');
                term = (start == std::string::npos)
                           ? ""
                           : term.substr(start, term.find_last_not_of(' ') - start + 1);

                if (term.empty() && searching)
                {
                    // Boş terimle onaylamak aramadan çıkış demek: tam
                    // listeye geri dön.
                    searching = false;
                    searchTerm.clear();
                    questions.clear();
                    apiSkip = 0;
                    hasMore = true;
                    loadMore();
                    highlight = 0;
                    offset = 0;
                }
                else if (!term.empty())
                {
                    searching = true;
                    searchTerm = term;
                    questions.clear();
                    apiSkip = 0;
                    hasMore = true;
                    loadMore();
                    highlight = 0;
                    offset = 0;
                }
                // term boş ve zaten arama yoksa: hiçbir şey değişmedi,
                // popup kapanıp normal listeye dönülür.
            }
        }
        else if (c == 'p' || c == 'P')
        {
            return SCREEN_PROFILE;
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

// Interactive, live-filtering autocomplete popup. Unlike a one-shot "press
// Tab, pick, done" menu, this owns the input loop while it's open: typing
// more letters or Backspace re-filters the list in place (querying
// wordPrefixAtCursor/autocompleteSuggestions again each time), Up/Down
// browses, Enter/Tab accepts the highlighted entry and inserts the
// remainder into `ed`. The popup closes itself once the prefix no longer
// matches anything (or matches nothing to begin with).
//
// Any key it doesn't own (arrows other than Up/Down, punctuation, Ctrl
// shortcuts, resize, Esc, ...) closes the popup and is written to
// `pendingKey` instead of being swallowed, so the caller can feed it back
// through the normal editor key handling exactly as if autocomplete had
// never intervened. `pendingKey` is 0 if nothing needs to be redispatched
// (accepted, or cancelled with Esc).
//
// `redrawBase` repaints the full underlying screen (info+code panels,
// hint bar) from scratch. The popup's size/position changes every
// keystroke, and a plain delwin() doesn't erase what was already drawn on
// the physical terminal -- without this, a shrinking or moving popup left
// visible fragments of its previous frame on screen (looked like two
// suggestion boxes stacked on top of each other). Calling it right before
// every popup (re)draw guarantees the screen underneath is clean first.
static void runLiveAutocomplete(Editor &ed, const std::string &langSlug,
                                 int codeOriginY, int codeOriginX, int &pendingKey,
                                 const std::function<void()> &redrawBase)
{
    pendingKey = 0;
    WINDOW *popup = nullptr;
    int highlight = 0;

    auto closePopup = [&]()
    {
        if (popup)
        {
            delwin(popup);
            popup = nullptr;
        }
    };

    while (true)
    {
        std::string prefix = wordPrefixAtCursor(ed);
        std::vector<std::string> suggestions = autocompleteSuggestions(prefix, langSlug, ed);

        if (prefix.empty() || suggestions.empty())
        {
            closePopup();
            redrawBase(); // önceki popup'tan kalmış olabilecek izleri temizle
            return;
        }

        int n = (int)suggestions.size();
        if (highlight >= n)
            highlight = 0;

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        bool showHints = (langSlug == "cpp");
        int boxWidth = 8;
        for (auto &s : suggestions)
        {
            int w = (int)s.size() + 4;
            if (showHints)
            {
                auto hintIt = cppFunctionHints.find(s);
                if (hintIt != cppFunctionHints.end())
                    w += 3 + (int)hintIt->second.size();
            }
            boxWidth = std::max(boxWidth, w);
        }
        boxWidth = std::min(boxWidth, std::max(cols - 2, 10));

        int boxHeight = std::min(n + 2, rows - 2);
        int listHeight = boxHeight - 2;

        int cursorScreenRow = codeOriginY + (ed.cursor.row - ed.scrollOffset);
        int cursorScreenCol = codeOriginX + codeGutterWidth(ed) + ed.cursor.col;

        int boxY = cursorScreenRow + 1;
        if (boxY + boxHeight > rows) // no room below: flip above the cursor line
            boxY = std::max(0, cursorScreenRow - boxHeight);
        int boxX = std::min(cursorScreenCol, std::max(cols - boxWidth, 0));
        if (boxX < 0)
            boxX = 0;

        closePopup();  // size/position can change every keystroke -- rebuild cleanly
        redrawBase();  // ekranı temiz bir tuvale döndür, sonra popup'ı üstüne çiz
        popup = newwin(boxHeight, boxWidth, boxY, boxX);
        keypad(popup, TRUE);

        wattron(popup, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
        box(popup, 0, 0);
        wattroff(popup, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);

        int offset = (highlight >= listHeight) ? highlight - listHeight + 1 : 0;

        for (int i = offset; i < offset + listHeight && i < n; i++)
        {
            int y = 1 + (i - offset);
            int attrs = (i == highlight) ? A_REVERSE : A_NORMAL;
            wattron(popup, attrs);

            std::string line = suggestions[i];
            if (showHints)
            {
                auto hintIt = cppFunctionHints.find(suggestions[i]);
                if (hintIt != cppFunctionHints.end())
                    line += "   " + hintIt->second;
            }
            mvwprintw(popup, y, 1, "%-*.*s", boxWidth - 2, boxWidth - 2, line.c_str());
            wattroff(popup, attrs);
        }

        wrefresh(popup);
        flushinp();
        timeout(-1);

        int c = wgetch(popup);

        if (c == KEY_UP)
            highlight = (highlight - 1 + n) % n;
        else if (c == KEY_DOWN)
            highlight = (highlight + 1) % n;
        else if (c == '\n' || c == KEY_ENTER || c == '\t')
        {
            std::string remainder = suggestions[highlight].substr(prefix.size());
            for (char ch : remainder)
                ed.insertChar(ch);
            closePopup();
            redrawBase();
            return;
        }
        else if (c == 27) // Esc: cancel, leave whatever was already typed as-is
        {
            closePopup();
            redrawBase();
            return;
        }
        else if (c == KEY_BACKSPACE || c == 127 || c == 8)
        {
            ed.backspace(); // loop again: prefix/suggestions recomputed at the top
        }
        else if (isprint(c) && (isalnum((unsigned char)c) || c == '_'))
        {
            ed.insertCharSmart((char)c); // still building the identifier: keep filtering live
        }
        else
        {
            // Anything else (Left/Right, punctuation that ends the
            // identifier, Ctrl shortcuts, KEY_RESIZE, ...): close and hand
            // the key back so it's handled exactly like it would be without
            // autocomplete in the picture.
            closePopup();
            redrawBase();
            pendingKey = c;
            return;
        }
    }
}


// Satır numarası sütununun genişliğini toplam satır sayısına göre belirler
// (ör. 999 satıra kadar 3 hane + 1 boşluk = 4 kolon), böylece kod uzadıkça
// gutter da büyür ama gereksiz yere geniş olmaz.
static int codeGutterWidth(const Editor &ed)
{
    int digits = 1;
    int n = (int)ed.lines.size();
    while (n >= 10)
    {
        n /= 10;
        digits++;
    }
    return digits + 2; // rakamlar + numaradan sonra 1 boşluk + ayraç boşluğu
}

// Buradan sonraki üç yardımcı, editöre parantez/süslü parantez eşleştirme
// ve "bu kapanış hangi bloğa ait" ipucu (VSCode/JetBrains'teki gibi)
// eklemek için kullanılıyor.

// Her satır için hangi karakterlerin "gerçek kod" (true), hangilerinin
// string/char literal veya yorum içinde (false) olduğunu işaretler.
// Böylece "if (c == '}')" gibi bir satırdaki sahte parantez/süslü parantez
// karakterleri eşleştirmeyi bozmaz.
static std::vector<std::vector<bool>> computeCodeMask(const std::vector<std::string> &lines)
{
    std::vector<std::vector<bool>> mask(lines.size());
    bool inBlockComment = false;

    for (size_t r = 0; r < lines.size(); r++)
    {
        const std::string &line = lines[r];
        mask[r].assign(line.size(), true);
        bool inString = false, inChar = false;

        for (size_t c = 0; c < line.size(); c++)
        {
            char ch = line[c];

            if (inBlockComment)
            {
                mask[r][c] = false;
                if (ch == '*' && c + 1 < line.size() && line[c + 1] == '/')
                {
                    mask[r][c + 1] = false;
                    c++;
                    inBlockComment = false;
                }
                continue;
            }
            if (inString)
            {
                mask[r][c] = false;
                if (ch == '\\' && c + 1 < line.size())
                {
                    mask[r][c + 1] = false;
                    c++;
                    continue;
                }
                if (ch == '"')
                    inString = false;
                continue;
            }
            if (inChar)
            {
                mask[r][c] = false;
                if (ch == '\\' && c + 1 < line.size())
                {
                    mask[r][c + 1] = false;
                    c++;
                    continue;
                }
                if (ch == '\'')
                    inChar = false;
                continue;
            }
            if (ch == '/' && c + 1 < line.size() && line[c + 1] == '/')
            {
                for (size_t k = c; k < line.size(); k++)
                    mask[r][k] = false;
                break;
            }
            if (ch == '/' && c + 1 < line.size() && line[c + 1] == '*')
            {
                mask[r][c] = false;
                mask[r][c + 1] = false;
                c++;
                inBlockComment = true;
                continue;
            }
            if (ch == '"')
            {
                inString = true;
                continue;
            }
            if (ch == '\'')
            {
                inChar = true;
                continue;
            }
        }
    }

    return mask;
}

// (row, col)'daki karakter bir parantez/süslü parantez/köşeli parantezse,
// eşini bulur. Sadece mask[]'e göre "gerçek kod" sayılan karakterleri
// dikkate alır, string/yorum içindekileri yok sayar.
static bool findMatchingBracket(const Editor &ed, const std::vector<std::vector<bool>> &mask,
                                 int row, int col, int &mRow, int &mCol)
{
    static const std::string OPENERS = "([{";
    static const std::string CLOSERS = ")]}";

    if (row < 0 || row >= (int)ed.lines.size())
        return false;
    const std::string &line = ed.lines[row];
    if (col < 0 || col >= (int)line.size())
        return false;
    if (col >= (int)mask[row].size() || !mask[row][col])
        return false;

    char ch = line[col];
    size_t openIdx = OPENERS.find(ch);
    size_t closeIdx = CLOSERS.find(ch);
    if (openIdx == std::string::npos && closeIdx == std::string::npos)
        return false;

    if (openIdx != std::string::npos)
    {
        char open = ch, close = CLOSERS[openIdx];
        int depth = 0;
        for (int r = row; r < (int)ed.lines.size(); r++)
        {
            const std::string &l = ed.lines[r];
            int startC = (r == row) ? col : 0;
            for (int c = startC; c < (int)l.size(); c++)
            {
                if (c >= (int)mask[r].size() || !mask[r][c])
                    continue;
                if (l[c] == open)
                    depth++;
                else if (l[c] == close)
                {
                    depth--;
                    if (depth == 0)
                    {
                        mRow = r;
                        mCol = c;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    else
    {
        char close = ch, open = OPENERS[closeIdx];
        int depth = 0;
        for (int r = row; r >= 0; r--)
        {
            const std::string &l = ed.lines[r];
            int startC = (r == row) ? col : (int)l.size() - 1;
            for (int c = startC; c >= 0; c--)
            {
                if (c >= (int)mask[r].size() || !mask[r][c])
                    continue;
                if (l[c] == close)
                    depth++;
                else if (l[c] == open)
                {
                    depth--;
                    if (depth == 0)
                    {
                        mRow = r;
                        mCol = c;
                        return true;
                    }
                }
            }
        }
        return false;
    }
}

// Basit, harici bağımlılık gerektirmeyen bir "Format Code" (Ctrl+F).
// clang-format kadar kapsamlı değil, ama VSCode/JetBrains'teki gibi
// süslü parantez derinliğine göre otomatik yeniden girintileme yapar:
// satır sonu boşlukları temizlenir ve her satır, o satıra kadar açık
// kalan {, ( ve [ sayısına göre TAB_WIDTH katları kadar içeri kaydırılır.
// computeCodeMask() sayesinde string/char literal veya yorum içindeki
// sahte parantezler ("if (c == '}')" gibi) sayıma dahil edilmez.
static void formatCode(Editor &ed)
{
    if (ed.lines.empty())
        return;

    auto mask = computeCodeMask(ed.lines);
    int depth = 0;

    for (int i = 0; i < (int)ed.lines.size(); i++)
    {
        std::string &line = ed.lines[i];

        size_t end = line.size();
        while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\t'))
            end--;
        line.erase(end);

        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t'))
            start++;
        std::string content = line.substr(start);

        if (content.empty())
        {
            line.clear();
            continue;
        }

        const auto &rowMask = (i < (int)mask.size()) ? mask[i] : std::vector<bool>();
        auto isCode = [&](size_t col)
        { return col >= rowMask.size() || rowMask[col]; };

        // Kapanış parantezi/süslü parantez/köşeli parantezle başlayan bir
        // satır, bir kademe daha az girintilenir (bloğu açan satırla aynı
        // hizaya gelsin diye) - ama bu sadece o satırın kendi girintisi
        // için geçerli; `depth`'in kalıcı değeri aşağıdaki taramada güncellenir.
        int lineDepth = depth;
        if (isCode(start) && (content[0] == '}' || content[0] == ')' || content[0] == ']'))
            lineDepth = std::max(0, depth - 1);

        line = std::string(lineDepth * Editor::TAB_WIDTH, ' ') + content;

        for (size_t c = 0; c < content.size(); c++)
        {
            if (!isCode(start + c))
                continue;
            char ch = content[c];
            if (ch == '{' || ch == '(' || ch == '[')
                depth++;
            else if (ch == '}' || ch == ')' || ch == ']')
                depth = std::max(0, depth - 1);
        }
    }

    ed.cursor.row = std::min(ed.cursor.row, (int)ed.lines.size() - 1);
    ed.cursor.col = std::min(ed.cursor.col, (int)ed.lines[ed.cursor.row].size());
    ed.scrollOffset = 0;
    ed.hScrollOffset = 0;
}

static void renderEditor(WINDOW *pad, Editor &ed, int visibleHeight, int visibleWidth)
{
    // Satırlar çok uzarsa (satır sayısı ya da tek bir satırın karakter
    // sayısı) pad'i büyüt. wresize aynı WINDOW* üzerinde çalışır, pointer
    // değişmez -- pad her keystroke'ta yeniden oluşturulmaz.
    int gutterWidthForSize = codeGutterWidth(ed);
    int maxLineLen = 0;
    for (auto &l : ed.lines)
        maxLineLen = std::max(maxLineLen, (int)l.size());

    int neededRows = std::max((int)ed.lines.size() + 50, visibleHeight);
    int neededCols = std::max({visibleWidth, maxLineLen + gutterWidthForSize + 40, 200});

    int curRows, curCols;
    getmaxyx(pad, curRows, curCols);
    if (neededRows > curRows || neededCols > curCols)
        wresize(pad, std::max(neededRows, curRows), std::max(neededCols, curCols));

    werase(pad);

    // Keep the cursor inside the visible window by adjusting scrollOffset (dikey).
    if (ed.cursor.row < ed.scrollOffset)
        ed.scrollOffset = ed.cursor.row;
    else if (ed.cursor.row >= ed.scrollOffset + visibleHeight)
        ed.scrollOffset = ed.cursor.row - visibleHeight + 1;
    if (ed.scrollOffset < 0)
        ed.scrollOffset = 0;

    // Satır numarası gutter'ı: LeetCode/VSCode benzeri sol kenar sütunu.
    int gutterWidth = codeGutterWidth(ed);

    // Yatay scroll: imleç metin alanının solunda/sağında kalırsa
    // hScrollOffset'i güncelle. Gutter genişliği çıkarıldıktan sonra kalan
    // alan gerçek yazı alanı (textAreaWidth).
    int textAreaWidth = std::max(1, visibleWidth - gutterWidth);
    if (ed.cursor.col < ed.hScrollOffset)
        ed.hScrollOffset = ed.cursor.col;
    else if (ed.cursor.col >= ed.hScrollOffset + textAreaWidth)
        ed.hScrollOffset = ed.cursor.col - textAreaWidth + 1;
    if (ed.hScrollOffset < 0)
        ed.hScrollOffset = 0;

    // Gutter'ı pad'in hScrollOffset kolonundan başlatarak çiziyoruz; böylece
    // prefresh çağrısı pad'i hScrollOffset kadar sağdan başlatınca, gutter
    // ekranın en solunda "yapışık" kalır ve sadece kod metni kayar.
    int textStart = ed.hScrollOffset + gutterWidth;

    std::vector<std::vector<bool>> mask = computeCodeMask(ed.lines);

    // İmlecin üstünde ya da hemen solunda bir parantez/süslü parantez
    // varsa eşini bul; ikisini de vurgulayacağız.
    int matchRow = -1, matchCol = -1, cursorBracketRow = -1, cursorBracketCol = -1;
    {
        int cr = ed.cursor.row, cc = ed.cursor.col;
        if (findMatchingBracket(ed, mask, cr, cc, matchRow, matchCol))
        {
            cursorBracketRow = cr;
            cursorBracketCol = cc;
        }
        else if (cc > 0 && findMatchingBracket(ed, mask, cr, cc - 1, matchRow, matchCol))
        {
            cursorBracketRow = cr;
            cursorBracketCol = cc - 1;
        }
    }

    for (int i = 0; i < (int)ed.lines.size(); i++)
    {
        bool isCurrentLine = (i == ed.cursor.row);

        // Aktif satırın numarası: normalde dim, imlecin olduğu satırda
        // bold + renkli -- hangi satırda olduğunu ekranda kaymadan görmek
        // için (özellikle yatay scroll açıkken çok işe yarıyor).
        if (isCurrentLine)
            wattron(pad, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
        else
            wattron(pad, A_DIM);
        mvwprintw(pad, i, ed.hScrollOffset, "%*d", gutterWidth - 1, i + 1);
        if (isCurrentLine)
            wattroff(pad, COLOR_PAIR(PAIR_LOGO_CMD) | A_BOLD);
        else
            wattroff(pad, A_DIM);

        mvwprintw(pad, i, textStart, "%s", ed.lines[i].c_str());
    }

    // Eşleşen parantez çiftini vurgula.
    if (matchRow != -1)
    {
        wattron(pad, COLOR_PAIR(PAIR_BRACKET_MATCH) | A_BOLD);
        mvwaddch(pad, cursorBracketRow, ed.hScrollOffset + gutterWidth + cursorBracketCol,
                 (unsigned char)ed.lines[cursorBracketRow][cursorBracketCol] | COLOR_PAIR(PAIR_BRACKET_MATCH) | A_BOLD);
        mvwaddch(pad, matchRow, ed.hScrollOffset + gutterWidth + matchCol,
                 (unsigned char)ed.lines[matchRow][matchCol] | COLOR_PAIR(PAIR_BRACKET_MATCH) | A_BOLD);
        wattroff(pad, COLOR_PAIR(PAIR_BRACKET_MATCH) | A_BOLD);
    }

    // Draw the cursor as a reverse-video block over the character beneath it
    // (or a blank space if the cursor sits past the end of the line).
    chtype under = ' ';
    if (ed.cursor.col < (int)ed.lines[ed.cursor.row].size())
        under = (unsigned char)ed.lines[ed.cursor.row][ed.cursor.col];
    mvwaddch(pad, ed.cursor.row, ed.hScrollOffset + gutterWidth + ed.cursor.col, under | A_REVERSE);
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
    bool haveEditedCode = (!lastSubmittedCode.empty() && lastEditedSlug == selectedQuestionSlug);

    if (haveEditedCode)
    {
        // Coming back from Run/Submit for this exact question: restore
        // what the user actually had in the editor instead of overwriting
        // it with the pristine template below.
        codeText = lastSubmittedCode;
        selectedLangSlug = lastEditedLangSlug;
    }
    else
    {
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

    // Split iki panelin (bilgi + kod) anlamlı biçimde sığması için
    // gereken en küçük terminal boyutu. Bundan küçük bir terminalde
    // newwin() negatif/sıfır boyutla çağrılabiliyordu; bu da pencereleri
    // NULL bırakıp ekranın sadece alt bilgi çubuğuyla boş görünmesine
    // (tam olarak bildirilen hataya) yol açıyordu.
    static const int MIN_ROWS = 12;
    static const int MIN_COLS = 50;
    bool screenTooSmall = false;

    auto closeWindows = [&]()
    {
        if (questionInformation)
            delwin(questionInformation);
        if (questionCode)
            delwin(questionCode);
        if (infoPad)
            delwin(infoPad);
        if (codePad)
            delwin(codePad);
        questionInformation = questionCode = nullptr;
        infoPad = codePad = nullptr;
    };

    auto setupWindows = [&]()
    {
        getmaxyx(stdscr, rows, cols);

        closeWindows();

        screenTooSmall = (rows < MIN_ROWS || cols < MIN_COLS);
        if (screenTooSmall)
            return; // pencereler NULL kalır; refreshAll() bir uyarı basar

        half = cols / 2;
        if (half < 10)
            half = 10;

        winHeight = rows - 1; // bottom line reserved for the shortcut bar

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
        if (screenTooSmall)
        {
            clear();
            std::string msg = "Terminal too small - please enlarge the window";
            std::string msg2 = "(min " + std::to_string(MIN_COLS) + "x" + std::to_string(MIN_ROWS) + ", Q to go back)";
            int midY = std::max(0, rows / 2);
            mvprintw(midY, std::max(0, (cols - (int)msg.size()) / 2), "%s", msg.c_str());
            mvprintw(midY + 1, std::max(0, (cols - (int)msg2.size()) / 2), "%s", msg2.c_str());
            refresh();
            return;
        }

        wattron(questionInformation, focus == FOCUS_INFO ? A_BOLD : A_NORMAL);
        box(questionInformation, 0, 0);
        wattroff(questionInformation, A_BOLD);

        wattron(questionCode, focus == FOCUS_CODE ? A_BOLD : A_NORMAL);
        box(questionCode, 0, 0);
        wattroff(questionCode, A_BOLD);

        renderEditor(codePad, ed, visibleHeight, codeWidth);

        drawHintBar(rows - 1, cols, "F2/Shift+TAB: Switch Panel | TAB: Indent/Autocomplete | Ctrl+F: Format | Arrows: Move/Scroll | Enter/Backspace: Edit | R: Run | S: Submit | L: Language | Q: Back");

        // questionInformation/questionCode (normal windows) and stdscr (the
        // footer line) all cover overlapping screen real-estate. Mixing
        // wrefresh()/prefresh()/refresh() here used to call doupdate() FOUR
        // separate times per frame -- each of those calls syncs the physical
        // terminal from whatever ncurses currently considers "changed", and
        // the final plain refresh() (== wrefresh(stdscr)) could win that
        // race and paint stdscr's mostly-blank buffer over the panels that
        // had just been drawn, leaving only the footer line visible. Queuing
        // every window with *noutrefresh (no physical sync) and calling
        // doupdate() exactly once composites all of them together in a
        // single atomic pass, so nothing can stomp anything else.
        wnoutrefresh(questionInformation);
        wnoutrefresh(questionCode);
        pnoutrefresh(infoPad, infoScroll, 0, 1, 1, winHeight - 2, half - 2);
        pnoutrefresh(codePad, ed.scrollOffset, ed.hScrollOffset, 1, half + 1, winHeight - 2, cols - 2);
        wnoutrefresh(stdscr);
        doupdate();
    };

    setupWindows();
    refreshAll();

    while (1)
    {
        int c = getch();

        if (c == KEY_RESIZE)
        {
            // Modern ncurses'in kendi SIGWINCH/KEY_RESIZE algılaması zaten
            // getch() KEY_RESIZE döndürmeden ÖNCE LINES/COLS'u doğru şekilde
            // güncelliyor -- resize_term(0,0) çağırmak buna gerek bırakmıyor
            // ve daha kötüsü, curscr'nin (doupdate()'in fiziksel ekranla
            // karşılaştırdığı iç referans) boyut takibini bozup sonraki
            // doupdate() çağrısının bazı hücrelerin zaten doğru olduğunu
            // yanlışlıkla sanıp onları hiç yazmamasına yol açabiliyordu --
            // tam da gözlemlenen belirti (resize öncesi sorunsuz, resize
            // sonrası panel/pad içeriği kayboluyor) bununla örtüşüyor.
            // endwin()+refresh() curscr'yi tamamen geçersiz kılar, böylece
            // sıradaki doupdate() ekranın tamamını garanti şekilde yeniden
            // çizer.
            endwin();
            refresh();
            clear();
            refresh();
            setupWindows();
        }
        else if (screenTooSmall)
        {
            // Pencereler yok (NULL): sadece çıkışa izin ver, başka hiçbir
            // tuşu editör/panel mantığına yönlendirme (aksi halde NULL
            // pencerelere erişilip çökebilir).
            if (c == 'q' || c == 'Q')
                break;
        }
        else if (c == '\t')
        {
            // Kod panelindeyken Tab artık paneli DEĞİŞTİRMEZ; önce
            // autocomplete dener (imleç bir kelimenin hemen sonundaysa),
            // eşleşme yoksa normal bir editördeki gibi girinti (4 boşluk)
            // ekler. Panel değiştirmek için Shift+Tab kullanılır — böylece
            // kod yazarken kullanıcı hiç beklenmedik şekilde info paneline
            // fırlatılmaz.
            bool handledAsAutocomplete = false;

            if (focus == FOCUS_CODE)
            {
                std::string prefix = wordPrefixAtCursor(ed);
                if (!prefix.empty())
                {
                    std::vector<std::string> suggestions = autocompleteSuggestions(prefix, selectedLangSlug, ed);
                    if (!suggestions.empty())
                    {
                        int pendingKey = 0;
                        runLiveAutocomplete(ed, selectedLangSlug, 1, half + 1, pendingKey, refreshAll);
                        handledAsAutocomplete = true;
                    }
                }

                if (!handledAsAutocomplete)
                    ed.insertTab();
            }
            else
            {
                focus = FOCUS_CODE;
            }
        }
        else if (c == KEY_BTAB || c == KEY_F(2)) // Shift+Tab veya F2: panel değiştir (Tab artık girinti için)
        {
            focus = (focus == FOCUS_INFO) ? FOCUS_CODE : FOCUS_INFO;
        }
        else if (focus == FOCUS_INFO)
        {
            // Description panel is read-only: arrows/paging just scroll it.
            int maxScroll = std::max(0, infoPadHeight - visibleHeight);

            if (c == KEY_UP)
            {
                if (infoScroll > 0)
                    infoScroll--;
            }
            else if (c == KEY_DOWN)
            {
                if (infoScroll < maxScroll)
                    infoScroll++;
            }
            else if (c == KEY_PPAGE)
                infoScroll = std::max(0, infoScroll - std::max(1, visibleHeight - 1));
            else if (c == KEY_NPAGE)
                infoScroll = std::min(maxScroll, infoScroll + std::max(1, visibleHeight - 1));
            else if (c == KEY_HOME)
                infoScroll = 0;
            else if (c == KEY_END)
                infoScroll = maxScroll;
            else if (c == 'r' || c == 'R')
            {
                lastSubmittedCode = ed.toString();
                lastEditedSlug = selectedQuestionSlug;
                lastEditedLangSlug = selectedLangSlug;
                lastRunTestcaseList = q.exampleTestcaseList;
                lastRunTestcases = joinTestcases(lastRunTestcaseList);

                closeWindows();

                return SCREEN_RUN;
            }
            else if (c == 's' || c == 'S')
            {
                lastSubmittedCode = ed.toString();
                lastEditedSlug = selectedQuestionSlug;
                lastEditedLangSlug = selectedLangSlug;

                closeWindows();

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
            // Paste detection: a terminal paste delivers its bytes to
            // ncurses one getch() at a time, but back-to-back with no
            // human-typing gaps between them. So whenever we're about to
            // handle a char that would modify the buffer, peek (non-
            // blocking) for another char already queued up. If one is
            // there, this is (almost certainly) a paste burst, not a
            // keystroke: drain the whole burst and insert it literally,
            // bypassing auto-indent, auto-closing brackets/quotes and the
            // live-autocomplete popup -- all of which are exactly what
            // mangles pasted code (extra indentation piling up line after
            // line, doubled closing brackets, characters stolen by the
            // popup's own getch()).
            bool pasting = false;
            if (isprint(c) || c == '\n' || c == KEY_ENTER)
            {
                nodelay(stdscr, TRUE);
                int probe = getch();
                nodelay(stdscr, FALSE);
                if (probe != ERR)
                {
                    ungetch(probe);
                    pasting = true;
                }
            }

            if (pasting)
            {
                ungetch(c); // put the first char back so the drain loop below handles it uniformly
                while (true)
                {
                    nodelay(stdscr, TRUE);
                    int pc = getch();
                    nodelay(stdscr, FALSE);
                    if (pc == ERR)
                        break; // burst drained

                    if (pc == '\r')
                        continue; // some terminals send CRLF; skip the CR half

                    if (pc == '\n' || pc == KEY_ENTER)
                    {
                        // Plain line split, no auto-indent: pasted text
                        // already carries its own indentation.
                        std::string rest = ed.lines[ed.cursor.row].substr(ed.cursor.col);
                        std::string before = ed.lines[ed.cursor.row].substr(0, ed.cursor.col);
                        ed.lines[ed.cursor.row] = before;
                        ed.lines.insert(ed.lines.begin() + ed.cursor.row + 1, rest);
                        ed.cursor.row++;
                        ed.cursor.col = 0;
                    }
                    else if (pc == '\t')
                    {
                        ed.insertTab();
                    }
                    else if (pc == KEY_BACKSPACE || pc == 127 || pc == 8)
                    {
                        ed.backspace();
                    }
                    else if (isprint(pc))
                    {
                        ed.insertChar((char)pc); // raw insert: no bracket/quote auto-closing
                    }
                    // Any other control byte caught mid-burst is dropped
                    // rather than misinterpreted as a shortcut.
                }
            }
            else if (c == KEY_UP)
                ed.moveUp();
            else if (c == KEY_DOWN)
                ed.moveDown();
            else if (c == KEY_LEFT)
                ed.moveLeft();
            else if (c == KEY_RIGHT)
                ed.moveRight();
            else if (c == KEY_HOME)
                ed.moveHome();
            else if (c == KEY_END)
                ed.moveEnd();
            else if (c == KEY_PPAGE) // Page Up
                ed.pageUp(std::max(1, visibleHeight - 1));
            else if (c == KEY_NPAGE) // Page Down
                ed.pageDown(std::max(1, visibleHeight - 1));
            else if (c == '\n' || c == KEY_ENTER)
                ed.newline();
            else if (c == KEY_BACKSPACE || c == 127 || c == 8)
                ed.backspace();
            else if (c == 18) // Ctrl+R as a run-against-samples shortcut while editing
            {
                lastSubmittedCode = ed.toString();
                lastEditedSlug = selectedQuestionSlug;
                lastEditedLangSlug = selectedLangSlug;
                lastRunTestcaseList = q.exampleTestcaseList;
                lastRunTestcases = joinTestcases(lastRunTestcaseList);

                closeWindows();

                return SCREEN_RUN;
            }
            else if (c == 19) // Ctrl+S as a submit shortcut while editing
            {
                lastSubmittedCode = ed.toString();
                lastEditedSlug = selectedQuestionSlug;
                lastEditedLangSlug = selectedLangSlug;

                closeWindows();

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
            else if (c == 6) // Ctrl+F: reindent the whole buffer (see formatCode())
            {
                formatCode(ed);
            }
            else if (isprint(c))
            {
                ed.insertCharSmart((char)c);

                // Live autocomplete: once at least 2 identifier characters
                // are typed, pop the suggestion list up immediately instead
                // of waiting for an explicit Tab press. The popup keeps
                // filtering as more characters come in and hands back
                // (via pendingKey) the first key it doesn't own, so nothing
                // typed while it's open is ever silently dropped.
                if (isalnum((unsigned char)c) || c == '_')
                {
                    std::string prefix = wordPrefixAtCursor(ed);
                    if (prefix.size() >= 1)
                    {
                        int pendingKey = 0;
                        runLiveAutocomplete(ed, selectedLangSlug, 1, half + 1, pendingKey, refreshAll);

                        if (pendingKey == KEY_UP)
                            ed.moveUp();
                        else if (pendingKey == KEY_DOWN)
                            ed.moveDown();
                        else if (pendingKey == KEY_LEFT)
                            ed.moveLeft();
                        else if (pendingKey == KEY_RIGHT)
                            ed.moveRight();
                        else if (pendingKey == KEY_HOME)
                            ed.moveHome();
                        else if (pendingKey == KEY_END)
                            ed.moveEnd();
                        else if (pendingKey == KEY_PPAGE)
                            ed.pageUp(std::max(1, visibleHeight - 1));
                        else if (pendingKey == KEY_NPAGE)
                            ed.pageDown(std::max(1, visibleHeight - 1));
                        else if (pendingKey == '\n' || pendingKey == KEY_ENTER)
                            ed.newline();
                        else if (pendingKey == KEY_BACKSPACE || pendingKey == 127 || pendingKey == 8)
                            ed.backspace();
                        else if (pendingKey != 0 && isprint(pendingKey))
                            ed.insertCharSmart((char)pendingKey);
                        // Ctrl+R/S/Q/L and anything else pressed while the
                        // popup was open are intentionally not redispatched
                        // here to keep this fast-path simple -- press again.
                    }
                }
            }
        }

        refreshAll();
    }

    closeWindows();

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

// Runs the current code against the question's sample testcases only
// (LeetCode's "Run" — fast feedback, no official submission), then shows a
// per-testcase Output vs Expected breakdown. Structurally mirrors
// submitScreen()'s upload -> poll -> scrollable result box flow.
Screen runScreen()
{
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int frame = 0;

    // --- Phase 1: sending the code to the interpreter ---
    for (int i = 0; i < 5; i++)
    {
        drawStatusBox(rows, cols, "Running your code...", frame++);
        napms(100);
    }

    // --- Real run + poll flow using leetcode_client.h ---
    std::string verdict = "Unknown Error";
    int colorPair = PAIR_SUBMIT_FAIL;
    std::vector<std::string> detailLines;

    try
    {
        runResponse rr = runCode(lastRunTestcases, selectedLangSlug,
                                  selectedQuestionId, lastSubmittedCode, selectedQuestionSlug);

        // Drop any keys mashed while waiting so the result box doesn't
        // instantly swallow one on its first getch(), same reasoning as submitScreen().
        flushinp();

        // interpret_solution is asynchronous too: poll until a statusCode
        // shows up (0 == still judging), showing an animated spinner meanwhile.
        runDetail rd;
        const int maxAttempts = 20;
        for (int attempt = 0; attempt < maxAttempts; attempt++)
        {
            std::string msg = "Running against sample testcases";
            for (int d = 0; d <= attempt % 3; d++)
                msg += ".";
            drawStatusBox(rows, cols, msg, frame++);

            rd = getRunDetail(rr.interpretId, selectedQuestionSlug);
            if (rd.statusCode != 0)
                break;

            napms(800); // Sample runs judge faster than full submissions
            flushinp();
        }

        int wrapWidth = std::min(cols - 8, 70);

        if (!rd.compileError.empty())
        {
            verdict = "Compile Error";
            colorPair = PAIR_SUBMIT_FAIL;
            detailLines.push_back("Compiler Output:");
            for (auto &l : wrapText(rd.compileError, wrapWidth))
                detailLines.push_back("  " + l);
        }
        else if (!rd.runSuccess)
        {
            verdict = rd.statusMsg.empty() ? "Runtime Error" : rd.statusMsg;
            colorPair = PAIR_SUBMIT_FAIL;

            std::string errText = !rd.fullCompileError.empty() ? rd.fullCompileError : rd.compileError;
            if (!errText.empty())
            {
                detailLines.push_back("Error:");
                for (auto &l : wrapText(errText, wrapWidth))
                    detailLines.push_back("  " + l);
            }
        }
        else
        {
            bool allPassed = (rd.totalTestcases > 0 && rd.totalCorrect == rd.totalTestcases);
            bool haveScore = rd.totalTestcases > 0;

            verdict = haveScore
                          ? (allPassed ? "All Testcases Passed" : "Some Testcases Failed")
                          : "Run Finished";
            colorPair = !haveScore ? PAIR_LOGO_CMD : (allPassed ? PAIR_SUBMIT_OK : PAIR_MEDIUM);
            if (haveScore)
                verdict += "  (" + std::to_string(rd.totalCorrect) + "/" +
                           std::to_string(rd.totalTestcases) + ")";

            // No "expected output" comes back from this endpoint, so we can't
            // mark individual cases pass/fail -- just show each sample
            // testcase's input next to what your code produced for it.
            size_t caseCount = rd.codeAnswer.size();
            for (size_t i = 0; i < caseCount; i++)
            {
                if (!detailLines.empty())
                    detailLines.push_back("");
                detailLines.push_back("Testcase " + std::to_string(i + 1) + ":");

                if (i < lastRunTestcaseList.size())
                    for (auto &l : wrapText("Input:  " + lastRunTestcaseList[i], wrapWidth))
                        detailLines.push_back("  " + l);

                for (auto &l : wrapText("Output: " + rd.codeAnswer[i], wrapWidth))
                    detailLines.push_back("  " + l);

                if (i < rd.stdOutputList.size() && !rd.stdOutputList[i].empty())
                    for (auto &l : wrapText("Stdout: " + rd.stdOutputList[i], wrapWidth))
                        detailLines.push_back("  " + l);
            }

            if (caseCount == 0 && !rd.statusMsg.empty())
                detailLines.push_back(rd.statusMsg);
        }
    }
    catch (const std::exception &e)
    {
        endwin();
        std::cout << "Exception: " << e.what() << std::endl;
        std::cin.get();
        exit(0);
    }

    // --- Result screen (same scrollable box pattern as submitScreen) ---
    clear();
    refresh();
    getmaxyx(stdscr, rows, cols);

    int boxWidth = std::min(cols - 4, 78);
    int boxHeight = std::max(10, std::min(rows - 2, 22));
    int boxY = std::max(0, (rows - boxHeight) / 2);
    int boxX = std::max(0, (cols - boxWidth) / 2);

    WINDOW *resultWin = newwin(boxHeight, boxWidth, boxY, boxX);
    keypad(resultWin, TRUE);

    auto printClipped = [&](int y, int x, const std::string &text)
    {
        int maxLen = std::max(0, boxWidth - x - 1);
        std::string clipped = (int)text.size() > maxLen ? text.substr(0, maxLen) : text;
        mvwprintw(resultWin, y, x, "%s", clipped.c_str());
    };

    wattron(resultWin, COLOR_PAIR(colorPair) | A_BOLD);
    box(resultWin, 0, 0);
    std::string title = " Run Result ";
    mvwprintw(resultWin, 0, std::max(1, (boxWidth - (int)title.size()) / 2), "%s", title.c_str());
    printClipped(1, std::max(1, (boxWidth - (int)verdict.size()) / 2), verdict);
    wattroff(resultWin, COLOR_PAIR(colorPair) | A_BOLD);

    int detailAreaTop = 3;
    int detailAreaHeight = std::max(1, boxHeight - detailAreaTop - 2);
    int detailPadHeight = std::max((int)detailLines.size(), 1);

    WINDOW *detailPad = newpad(detailPadHeight, std::max(boxWidth - 4, 1));
    for (int i = 0; i < (int)detailLines.size(); i++)
        mvwprintw(detailPad, i, 0, "%s", detailLines[i].c_str());

    int detailScroll = 0;
    bool scrollable = (int)detailLines.size() > detailAreaHeight;
    std::string footerText = scrollable
                                  ? "Up/Down: Scroll  |  Any other key: Back to editor"
                                  : "Press any key to return to editor...";

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

// --- Profile Screen (MePage) --------------------------------------------
// leetcode.com/u/<user> profilinin sadeleştirilmiş bir terminal görünümü:
// zorluk bazlı çözülen/toplam soru sayıları, en çok kullanılan diller ve
// öne çıkan yetenek (skill tag) etiketleri. Web sitesindeki ısı haritası,
// rozetler, rank gibi öğeler burada yok -- API'den zaten aldığımız ve
// terminalde anlamlı gösterilebilecek kısımlara odaklanıldı.

// numAcceptedQuestions / userSessionBeatsPercentage gibi difficulty bazlı
// listelerden belirli bir "difficulty" değerine karşılık gelen sayıyı
// bulur; bulunamazsa 0 döner.
static int findCountByDifficulty(const std::vector<QuestionCount> &counts, const std::string &difficulty)
{
    for (auto &c : counts)
        if (c.difficulty == difficulty)
            return c.count;
    return 0;
}

static int findCountByDifficulty(const std::vector<AllQuestionsCount> &counts, const std::string &difficulty)
{
    for (auto &c : counts)
        if (c.difficulty == difficulty)
            return c.count;
    return 0;
}

// solved/total oranını "[####------]" biçiminde bir bara çevirir
// (percentileBar ile aynı fikir, ama yüzde yerine ham count/max oranı
// üzerinden -- dil/etiket barlarında elimizde "yüzde" değil "sayı" var).
static std::string ratioBar(int part, int whole, int width)
{
    if (width < 6)
        width = 6;
    int inner = width - 2;
    double frac = whole > 0 ? (double)part / (double)whole : 0.0;
    if (frac < 0)
        frac = 0;
    if (frac > 1)
        frac = 1;
    int filled = (int)(frac * inner + 0.5);
    filled = std::max(0, std::min(inner, filled));

    std::string bar = "[";
    bar += std::string(filled, '#');
    bar += std::string(inner - filled, '-');
    bar += "]";
    return bar;
}

Screen profileScreen()
{
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    drawStatusBox(rows, cols, "Loading profile...", 0);

    // getMePage tek bir userSlug bekliyor; kullanıcının kendi profilini
    // istiyoruz, o yüzden önce globalData'dan oturum sahibinin username'ini
    // öğreniyoruz.
    globalData me = getglobalData();
    MePage page = getMePage(me.username);

    int totalSolved = findCountByDifficulty(page.questionProgress.numAcceptedQuestions, "All");
    int totalCount = findCountByDifficulty(page.submitStats.allQuestionsCount, "All");

    struct DiffRow
    {
        std::string label;
        int pair;
    };
    std::vector<DiffRow> diffs = {
        {"Easy", PAIR_EASY}, {"Medium", PAIR_MEDIUM}, {"Hard", PAIR_HARD}};

    // Diller: en çok çözülenden aza sırala.
    std::vector<LanguageProblem> languages = page.languageProblemData.languageProblemCount;
    std::sort(languages.begin(), languages.end(), [](const LanguageProblem &a, const LanguageProblem &b)
              { return a.problemsSolved > b.problemsSolved; });

    // Skill etiketleri: her kategoride en çok çözülen ilk 5 tanesi (liste
    // web sitesindekiyle aynı şekilde onlarca etiket içerebiliyor, hepsini
    // basmak dar bir terminalde okunmaz bir yığın oluşturuyor).
    auto topTags = [](std::vector<TagProblem> tags, size_t limit)
    {
        std::sort(tags.begin(), tags.end(), [](const TagProblem &a, const TagProblem &b)
                  { return a.problemsSolved > b.problemsSolved; });
        if (tags.size() > limit)
            tags.resize(limit);
        return tags;
    };
    std::vector<TagProblem> advancedTop = topTags(page.tagProblemCounts.advanced, 5);
    std::vector<TagProblem> intermediateTop = topTags(page.tagProblemCounts.intermediate, 5);
    std::vector<TagProblem> fundamentalTop = topTags(page.tagProblemCounts.fundamental, 5);

    // Bir zorluk/dil/etiket satırını -- opsiyonel renkle -- basıp satır
    // sayacını ilerleten yardımcı, aşağıdaki tekrar eden bloklar için.
    // Artık stdscr yerine profil kartının WINDOW*'una basıyor. Tek satıra
    // sığmayan etiket listesini "..." ile kesmek yerine gerektiği kadar
    // satıra sarıyor, böylece hiçbir skill kaybolmuyor.
    auto printTagGroup = [&](WINDOW *win, int &row, int startX, int contentWidth, int maxRow,
                              const std::string &label, const std::vector<TagProblem> &tags)
    {
        if (row > maxRow)
            return;

        wattron(win, A_BOLD);
        mvwprintw(win, row, startX, "%s", label.c_str());
        wattroff(win, A_BOLD);
        row++;

        if (tags.empty())
        {
            if (row > maxRow)
                return;
            wattron(win, A_DIM);
            mvwprintw(win, row, startX, "  (none)");
            wattroff(win, A_DIM);
            row++;
            return;
        }

        int wrapWidth = std::max(10, contentWidth - 2); // "  " girinti payı
        std::string line;
        for (size_t i = 0; i < tags.size() && row <= maxRow; i++)
        {
            std::string piece = tags[i].tagName + " (" + std::to_string(tags[i].problemsSolved) + ")";
            if (i + 1 < tags.size())
                piece += ", ";

            if (!line.empty() && (int)(line.size() + piece.size()) > wrapWidth)
            {
                mvwprintw(win, row, startX, "  %s", line.c_str());
                row++;
                line.clear();
                if (row > maxRow)
                    break;
            }
            line += piece;
        }
        if (!line.empty() && row <= maxRow)
        {
            mvwprintw(win, row, startX, "  %s", line.c_str());
            row++;
        }
    };

    // Bölüm başlıkları arasına ince bir ayraç çizgisi çeker (kart görünümünü
    // düz bir metin dökümü olmaktan çıkarıp bölümleri görsel olarak ayırır).
    // Kutunun kendisi gibi turuncu değil, diğer tüm içerikle aynı nötr renkte.
    auto drawSeparator = [&](WINDOW *win, int &row, int boxWidth)
    {
        mvwhline(win, row, 1, ACS_HLINE, std::max(0, boxWidth - 2));
        row++;
    };

    static const int MIN_ROWS = 10;
    static const int MIN_COLS = 34;

    while (1)
    {
        getmaxyx(stdscr, rows, cols);
        clear();

        if (rows < MIN_ROWS || cols < MIN_COLS)
        {
            std::string msg = "Terminal too small for profile view";
            mvprintw(std::max(0, rows / 2), std::max(0, (cols - (int)msg.size()) / 2), "%s", msg.c_str());
            drawHintBar(rows - 1, cols, "Q/Esc: Back to list");
            refresh();

            int c = getch();
            if (c == 'q' || c == 'Q' || c == 27)
                return SCREEN_MAIN;
            continue;
        }

        // Skill etiketleri artık sarmalandığı için kutu biraz daha geniş --
        // aynı satır sayısı içinde daha az sarmalama, daha az kesilen içerik.
        int contentWidth = std::min(cols - 8, 92);
        int boxWidth = contentWidth + 6;
        int boxHeight = rows - 1;
        int maxRow = boxHeight - 2;
        int boxX = std::max(0, (cols - boxWidth) / 2);
        int startX = 3; // relative to the window, left padding inside the border

        WINDOW *win = newwin(boxHeight, boxWidth, 0, boxX);

        // Tek renkli tema: sadece en dıştaki kutu turuncu, geri kalan her
        // şey terminalin varsayılan siyah/beyazında.
        wattron(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(PAIR_CONFIG_BORDER) | A_BOLD);

        std::string title = " " + (me.username.empty() ? "Profile" : me.username) + " ";
        wattron(win, A_BOLD);
        mvwprintw(win, 0, std::max(1, (boxWidth - (int)title.size()) / 2), "%s", title.c_str());
        wattroff(win, A_BOLD);

        int row = 2;

        mvwprintw(win, row, startX, "Following: %d    Followers: %d",
                  page.followData.following, page.followData.followers);
        row++;
        drawSeparator(win, row, boxWidth);
        row++;

        wattron(win, A_BOLD);
        mvwprintw(win, row, startX, "Solved  %d / %d", totalSolved, totalCount);
        wattroff(win, A_BOLD);
        row++;

        for (auto &d : diffs)
        {
            int solved = findCountByDifficulty(page.questionProgress.numAcceptedQuestions, d.label);
            int total = findCountByDifficulty(page.submitStats.allQuestionsCount, d.label);
            std::string bar = ratioBar(solved, total, 30);

            wattron(win, A_BOLD);
            mvwprintw(win, row, startX, "%-7s", d.label.c_str());
            wprintw(win, "%s", bar.c_str());
            wattroff(win, A_BOLD);

            wprintw(win, " %d/%d", solved, total);
            row++;
        }
        row++;

        if (row < maxRow)
        {
            drawSeparator(win, row, boxWidth);
            row++;

            mvwprintw(win, row, startX, "Languages");
            row++;

            if (languages.empty())
            {
                wattron(win, A_DIM);
                mvwprintw(win, row, startX, "(no submissions yet)");
                wattroff(win, A_DIM);
                row++;
            }
            else
            {
                int maxSolved = languages.front().problemsSolved;
                int shown = 0;
                for (auto &lang : languages)
                {
                    if (shown >= 6 || row >= maxRow)
                        break;
                    std::string bar = ratioBar(lang.problemsSolved, maxSolved, 20);
                    mvwprintw(win, row, startX, "%-10s %s %d", lang.languageName.c_str(), bar.c_str(), lang.problemsSolved);
                    row++;
                    shown++;
                }
            }
            row++;
        }

        if (row < maxRow)
        {
            drawSeparator(win, row, boxWidth);
            row++;

            wattron(win, A_BOLD);
            mvwprintw(win, row, startX, "Top Skills");
            wattroff(win, A_BOLD);
            row++;
            printTagGroup(win, row, startX, contentWidth, maxRow, "Advanced", advancedTop);
            printTagGroup(win, row, startX, contentWidth, maxRow, "Intermediate", intermediateTop);
            printTagGroup(win, row, startX, contentWidth, maxRow, "Fundamental", fundamentalTop);
        }

        drawHintBar(rows - 1, cols, "Q/Esc: Back to list");
        wnoutrefresh(stdscr);
        wnoutrefresh(win);
        doupdate();

        delwin(win);

        int c = getch();
        if (c == 'q' || c == 'Q' || c == 27)
            return SCREEN_MAIN;
        // KEY_RESIZE (ve tanımadığımız başka her tuş) döngü başına döner,
        // bir sonraki turda ekran yeni boyuta göre yeniden çizilir.
    }
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

        case SCREEN_RUN:
            current = runScreen();
            break;

        case SCREEN_PROFILE:
            current = profileScreen();
            break;

        default:
            current = SCREEN_EXIT;
            break;
        }
    }
    endwin();
    return 0;
}