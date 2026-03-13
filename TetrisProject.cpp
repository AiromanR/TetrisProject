#include <iostream>
#include <conio.h>
#include <windows.h>

const int WIDTH = 10;
const int HEIGHT = 20;

int FALL_SPEED = 35;//меняется в зависимости от уровня

const char INSTALLED_CELL = '#';
const char FALLING_FIGURE_CELL = '@';
const char SHADOW_CELL = '~';

//цвета
enum Colors {
    BLACK = 0, DARKYELLOW = 6, WHITE = 7, BLUE = 9, GREEN = 10, CYAN = 11, RED = 12, MAGENTA = 13, YELLOW = 14
};

const Colors TETRAMINO_COLORS[7] = { CYAN, YELLOW, MAGENTA, GREEN, RED, BLUE, DARKYELLOW };
const HANDLE HCONSOLE = GetStdHandle(STD_OUTPUT_HANDLE);

int TETRAMINO_SHAPES[7][4][4][2] = {
    // I
    {{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}, {{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}},
    // O
    {{{-1,-1},{0,-1},{-1,0},{0,0}}, {{-1,-1},{0,-1},{-1,0},{0,0}}, {{-1,-1},{0,-1},{-1,0},{0,0}}, {{-1,-1},{0,-1},{-1,0},{0,0}}},
    // T
    {{{-1,0},{0,0},{1,0},{0,-1}}, {{0,-1},{0,0},{0,1},{1,0}}, {{-1,0},{0,0},{1,0},{0,1}}, {{0,-1},{0,0},{0,1},{-1,0}}},
    // S
    {{{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}}, {{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}}},
    // Z
    {{{-1,-1},{0,-1},{0,0},{1,0}}, {{0,-1},{0,0},{-1,0},{-1,1}}, {{-1,-1},{0,-1},{0,0},{1,0}}, {{0,-1},{0,0},{-1,0},{-1,1}}},
    // J
    {{{-1,0},{0,0},{1,0},{-1,-1}}, {{0,-1},{0,0},{0,1},{-1,1}}, {{-1,0},{0,0},{1,0},{1,1}}, {{0,-1},{0,0},{0,1},{1,-1}}},
    // L
    {{{-1,0},{0,0},{1,0},{1,-1}}, {{0,-1},{0,0},{0,1},{1,1}}, {{-1,0},{0,0},{1,0},{-1,1}}, {{0,-1},{0,0},{0,1},{-1,-1}}}
};

struct Cell {
    char value = ' ';
    int color = WHITE;
    Cell* prev = nullptr;
    Cell* next = nullptr;
};

struct Row {
    Cell* head = nullptr;
    Cell* tail = nullptr;
    Row* prev = nullptr;
    Row* next = nullptr;
    int filled_count = 0;

    ~Row() {
        Cell* cur = head;
        while (cur) {
            Cell* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }
};

struct Tetris {
    Row* top_row = nullptr;
    Row* bottom_row = nullptr;
    int row_count = 0;
    bool needDraw = false;
    int score = 0;

    struct Tetramino {
        int color = WHITE;
        int type = -1;
        int rotation = 0;
        int x = WIDTH / 2;
        int y = 0;
    } current, next;

    void init() {
        clearField();
        generateNext();
        spawnTetramino();
        generateNext();
    }

    void clearField() {
        Row* cur = top_row;
        while (cur) {
            Row* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        top_row = bottom_row = nullptr;
        row_count = 0;
        score = 0;

        for (int i = 0; i < HEIGHT; ++i) {
            addEmptyRowAtTop();
        }
    }

    Row* createEmptyRow() {
        Row* row = new Row();
        Cell* prev_cell = nullptr;
        for (int j = 0; j < WIDTH; ++j) {
            Cell* cell = new Cell();
            if (prev_cell) {
                prev_cell->next = cell;
                cell->prev = prev_cell;
            }
            else {
                row->head = cell;
            }
            prev_cell = cell;
        }
        row->tail = prev_cell;
        return row;
    }

    void addEmptyRowAtTop() {
        Row* row = createEmptyRow();
        if (!top_row) {
            top_row = bottom_row = row;
        }
        else {
            top_row->prev = row;
            row->next = top_row;
            top_row = row;
        }
        row_count++;
    }

    void generateNext() {
        next.type = rand() % 7;
        next.color = TETRAMINO_COLORS[next.type];
    }

    void spawnTetramino() {
        current.type = next.type;
        current.color = next.color;
        current.rotation = 0;
        current.x = WIDTH / 2;
        current.y = 0;
    }

    Row* getRowAt(int y) {
        if (y < 0 || y >= row_count) return nullptr;
        Row* r = top_row;
        for (int i = 0; i < y; ++i) {
            if (!r) return nullptr;
            r = r->next;
        }
        return r;
    }

    Cell* getCellAt(int y, int x) {
        Row* row = getRowAt(y);
        if (!row || x < 0 || x >= WIDTH) return nullptr;
        Cell* c = row->head;
        for (int j = 0; j < x; ++j) {
            if (!c) return nullptr;
            c = c->next;
        }
        return c;
    }

    bool isValidPosition(int dx = 0, int dy = 0) {
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0] + dx;
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1] + dy;
            
            if (nx < 0 || nx >= WIDTH) return false;
            if (ny < 0) continue;
            
            Row* row = getRowAt(ny);
            if (!row) return false;
            
            Cell* cell = getCellAt(ny, nx);
            if (!cell || cell->value != ' ') return false;
        }
        return true;
    }

    void rotate() {
        int oldRotation = current.rotation;
        current.rotation = (current.rotation + 1) % 4;
        if 
            (isValidPosition()) needDraw = true;
        else 
            current.rotation = oldRotation;
        
    }

    void moveLeft() { if (isValidPosition(-1, 0)) { current.x--; needDraw = true; } }
    void moveRight() { if (isValidPosition(1, 0)) { current.x++; needDraw = true; } }
    void dropSoft() { if (isValidPosition(0, 1)) { current.y++; needDraw = true; } }

    void dropHard() {
        while (isValidPosition(0, 1)) {
            current.y++;
            needDraw = true;
        }
    }

    int getShadowY() {
        int shadowY = current.y;
        while (true) {
            bool canGoDown = isValidPosition(0, shadowY - current.y + 1);
            if (!canGoDown) break;
            shadowY++;
        }
        return shadowY;
    }

    void mergeToField() {
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0];
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1];
            if (ny < 0) continue;

            Cell* cell = getCellAt(ny, nx);
            if (cell) {
                cell->value = INSTALLED_CELL;
                cell->color = current.color;
                Row* r = getRowAt(ny);
                if (r) r->filled_count++;
            }
        }
    }

    void clearFullRows() {
        int cleared = 0;
        Row* r = bottom_row;
        while (r) {
            Row* prev = r->prev;
            if (r->filled_count == WIDTH) {
                if (r->prev) r->prev->next = r->next;
                if (r->next) r->next->prev = r->prev;
                if (r == top_row) top_row = r->next;
                if (r == bottom_row) bottom_row = r->prev;

                delete r;
                row_count--;
                addEmptyRowAtTop();
                cleared++;
            }
            r = prev;
        }
        if (cleared > 0) {
            switch (cleared) {
            case 1: score += 100; break;
            case 2: score += 250; break;
            case 3: score += 500; break;
            case 4: score += 1000; break;
            }
        }
    }

    bool isGameOver() { return !isValidPosition(0, 0);}

    void draw() {
        system("cls");
        std::cout << "+----------+ Следующая:\n";

        Row* row = top_row;
        int disp = 0;

        while (row && disp < HEIGHT) {
            std::cout << "|";
            Cell* c = row->head;
            for (int j = 0; j < WIDTH; ++j) {
                char ch = ' ';
                int col = WHITE;

                if (c) {
                    ch = c->value;
                    col = c->color;
                    c = c->next;
                }

                // текущая фигура
                for (int k = 0; k < 4; ++k) {
                    int fx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                    int fy = current.y + TETRAMINO_SHAPES[current.type][current.rotation][k][1];
                    if (fy == disp && fx == j && fy >= 0) {
                        ch = FALLING_FIGURE_CELL;
                        col = current.color;
                        break;
                    }
                }

                // тень
                int shadowY = getShadowY();
                if (ch == ' ' && disp == shadowY && shadowY > current.y) {
                    for (int k = 0; k < 4; ++k) {
                        int fx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                        if (fx == j) {
                            ch = SHADOW_CELL;
                            col = current.color;
                            break;
                        }
                    }
                }

                SetConsoleTextAttribute(HCONSOLE, col);
                std::cout << ch;
            }
            SetConsoleTextAttribute(HCONSOLE, WHITE);
            std::cout << "|";

            // следующая фигура (справа, только первые 4 строки)
            if (disp < 4) {
                std::cout << "   ";
                for (int j = 0; j < 4; ++j) {
                    char ch = ' ';
                    int col = WHITE;
                    for (int k = 0; k < 4; ++k) {
                        int fx = 1 + TETRAMINO_SHAPES[next.type][0][k][0];
                        int fy = 2 + TETRAMINO_SHAPES[next.type][0][k][1];
                        if (fx == j && fy == disp) {
                            ch = FALLING_FIGURE_CELL;
                            col = next.color;
                            break;
                        }
                    }
                    SetConsoleTextAttribute(HCONSOLE, col);
                    std::cout << ch;
                    SetConsoleTextAttribute(HCONSOLE, WHITE);
                }
            }
            std::cout << "\n";
            row = row->next;
            disp++;
        }

        while (disp < HEIGHT) {
            std::cout << "|          |\n";
            disp++;
        }

        SetConsoleTextAttribute(HCONSOLE, WHITE);
        std::cout << "+----------+\n";
        std::cout << "Очки: " << score << "\n";
    }

    ~Tetris() {
        Row* cur = top_row;
        while (cur) {
            Row* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }
};

