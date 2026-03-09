#include <iostream>  
#include <conio.h>   
#include <windows.h> 

const int WIDTH = 10; // Ширина игрового поля
const int HEIGHT = 20; // Высота игрового поля
const int FALL_SPEED = 35; // Скорость автоматического падения ~350мс

const char INSTALLED_CELL = '#'; // Символ для клеток установленных на поле
const char FALLING_FIGURE_CELL = '@'; // Символ для клеток падающей фигуры

// Это массив всех тетрамино (7 типов: I, O, T, S, Z, J, L).
// Каждый тип имеет 4 возможных поворота.
// Каждая фигура состоит из 4 блоков, каждый блок - координаты [x, y] относительно центра фигуры.
// Координаты могут быть отрицательными, чтобы центр был в (0,0) для удобства поворотов.
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

// Структура Cell представляет одну ячейку поля.
// value - символ в ячейке (' ' - пусто, '#' - установлен блок).
struct Cell {
    char value = ' ';
    Cell* prev = nullptr; // Указатель на предыдущую ячейку в строке
    Cell* next = nullptr; // Указатель на следующую ячейку в строке
};

// Структура Row представляет одну строку поля.
// head и tail - начало и конец списка ячеек (для быстрого доступа).
// prev и next - указатели для двусвязного списка строк (поле - список строк).
// filled_count - счётчик непустых ячеек (для проверки, полна ли строка WIDTH ячейками '#').
// Деструктор ~Row() освобождает память ячеек при удалении строки.
struct Row {
    Cell* head = nullptr; // Первая ячейка строки
    Cell* tail = nullptr; // Последняя ячейка строки
    Row* prev = nullptr; // Предыдущая строка
    Row* next = nullptr; // Следующая строка
    int filled_count = 0; // сколько НЕ пустых ячеек, для удобной проверки на заполненность

    ~Row() { // Деструктор
        Cell* cur = head; // Начинаем с головы
        while (cur) { // Пока есть ячейки
            Cell* nxt = cur->next; // Сохраняем следующую
            delete cur; // Удаляем текущую
            cur = nxt; // Переходим к следующей
        }
    }
};

// top_row и bottom_row - верхняя и нижняя строки поля (двусвязный список).
// row_count - текущее количество строк (должно быть HEIGHT).
// needDraw - флаг, что нужно перерисовать поле (оптимизация: рисуем только при изменениях).
// Вложенная структура Tetramino - текущая падающая фигура.
// type - 0-6 (индекс в TETRAMINO_SHAPES).
// rotation - 0-3 (текущий поворот).
// x, y - позиция центра фигуры (x - горизонталь, y - вертикаль, 0 сверху).
struct Tetris {
    Row* top_row = nullptr; // самая верхняя строка (y = 0)
    Row* bottom_row = nullptr; // самая нижняя
    int row_count = 0; // Количество строк в поле
    bool needDraw = false; // Флаг: нужно ли перерисовывать поле

    // Объект падающей тетраминошки
    struct Tetramino {
        int type; // 0..6 — индекс в TETRAMINO_SHAPES
        int rotation = 0; // 0..3 — текущее вращение
        int x = WIDTH / 2; // центр по горизонтали
        int y = 0; // позиция по вертикали (0 = сверху)
    } current; // Текущая фигура

    // Инициализация игры
    void init() {
        clearField(); // Очищаем поле
        spawnTetramino(); // Создаём новую фигуру
    }

    // Очистка поля: удаляем все строки, обнуляем указатели и счётчики.
    // Затем добавляем HEIGHT пустых строк сверху.
    void clearField() {
        Row* cur = top_row; // Начинаем с верха
        while (cur) { // Пока есть строки
            Row* nxt = cur->next; // Сохраняем следующую
            delete cur; // Удаляем текущую (деструктор Row освободит ячейки)
            cur = nxt; // Переходим
        }
        top_row = bottom_row = nullptr; // Обнуляем указатели
        row_count = 0; // Обнуляем счётчик
        for (int i = 0; i < HEIGHT; ++i) { // Добавляем HEIGHT пустых строк
            addEmptyRowAtTop(); // Добавляем сверху
        }
    }

    Row* createEmptyRow() {
        Row* row = new Row(); // Новая строка
        Cell* prev_cell = nullptr; // Предыдущая ячейка для связывания
        for (int j = 0; j < WIDTH; ++j) { // Для каждой позиции
            Cell* cell = new Cell(); // Новая ячейка (' ')
            if (prev_cell) { // Если есть предыдущая
                prev_cell->next = cell; // Связываем
                cell->prev = prev_cell;
            }
            else { // Первая ячейка
                row->head = cell;
            }
            prev_cell = cell; // Обновляем предыдущую
        }
        row->tail = prev_cell; // Последняя - tail
        return row;
    }

