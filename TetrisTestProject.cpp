#include <iostream>
#include <conio.h>
#include <windows.h>
#include <ctime>
#include <cstdlib>

const int WIDTH = 10; // Ширина игрового поля
const int INITIAL_VISIBLE_HEIGHT = 20; // Высота игрового поля

// Разделение движения и отоброжения
// То есть фигура падает каждые полсекунды вниз, но чтобы мы могли видеть движения влево и вправо поле обновляется 5 раз в полсекунды
const int GAME_TICK_MS = 500; // Влияет на скорость игры, скорость падения фигур
const int DISPLAY_REFRESH_MS = 100; // Частота обновления консоли (что-то типо фпс)

const char INSTALLED_CELL = '#'; // Символ для клеток установленных на поле
const char FALLING_FIGURE_CELL = '@'; // Символ для клеток падающей фигуры

// Массив фигур (7 типов фигур, каждая с 4 поворотами, каждая фигура состоит из 4 блоков с координатами [x, y] относительно центра)
const int TETRAMINO_SHAPES[7][4][4][2] = {
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
    Cell* prev = nullptr;
    Cell* next = nullptr;
};

struct Row {
    Cell* head = nullptr;
    Cell* tail = nullptr;
    Row* prev = nullptr;
    Row* next = nullptr;
    int filled_count = 0;     // сколько НЕ пустых ячеек, для удобной проверки на заполненность
    int width = 0;

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
    Row* top_row = nullptr;   // самая верхняя строка (y ≈ 0)
    Row* bottom_row = nullptr;   // самая нижняя
    int row_count = 0;

    // Текущая падающая тетрамино
    struct Tetramino {
        int type = -1;           // 0..6 — индекс в TETRAMINO_SHAPES
        int rotation = 0;        // 0..3 — текущее вращение
        int x = WIDTH / 2 - 2;   // центр по горизонтали
        int y = 0;               // позиция по вертикали (0 = сверху)
    } current;

    void init() {
        clearField();
        spawnTetramino();
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

        for (int i = 0; i < INITIAL_VISIBLE_HEIGHT; ++i) {
            addEmptyRowAtTop();
        }
    }

    Row* createEmptyRow() {
        Row* row = new Row();
        row->width = WIDTH;
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

    void spawnTetramino() {
        current.type = rand() % 7;
        current.rotation = 0;
        current.x = WIDTH / 2 - 2;
        current.y = 0;
    }

    Row* getRowAt(int y) const {
        if (y < 0 || y >= row_count) return nullptr;
        Row* r = top_row;
        for (int i = 0; i < y; ++i) {
            if (!r) return nullptr;
            r = r->next;
        }
        return r;
    }

    Cell* getCellAt(int y, int x) const {
        Row* row = getRowAt(y);
        if (!row || x < 0 || x >= WIDTH) return nullptr;
        Cell* c = row->head;
        for (int j = 0; j < x; ++j) {
            if (!c) return nullptr;
            c = c->next;
        }
        return c;
    }

    bool isValidPosition(int dx = 0, int dy = 0) const {
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
        if (!isValidPosition()) {
            current.rotation = oldRotation;
        }
    }

    void moveLeft() { if (isValidPosition(-1, 0)) current.x--; }
    void moveRight() { if (isValidPosition(1, 0)) current.x++; }
    void dropSoft() { if (isValidPosition(0, 1)) current.y++; }

    void dropHard() {
        while (isValidPosition(0, 1)) {
            current.y++;
        }
    }

    void mergeToField() {
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0];
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1];
            if (ny < 0) continue;

            Cell* cell = getCellAt(ny, nx);
            if (cell) {
                cell->value = INSTALLED_CELL;
                Row* r = getRowAt(ny);
                if (r) r->filled_count++;
            }
        }
        clearFullRows();
    }

    void clearFullRows() {
        Row* r = bottom_row;
        while (r) {
            Row* prev = r->prev;
            if (r->filled_count == WIDTH) {
                // Удаляем строку из двусвязного списка
                if (r->prev) r->prev->next = r->next;
                if (r->next) r->next->prev = r->prev;
                if (r == top_row)    top_row = r->next;
                if (r == bottom_row) bottom_row = r->prev;

                delete r;
                row_count--;
                addEmptyRowAtTop();
            }
            r = prev;
        }
    }

    bool isGameOver() const {
        return !isValidPosition(0, 0);
    }

    void draw() {
        system("cls");
        std::cout << "+----------+\n";

        Row* row = top_row;
        int displayed = 0;

        while (row && displayed < INITIAL_VISIBLE_HEIGHT) {
            std::cout << "|";
            Cell* cell = row->head;
            for (int j = 0; j < WIDTH; ++j) {
                char ch = ' ';
                if (cell) {
                    ch = cell->value;
                    cell = cell->next;
                }

                // Отрисовка падающей тетрамино
                for (int k = 0; k < 4; ++k) {
                    int fx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                    int fy = current.y + TETRAMINO_SHAPES[current.type][current.rotation][k][1];
                    if (fy == displayed && fx == j && fy >= 0) {
                        ch = FALLING_FIGURE_CELL;
                        break;
                    }
                }
                std::cout << ch;
            }
            std::cout << "|\n";
            row = row->next;
            displayed++;
        }

        while (displayed < INITIAL_VISIBLE_HEIGHT) {
            std::cout << "|          |\n";
            displayed++;
        }

        std::cout << "+----------+\n";
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
    Tetris game;
    game.init();

    long lastTick = 0;
    long lastDisplay = 0;

    while (true) {
        long now = GetTickCount();

        // Обработка ввода
        while (_kbhit()) {
            char key = static_cast<char>(_getch());
            switch (key) {
            case 'a': case 'A': game.moveLeft();  break;
            case 'd': case 'D': game.moveRight(); break;
            case 'w': case 'W': game.rotate();    break;
            case 's': case 'S':
            case ' ':           game.dropHard();  break;
            }
        }

        // Автоматическое падение
        if (now - lastTick >= GAME_TICK_MS) {
            lastTick = now;
            if (game.isValidPosition(0, 1)) {
                game.dropSoft();
            }
            else {
                game.mergeToField();
                game.spawnTetramino();
                if (game.isGameOver()) {
                    break;
                }
            }
        }

        // Обновление экрана
        if (now - lastDisplay >= DISPLAY_REFRESH_MS) {
            lastDisplay = now;
            game.draw();
        }

        Sleep(10);
    }

    system("cls");
    std::cout << "ИГРА ОКОНЧЕНА\n";
    system("pause");
    return 0;
}