#include <iostream>
#include <conio.h>
#include <windows.h>

//поле
const int WIDTH = 10;
const int HEIGHT = 20;

int FALL_SPEED = 35;//меняется в зависимости от уровня

//символы
const char INSTALLED_CELL = '#';
const char FALLING_FIGURE_CELL = '@';
const char SHADOW_CELL = '~';

//цвета
enum Colors {
    BLACK = 0, DARKYELLOW = 6, WHITE = 7, BLUE = 9, GREEN = 10, CYAN = 11, RED = 12, MAGENTA = 13, YELLOW = 14
};

const Colors TETRAMINO_COLORS[7] = { CYAN, YELLOW, MAGENTA, GREEN, RED, BLUE, DARKYELLOW };
const HANDLE HCONSOLE = GetStdHandle(STD_OUTPUT_HANDLE);

// Массив фигур (7 типов фигур, каждая с 4 поворотами, каждая фигура состоит из 4 блоков с координатами [x, y] относительно центра)
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

//ячейка - двусвязный список
struct Cell {
    char value = ' ';
    int color = WHITE;
    Cell* prev = nullptr;
    Cell* next = nullptr;
};

//строка - двусвязный список 
struct Row {
    Cell* head = nullptr;
    Cell* tail = nullptr;
    Row* prev = nullptr;
    Row* next = nullptr;
    int filled_count = 0; //сколько НЕ пустых ячеек, для удобной проверки на заполненность

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
    int row_count = 0;          //количество строк
    bool needDraw = false;      //флаг необходимости перерисовки
    int score = 0;              //очки

    //фигура тетрамино - во время игры существует 2 штуки: текущая и следующая
    struct Tetramino {
        int color = WHITE;
        int type = -1;     //0..6 — индекс в TETRAMINO_SHAPES
        int rotation = 0;  //0..3 — текущее вращение
        int x = WIDTH / 2; //центр по горизонтали
        int y = 0;         //позиция по вертикали (0 = сверху)
    } current, next;

    //инициализация игры
    void init() {
        clearField();       //очистка поля
        generateNext();     //генерация следующей фигуры
        spawnTetramino();   //она становится текущей
        generateNext();     //генерация новой следующей
    }

    //очистка поля
    void clearField() {
        Row* cur = top_row;

        //удаление всех строк
        while (cur) {
            Row* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
        top_row = bottom_row = nullptr;
        row_count = 0;
        score = 0;

        //добавление HEIGHT пустых строк
        for (int i = 0; i < HEIGHT; ++i) {
            addEmptyRowAtTop();
        }
    }

    //создать пустую строку
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

    //добавить пустую строку вверх
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

    //генерация следующей тетрамино
    void generateNext() {
        next.type = rand() % 7;
        next.color = TETRAMINO_COLORS[next.type];
    }

    //спавн падающей тетрамино
    void spawnTetramino() {
        current.type = next.type;
        current.color = next.color;
        current.rotation = 0;
        current.x = WIDTH / 2;
        current.y = 0;
    }

    //получить строку по игрику (0 = верх)
    Row* getRowAt(int y) {
        if (y < 0 || y >= row_count) return nullptr;
        Row* r = top_row;
        for (int i = 0; i < y; ++i) {
            if (!r) return nullptr;
            r = r->next;
        }
        return r;
    }

    //получить ячейку по координатам
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

    //проверка на возможность фигуры находиться на координатах
    bool isValidPosition(int dx = 0, int dy = 0) {
        //смотрим каждый из 4 блоков фигуры
        for (int i = 0; i < 4; ++i) {
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0] + dx;
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1] + dy;

            //проверка границ поля
            if (nx < 0 || nx >= WIDTH) return false;
            if (ny < 0) continue;   //выше поля игнорируем

            Row* row = getRowAt(ny);
            if (!row) return false;     //выход за границы

            //проверка столкновения с упавшими фигурами
            Cell* cell = getCellAt(ny, nx);
            if (!cell || cell->value != ' ') return false;  //клетка занята
        }
        return true;
    }

    //вращение фигуры
    void rotate() {
        int oldRotation = current.rotation;
        current.rotation = (current.rotation + 1) % 4;
        if (isValidPosition())
            needDraw = true;
        else
            current.rotation = oldRotation;
    }

    //движение фигуры
    void moveLeft() { if (isValidPosition(-1, 0)) { current.x--; needDraw = true; } }
    void moveRight() { if (isValidPosition(1, 0)) { current.x++; needDraw = true; } }
    void dropSoft() { if (isValidPosition(0, 1)) { current.y++; needDraw = true; } }

    void dropHard() {
        while (isValidPosition(0, 1)) {
            current.y++;
            needDraw = true;
        }
    }

    //получить тень фигуры
    int getShadowY() {
        int shadowY = current.y;

        //смещение самого нижнего блока
        int maxOffset = -10;
        for (int k = 0; k < 4; ++k) {
            int offset = TETRAMINO_SHAPES[current.type][current.rotation][k][1];
            if (offset > maxOffset) maxOffset = offset;
        }

        //пока можем опуститься так, чтобы нижний блок не врезался
        while (true) {
            bool canGoDown = true;

            for (int k = 0; k < 4; ++k) {
                int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                //клетка, где будет блок после спуска
                int ny = shadowY + TETRAMINO_SHAPES[current.type][current.rotation][k][1] - maxOffset + 1;

                //уперлись в самый низ
                if (ny >= row_count) {
                    canGoDown = false;
                    break;
                }
                if (ny < 0) continue;

                Cell* cell = getCellAt(ny, nx);
                //уперлись в фигуру на поле
                if (cell && cell->value != ' ') {
                    canGoDown = false;
                    break;
                }
            }

            if (!canGoDown) break;
            shadowY++;
        }

        return shadowY;
    }

    //закрепление фигуры на поле
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

    //удаление заполненных строк
    void clearFullRows() {
        int cleared = 0;
        Row* r = bottom_row;    //идем снизу вверх
        while (r) {
            Row* prev = r->prev;
            //если строка заполнена - удаляем и добавляем новую
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

        //начисление очков в зависимости от количества сбитых строк
        if (cleared > 0) {
            switch (cleared) {
            case 1: score += 100; break;
            case 2: score += 250; break;
            case 3: score += 500; break;
            case 4: score += 1000; break;
            }
        }
    }

    //окончание игры
    bool isGameOver() { 
        return !isValidPosition(0, 0); 
    }

    //отрисовка
    void draw() {
        system("cls");
        std::cout << "+----------+ Следующая:\n";

        Row* row = top_row;
        int disp = 0;   //текущая отображаемая строка

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

                //текущая фигура
                for (int k = 0; k < 4; ++k) {
                    int fx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                    int fy = current.y + TETRAMINO_SHAPES[current.type][current.rotation][k][1];
                    if (fy == disp && fx == j && fy >= 0) {
                        ch = FALLING_FIGURE_CELL;
                        col = current.color;
                        break;
                    }
                }

                //тень
                static int shadowY = -1;
                if (disp == 0) shadowY = getShadowY();

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

            //пок следующей фигуры (справа от игрового поля, требуется только первые 3 строки)
            if (disp < 3) {
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

        int counter = 0;    //счетчик скорости падения
        bool game_over = false;

        while (!game_over) {
            //управление с учётом русской раскладки
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
                //если фигура может упасть - падает
                if (game.isValidPosition(0, 1)) {
                    game.dropSoft();
                }
                else {
                    //фигура упала
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