    // Добавление пустой строки сверху: создаём строку, связываем с текущим top_row
    // Если поле пустое, top_row и bottom_row = новой строке
    void addEmptyRowAtTop() {
        Row* row = createEmptyRow(); // Создаём пустую
        if (!top_row) { // Если поле пустое
            top_row = bottom_row = row;
        }
        else { // Иначе вставляем сверху
            top_row->prev = row; // Текущий top теперь ниже
            row->next = top_row; // Новая связана с текущим top
            top_row = row; // Обновляем top
        }
        row_count++; // Увеличиваем счётчик
    }

    // Спавн новой фигуры: случайный тип 0-6, rotation=0, x=WIDTH/2
    // y=0 (сверху)
    void spawnTetramino() {
        current.type = rand() % 7; // Случайный тип
        current.rotation = 0; // Без поворота
        current.x = WIDTH / 2; // Центр
        current.y = 0; // Сверху
    }

    // Получение строки по y (0 - сверху).
    // Проходим от top_row вниз y раз.
    Row* getRowAt(int y) {
        if (y < 0 || y >= row_count) return nullptr; // За пределами - nullptr
        Row* r = top_row; // Начинаем сверху
        for (int i = 0; i < y; ++i) { // Спускаемся y раз
            if (!r) return nullptr; // Если конец - ошибка
            r = r->next;
        }
        return r;
    }

    // Получение ячейки по (y, x): сначала строку, затем от head x раз вправо.
    Cell* getCellAt(int y, int x) {
        Row* row = getRowAt(y); // Получаем строку
        if (!row || x < 0 || x >= WIDTH) return nullptr; // За пределами - nullptr
        Cell* c = row->head; // Начинаем слева
        for (int j = 0; j < x; ++j) { // Идём x раз вправо
            if (!c) return nullptr;
            c = c->next;
        }
        return c;
    }

