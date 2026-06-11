#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <ctime>

using namespace sf;

const int MAX_C = 32, MAX_R = 32;
int grid[MAX_C][MAX_R], sgrid[MAX_C][MAX_R];
int cols = 9, rows = 9, totalMines = 10, moves = 0;
bool firstClick = true, gameActive = false;

enum GameState { PLAYING, GAME_OVER, GAME_WON } state = PLAYING;

// Генерация поля: мины не ставятся рядом с первым кликом
void generateField(int fx, int fy) {
    int total = cols * rows, placed = 0;
    for (int i = 0; i < total; ++i) {
        grid[i % cols + 1][i / cols + 1] = 0;
        sgrid[i % cols + 1][i / cols + 1] = 10;
    }
    while (placed < totalMines) {
        int px = rand() % cols + 1, py = rand() % rows + 1;
        if (grid[px][py] == 9 || (abs(px-fx) <= 1 && abs(py-fy) <= 1)) continue;
        grid[px][py] = 9; placed++;
    }
    for (int i = 0; i < total; ++i) {
        int x = i % cols + 1, y = i / cols + 1;
        if (grid[x][y] == 9) continue;
        for (int d = 0; d < 9; ++d) {
            int nx = x + (d % 3 - 1), ny = y + (d / 3 - 1);
            if (nx > 0 && nx <= cols && ny > 0 && ny <= rows && grid[nx][ny] == 9) grid[x][y]++;
        }
    }
}

void resetGame() {
    firstClick = true; gameActive = false; moves = 0; state = PLAYING;
    for (int i = 0; i < cols * rows; ++i) {
        grid[i % cols + 1][i / cols + 1] = 0;
        sgrid[i % cols + 1][i / cols + 1] = 10;
    }
}

// Рекурсивное открытие пустых областей (flood fill)
void openCells(int x, int y) {
    if (sgrid[x][y] == 10) {
        sgrid[x][y] = grid[x][y];
        if (grid[x][y] == 0) {
            for (int d = 0; d < 9; ++d) {
                int nx = x + (d % 3 - 1), ny = y + (d / 3 - 1);
                if (nx > 0 && nx <= cols && ny > 0 && ny <= rows && d != 4) openCells(nx, ny);
            }
        }
    }
}

// Победа: все клетки без мин открыты
bool checkWin() {
    int opened = 0;
    for (int i = 0; i < cols * rows; ++i)
        opened += (sgrid[i % cols + 1][i / cols + 1] < 9);
    return opened == (cols * rows - totalMines);
}

int main() {
    srand(time(0));
    resetGame(); gameActive = true;

    RenderWindow app(VideoMode({(unsigned)((cols+2)*32), (unsigned)((rows+2)*32)}), "Minesweeper");
    app.setFramerateLimit(60);

    while (app.isOpen()) {
        while (const auto e = app.pollEvent()) {
            if (e->is<Event::Closed>()) app.close();

            if (const auto* mb = e->getIf<Event::MouseButtonPressed>()) {
                int x = mb->position.x / 32, y = mb->position.y / 32;
                if (x > 0 && x <= cols && y > 0 && y <= rows && state == PLAYING) {
                    if (mb->button == Mouse::Button::Left && sgrid[x][y] == 10) {
                        if (firstClick) { generateField(x, y); firstClick = false; }
                        if (grid[x][y] == 9) {
                            state = GAME_OVER;
                            for (int i = 0; i < cols*rows; ++i)
                                if (grid[i%cols+1][i/cols+1]==9) sgrid[i%cols+1][i/cols+1]=9;
                        } else {
                            moves++;
                            grid[x][y] == 0 ? openCells(x, y) : (void)(sgrid[x][y] = grid[x][y]);
                            if (checkWin()) state = GAME_WON;
                        }
                    } else if (mb->button == Mouse::Button::Right && sgrid[x][y] >= 10) {
                        sgrid[x][y] = (sgrid[x][y] == 10) ? 11 : 10;
                    }
                }
            }
        }
        // Пока просто заливка — UI добавим в следующей ветке
        app.clear(Color(192, 192, 192));
        app.display();
    }
    return 0;
}
