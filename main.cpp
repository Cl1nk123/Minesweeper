#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace sf;

const int MAX_C = 32, MAX_R = 32;
int grid[MAX_C][MAX_R], sgrid[MAX_C][MAX_R];
int cols = 9, rows = 9, totalMines = 10, moves = 0;
bool firstClick = true, gameActive = false;

enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON } state = MENU;
Clock gameClock;
float elapsedTime = 0.f;
std::string playerName = "Player";

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
    firstClick = true; gameActive = false; elapsedTime = 0.f; moves = 0; gameClock.restart();
    for (int i = 0; i < cols * rows; ++i) {
        grid[i % cols + 1][i / cols + 1] = 0;
        sgrid[i % cols + 1][i / cols + 1] = 10;
    }
}

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

bool checkWin() {
    int opened = 0;
    for (int i = 0; i < cols * rows; ++i) opened += (sgrid[i % cols + 1][i / cols + 1] < 9);
    return opened == (cols * rows - totalMines);
}

int main() {
    std::cout << "Enter your name: "; std::cin >> playerName;
    srand(time(0));

    RenderWindow app(VideoMode({800u, 600u}), "Minesweeper Menu");
    app.setFramerateLimit(60);

    Texture t;
    if (!t.loadFromFile("images/tiles.jpg")) return -1;
    Sprite spr(t);

    Font font;
    if (!font.openFromFile("fonts/Carlito-Regular.ttf")) return -1;

    auto mkText = [&](Text& txt, const std::string& s, unsigned sz, Color c, float y) {
        txt.setString(s); txt.setCharacterSize(sz); txt.setFillColor(c);
        txt.setPosition({400.f - txt.getGlobalBounds().size.x / 2.f, y});
    };

    // Меню
    Text title(font), bEasy(font), bMed(font), bHard(font), infoTxt(font);
    mkText(title, "MINESWEEPER", 60, Color::Black, 30);
    mkText(bEasy, "1. Beginner (9x9, 10 mines)",       30, Color::Blue, 120);
    mkText(bMed,  "2. Intermediate (16x16, 40 mines)",  30, Color::Blue, 170);
    mkText(bHard, "3. Expert (30x16, 99 mines)",        30, Color::Blue, 220);
    infoTxt.setCharacterSize(20); infoTxt.setFillColor(Color::Black);

    while (app.isOpen()) {
        while (const auto e = app.pollEvent()) {
            if (e->is<Event::Closed>()) app.close();

            if (const auto* mb = e->getIf<Event::MouseButtonPressed>()) {
                float mx = (float)mb->position.x, my = (float)mb->position.y;

                if (state == MENU && mb->button == Mouse::Button::Left) {
                    bool start = false;
                    if (bEasy.getGlobalBounds().contains({mx,my})) { cols=9;  rows=9;  totalMines=10; start=true; }
                    else if (bMed.getGlobalBounds().contains({mx,my})) { cols=16; rows=16; totalMines=40; start=true; }
                    else if (bHard.getGlobalBounds().contains({mx,my})) { cols=30; rows=16; totalMines=99; start=true; }
                    if (start) {
                        state = PLAYING; resetGame(); gameActive = true;
                        app.create(VideoMode({(unsigned)((cols+2)*32),(unsigned)((rows+3)*32)}), "Minesweeper");
                    }
                } else if (state == PLAYING) {
                    int x = (int)mx / 32, y = (int)my / 32;
                    if (x > 0 && x <= cols && y > 0 && y <= rows) {
                        if (mb->button == Mouse::Button::Left && sgrid[x][y] == 10) {
                            if (firstClick) { generateField(x, y); firstClick=false; gameClock.restart(); }
                            if (grid[x][y] == 9) {
                                state = GAME_OVER;
                                for (int i = 0; i < cols*rows; ++i)
                                    if (grid[i%cols+1][i/cols+1]==9) sgrid[i%cols+1][i/cols+1]=9;
                            } else {
                                moves++;
                                grid[x][y] == 0 ? openCells(x, y) : (void)(sgrid[x][y] = grid[x][y]);
                                if (checkWin()) { state = GAME_WON; elapsedTime += gameClock.getElapsedTime().asSeconds(); }
                            }
                        } else if (mb->button == Mouse::Button::Right && sgrid[x][y] >= 10) {
                            sgrid[x][y] = (sgrid[x][y] == 10) ? 11 : 10;
                        }
                    }
                } else if ((state == GAME_OVER || state == GAME_WON) && mb->button == Mouse::Button::Left) {
                    state = MENU;
                    app.create(VideoMode({800u, 600u}), "Minesweeper Menu");
                }
            }
        }

        if (state == PLAYING && !firstClick && gameActive) elapsedTime += gameClock.restart().asSeconds();
        else if (state == PLAYING) gameClock.restart();

        app.clear(Color::White);
        if (state == MENU) {
            app.draw(title); app.draw(bEasy); app.draw(bMed); app.draw(bHard);
        } else {
            for (int i = 0; i < cols * rows; ++i) {
                int x = i % cols + 1, y = i / cols + 1;
                spr.setTextureRect(IntRect({sgrid[x][y] * 32, 0}, {32, 32}));
                spr.setPosition({(float)(x*32), (float)(y*32)});
                app.draw(spr);
            }
            std::string inf = "Time: " + std::to_string((int)elapsedTime) + "s  Moves: " + std::to_string(moves);
            if (state == GAME_OVER) inf += "  [ GAME OVER ]";
            if (state == GAME_WON)  inf += "  [ YOU WIN! ]";
            infoTxt.setString(inf);
            infoTxt.setPosition({32.f, (float)((rows+1)*32)});
            app.draw(infoTxt);
        }
        app.display();
    }
    return 0;
}