    // Проверка, валидна ли позиция фигуры с смещением (dx, dy).
    // Для каждого из 4 блоков: вычисляем nx, ny = текущая + смещение + относительная координата блока.
    // Проверяем: nx в [0, WIDTH), ny >=0 и < row_count, ячейка пуста (' ').
    // Если ny <0 - ок (фигура может торчать сверху при спавне).
    bool isValidPosition(int dx = 0, int dy = 0) {
        for (int i = 0; i < 4; ++i) { // Для каждого блока
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0] + dx; // Новая x
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1] + dy; // Новая y
            if (nx < 0 || nx >= WIDTH) return false; // За горизонтальными границами
            if (ny < 0) continue; // Торчит сверху - ок
            Row* row = getRowAt(ny); // Получаем строку
            if (!row) return false; // Нет строки
            Cell* cell = getCellAt(ny, nx); // Получаем ячейку
            if (!cell || cell->value != ' ') return false; // Нет ячейки или занята
        }
        return true; // Всё ок
    }

    // Поворот фигуры: увеличиваем rotation %4.
    // Проверяем валидность новой позиции; если нет - откатываем.
    // Если да - ставим needDraw = true (нужно перерисовать).
    void rotate() {
        int oldRotation = current.rotation; // Сохраняем старый
        current.rotation = (current.rotation + 1) % 4; // Новый
        if (isValidPosition()) { // Проверяем
            needDraw = true; // Ок - рисуем
        }
        else current.rotation = oldRotation; // Откат
        
    }

    // Движение влево: проверяем позицию с dx=-1, если ок - current.x--, needDraw=true.
    void moveLeft() {
        if (isValidPosition(-1, 0)) {
            current.x--;
            needDraw = true;
        }
    }

    // Аналогично вправо: dx=1.
    void moveRight() {
        if (isValidPosition(1, 0)) {
            current.x++;
            needDraw = true;
        }
    }

    // Мягкое падение: dy=1, если ок - current.y++, needDraw=true.
    void dropSoft() {
        if (isValidPosition(0, 1)) {
            current.y++;
            needDraw = true;
        }
    }

    // Жёсткое падение: пока возможно dy=1, спускаем вниз (до упора).
    void dropHard() {
        while (isValidPosition(0, 1)) {
            current.y++;
            needDraw = true;
        }
    }

    // Слияние фигуры с полем: для каждого блока вычисляем (nx, ny), если ny>=0 - ставим '#' в ячейку, увеличиваем filled_count строки.
    // Затем очищаем полные строки.
    void mergeToField() {
        for (int i = 0; i < 4; ++i) { // Для каждого блока
            int nx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][i][0];
            int ny = current.y + TETRAMINO_SHAPES[current.type][current.rotation][i][1];
            if (ny < 0) continue; // Торчит сверху - игнор
            Cell* cell = getCellAt(ny, nx); // Ячейка
            if (cell) {
                cell->value = INSTALLED_CELL; // '#'
                Row* r = getRowAt(ny); // Строка
                if (r) r->filled_count++; // Увеличиваем счётчик
            }
        }
    }

    // Очистка полных строк: идём от bottom_row вверх (prev).
    // Если filled_count == WIDTH - удаляем строку: перестраиваем указатели prev/next, обновляем top/bottom если нужно.
    // Уменьшаем row_count, добавляем пустую строку сверху.
    void clearFullRows() {
        Row* r = bottom_row; // Начинаем снизу
        while (r) {
            Row* prev = r->prev; // Сохраняем prev (идём вверх)
            if (r->filled_count == WIDTH) { // Полная?
                if (r->prev) r->prev->next = r->next; // Связываем prev с next
                if (r->next) r->next->prev = r->prev; // И next с prev
                if (r == top_row) top_row = r->next; // Если top - новый top = next
                if (r == bottom_row) bottom_row = r->prev; // Если bottom - новый bottom = prev
                delete r; // Удаляем (деструктор освободит ячейки)
                row_count--; // Уменьшаем
                addEmptyRowAtTop(); // Добавляем пустую сверху (сдвиг вниз)
            }
            r = prev; // Переходим вверх
        }
    }

    // Проверка конца игры: если новая фигура в позиции (0,0) не валидна - переполнение.
    bool isGameOver() {
        return !isValidPosition(0, 0);
    }

    // Отрисовка поля: очищаем экран (system("cls")).
    // Рисуем рамку сверху "+----------+".
    // Затем для каждой строки (до HEIGHT): "|", затем для каждой ячейки - её value или '@' если это блок падающей фигуры.
    // Проверяем для каждого (j, displayed) совпадение с блоками current.
    // Если строк меньше HEIGHT - добиваем пустыми "|          |".
    // Рамка снизу.
    void draw() {
        system("cls"); // Очистка экрана
        std::cout << "+----------+\n"; // Верх рамки
        Row* row = top_row; // Начинаем сверху
        int displayed = 0; // Счётчик отображаемых строк
        while (row && displayed < HEIGHT) { // Пока есть строки и не до HEIGHT
            std::cout << "|"; // Левая рамка
            Cell* cell = row->head; // Ячейки строки
            for (int j = 0; j < WIDTH; ++j) { // Для каждой позиции
                char ch; // По умолчанию пусто
                if (cell) { // Если ячейка есть
                    ch = cell->value; // Её значение
                    cell = cell->next; // Следующая
                }
                // Отрисовка падающей тетрамино: проверяем, совпадает ли (j, displayed) с каким-то блоком
                for (int k = 0; k < 4; ++k) { // Для каждого блока фигуры
                    int fx = current.x + TETRAMINO_SHAPES[current.type][current.rotation][k][0];
                    int fy = current.y + TETRAMINO_SHAPES[current.type][current.rotation][k][1];
                    if (fy == displayed && fx == j && fy >= 0) { // Совпадение и не сверху
                        ch = FALLING_FIGURE_CELL; // '@'
                        break;
                    }
                }
                std::cout << ch; // Выводим
            }
            std::cout << "|\n"; // Правая рамка и новая строка
            row = row->next; // Следующая строка
            displayed++;
        }
        while (displayed < HEIGHT) { // Если меньше HEIGHT - пустые строки
            std::cout << "|          |\n"; // Пустая
            displayed++;
        }
        std::cout << "+----------+\n"; // Низ рамки
    }

    // Деструктор Tetris: удаляем все строки (деструкторы Row освободят ячейки).
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

    int counterForDisplay = 0; 

    while (true) {
        while (_kbhit()) {
            switch (_getwch()) {
                case 'a': case 'A': case L'ф': case L'Ф': game.moveLeft();  break; // Влево
                case 'd': case 'D': case L'в': case L'В': game.moveRight(); break; // Вправо
                case 'w': case 'W': case L'ц': case L'Ц': game.rotate();    break; // Поворот
                case ' ':                                 game.dropHard();  break; // Жёсткое падение (пробел)
                case 's': case 'S': case L'ы': case L'Ы': game.dropSoft();  break; // Мягкое (вниз)
            }
        }

        // Автоматическое падение
        if (counterForDisplay == FALL_SPEED) {
            counterForDisplay = 0;

            game.draw();
            if (game.isValidPosition(0, 1)) {
                game.dropSoft();
            }
            else {
                game.mergeToField(); // Сливаем с полем
                game.clearFullRows(); // Отчищаем строки
                game.spawnTetramino(); // Новая фигура
                if (game.isGameOver()) { // Переполнение?
                    break;
                }
            }
        } else counterForDisplay++;

        if (game.needDraw) { // Если было движение - рисуем
            game.draw();
            game.needDraw = false;
        }
        Sleep(10); // Пауза 10ms
    }
    system("cls");
    std::cout << "ИГРА ОКОНЧЕНА\n";
    system("pause");
}