int main() {
    srand(time(NULL));
    setlocale(LC_ALL, "Russian");

    bool play_again = true;
    while (play_again) {
        std::cout << "Тетрис\n";
        std::cout << "1 = Новичок\n";
        std::cout << "2 = Средний\n";
        std::cout << "3 = Профи\n> ";

        int lvl;
        std::cin >> lvl;
        switch (lvl) {
            case 1: FALL_SPEED = 50; break;
            case 2: FALL_SPEED = 35; break;
            case 3: FALL_SPEED = 18; break;
            default: FALL_SPEED = 35;
        }

        Tetris game;
        game.init();

        int counter = 0;
        bool game_over = false;

        while (!game_over) {
            while (_kbhit()) {
                switch (_getwch()) {
                case 'a': case 'A': case L'ф': case L'Ф': game.moveLeft();  break;
                case 'd': case 'D': case L'в': case L'В': game.moveRight(); break;
                case 'w': case 'W': case L'ц': case L'Ц': game.rotate();    break;
                case 's': case 'S': case L'ы': case L'Ы': game.dropSoft();  break;
                case ' ':                                 game.dropHard();  break;
                }
            }

            counter++;
            if (counter >= FALL_SPEED) {
                counter = 0;
                if (game.isValidPosition(0, 1)) {
                    game.dropSoft();
                }
                else {
                    game.mergeToField();
                    game.clearFullRows();
                    game.spawnTetramino();
                    game.generateNext();
                    if (game.isGameOver()) {
                        game_over = true;
                    }
                }
                game.needDraw = true;
            }

            if (game.needDraw) {
                game.draw();
                game.needDraw = false;
            }

            Sleep(10);
        }

        system("cls");
        SetConsoleTextAttribute(HCONSOLE, RED);
        std::cout << "ИГРА ОКОНЧЕНА\n";
        SetConsoleTextAttribute(HCONSOLE, WHITE);
        std::cout << "Очки: " << game.score << "\n\n";
        std::cout << "Ещё раз? (д/н) ";

        char ans;
        std::cin >> ans;
        play_again = (ans == 'д' || ans == 'Д' || ans == 'y' || ans == 'Y');
    }

    system("pause");
}