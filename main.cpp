#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>

using namespace sf;

const int MAX_C = 32, MAX_R = 32;
int grid[MAX_C][MAX_R], sgrid[MAX_C][MAX_R];
int cols = 9, rows = 9, totalMines = 10, moves = 0;
bool firstClick = true, gameActive = false, logOn = false;

// Переименовано в GameState чтобы не конфликтовать с sf::State (SFML 3)
enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON } state = MENU;
Clock gameClock;
float elapsedTime = 0.f;

struct Record { std::string name; float time; int level; };
std::vector<Record> records;
std::ofstream logFile;
std::string playerName = "Player";

// --- Вспомогательные функции ---
void loadRecords() {
    records.clear();
    std::ifstream f("records.txt");
    if (f) { Record r; while (f >> r.name >> r.time >> r.level) records.push_back(r); }
}

void saveRecords() {
    std::ofstream f("records.txt");
    for (auto& r : records) f << r.name << " " << r.time << " " << r.level << "\n";
}

void logAct(const std::string& act) {
    if (!logOn) return;
    if (!logFile.is_open()) logFile.open("game.log", std::ios::app);
    logFile << std::fixed << std::setprecision(2) << "[" << elapsedTime + gameClock.getElapsedTime().asSeconds() << "s] " << act << "\n";
    logFile.flush();
}

// Возвращает: 1 = успех, 0 = файл не найден, -1 = файл повреждён
int ioGame(bool save) {
    if (save) {
        std::ofstream f("savegame.txt");
        if (!f) return 0;
        f << cols << " " << rows << " " << totalMines << " "
          << elapsedTime << " " << moves << " " << firstClick << "\n";
        for (int i = 0; i < cols * rows; ++i)
            f << grid[i % cols + 1][i / cols + 1] << " ";
        f << "\n";
        for (int i = 0; i < cols * rows; ++i)
            f << sgrid[i % cols + 1][i / cols + 1] << " ";
        f << "\n";
    } else {
        std::ifstream f("savegame.txt");
        if (!f) return 0;

        int tmpCols, tmpRows, tmpMines, tmpMoves, tmpFirst;
        float tmpTime;
        if (!(f >> tmpCols >> tmpRows >> tmpMines >> tmpTime >> tmpMoves >> tmpFirst))
            return -1;

        // Проверяем размеры ДО чтения массивов — иначе цикл вылетит за границы
        if (tmpCols < 1 || tmpCols > MAX_C || tmpRows < 1 || tmpRows > MAX_R) return -1;
        if (tmpMines < 1 || tmpMines >= tmpCols * tmpRows) return -1;

        int tmpGrid[MAX_C][MAX_R], tmpSgrid[MAX_C][MAX_R];
        for (int i = 0; i < tmpCols * tmpRows; ++i)
            if (!(f >> tmpGrid[i % tmpCols + 1][i / tmpCols + 1])) return -1;
        for (int i = 0; i < tmpCols * tmpRows; ++i)
            if (!(f >> tmpSgrid[i % tmpCols + 1][i / tmpCols + 1])) return -1;

        // Проверяем значения клеток
        for (int i = 0; i < tmpCols * tmpRows; ++i) {
            int v = tmpGrid[i % tmpCols + 1][i / tmpCols + 1];
            if (v < 0 || v > 9) return -1;
        }

        // Всё ок — применяем загруженные данные
        cols = tmpCols; rows = tmpRows; totalMines = tmpMines;
        elapsedTime = tmpTime; moves = tmpMoves; firstClick = tmpFirst;
        for (int i = 0; i < cols * rows; ++i) {
            grid[i % cols + 1][i / cols + 1]  = tmpGrid[i % cols + 1][i / cols + 1];
            sgrid[i % cols + 1][i / cols + 1] = tmpSgrid[i % cols + 1][i / cols + 1];
        }
        gameClock.restart(); gameActive = true; state = PLAYING;
    }
    return 1;
}

// --- Игровая логика (БЕЗ ДВОЙНЫХ ЦИКЛОВ) ---
void generateField(int fx, int fy) {
    int total = cols * rows, placed = 0;
    for (int i = 0; i < total; ++i) {
        grid[i % cols + 1][i / cols + 1] = 0;
        sgrid[i % cols + 1][i / cols + 1] = 10;
    }
    while (placed < totalMines) {
        int px = rand() % cols + 1, py = rand() % rows + 1;
        if (grid[px][py] == 9 || (abs(px - fx) <= 1 && abs(py - fy) <= 1)) continue;
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
        logAct("Auto-Open " + std::to_string(x) + "," + std::to_string(y) + " = " + std::to_string(grid[x][y]));
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

// --- Main ---
int main() {
    std::cout << "--- MINESWEEPER ---\nEnter your name: "; std::cin >> playerName;
    srand(time(0)); loadRecords();

    // SFML 3: VideoMode принимает Vector2u, Texture::loadFromFile — статический метод
    RenderWindow app(VideoMode({800u, 600u}), "Minesweeper Menu");
    app.setFramerateLimit(60);

    Texture t;
    if (!t.loadFromFile("images/tiles.jpg")) { std::cerr << "Cannot load images/tiles.jpg\n"; return -1; }

    Font font;
    if (!font.openFromFile("fonts/Carlito-Regular.ttf")) { std::cerr << "Cannot load font\n"; return -1; }

    // SFML 3: Sprite требует текстуру в конструкторе
    Sprite spr(t);

    // SFML 3: Text требует шрифт в конструкторе; setFont удалён
    // mkText центрирует текст по X на позиции y
    auto mkText = [&](Text& txt, const std::string& s, unsigned sz, Color c, float y) {
        txt.setString(s); txt.setCharacterSize(sz); txt.setFillColor(c);
        txt.setPosition({400.f - txt.getGlobalBounds().size.x / 2.f, y});
    };

    // Все Text инициализируются с шрифтом сразу
    Text title(font), bEasy(font), bMed(font), bHard(font), bLoad(font), recTxt(font), infoTxt(font), statTxt(font);
    mkText(title, "MINESWEEPER", 60, Color::Black, 30);
    mkText(bEasy, "1. Beginner (9x9, 10 mines)", 30, Color::Blue, 120);
    mkText(bMed,  "2. Intermediate (16x16, 40 mines)", 30, Color::Blue, 170);
    mkText(bHard, "3. Expert (30x16, 99 mines)", 30, Color::Blue, 220);
    mkText(bLoad, "4. Load Saved Game", 30, Color::Magenta, 290);
    infoTxt.setCharacterSize(20); infoTxt.setFillColor(Color::Black);
    statTxt.setCharacterSize(20); statTxt.setFillColor(Color::Red);

    while (app.isOpen()) {
        // SFML 3: pollEvent возвращает std::optional<sf::Event>
        while (const auto e = app.pollEvent()) {
            if (e->is<Event::Closed>()) {
                if (state == PLAYING) ioGame(true);
                app.close();
            }

            if (e->is<Event::MouseButtonPressed>()) {
                const auto& mb = e->getIf<Event::MouseButtonPressed>();
                float mx = (float)mb->position.x, my = (float)mb->position.y;

                if (state == MENU && mb->button == Mouse::Button::Left) {
                    bool start = false;
                    if (bEasy.getGlobalBounds().contains(Vector2f{mx, my})) { cols=9;  rows=9;  totalMines=10; start=true; }
                    else if (bMed.getGlobalBounds().contains(Vector2f{mx, my})) { cols=16; rows=16; totalMines=40; start=true; }
                    else if (bHard.getGlobalBounds().contains(Vector2f{mx, my})) { cols=30; rows=16; totalMines=99; start=true; }
                    else if (bLoad.getGlobalBounds().contains(Vector2f{mx, my})) {
                        int res = ioGame(false);
                        if (res == 1) { logOn=true; logAct("\n--- LOADED ---"); app.setSize({(unsigned)((cols+2)*32), (unsigned)((rows+3)*32)}); app.setTitle("Игра"); }
                        else if (res == 0) statTxt.setString("Сохранение не найдено!");
                        else statTxt.setString("Ошибка: файл сохранения повреждён!");
                    }
                    if (start) {
                        state = PLAYING; resetGame(); gameActive = logOn = true;
                        logAct("\n--- NEW GAME (" + std::to_string(cols) + "x" + std::to_string(rows) + ") ---");
                        app.setSize({(unsigned)((cols+2)*32), (unsigned)((rows+3)*32)}); app.setTitle("Minesweeper");
                    }
                } else if (state == PLAYING) {
                    int x = (int)mx / 32, y = (int)my / 32;
                    if (x > 0 && x <= cols && y > 0 && y <= rows) {
                        if (mb->button == Mouse::Button::Left && sgrid[x][y] == 10) {
                            if (firstClick) { generateField(x, y); firstClick=false; gameClock.restart(); logAct("First click at " + std::to_string(x) + "," + std::to_string(y)); }
                            if (grid[x][y] == 9) {
                                state = GAME_OVER; logAct("Hit MINE! Game Over.");
                                for (int i = 0; i < cols*rows; ++i) if (grid[i%cols+1][i/cols+1]==9) sgrid[i%cols+1][i/cols+1]=9;
                            } else {
                                moves++; logAct("Click " + std::to_string(x) + "," + std::to_string(y));
                                grid[x][y] == 0 ? openCells(x, y) : (void)(sgrid[x][y] = grid[x][y]);
                                if (checkWin()) {
                                    state = GAME_WON; elapsedTime += gameClock.getElapsedTime().asSeconds();
                                    logAct("WON in " + std::to_string(elapsedTime) + "s");
                                    records.push_back({playerName, elapsedTime, cols==9?0:(cols==16?1:2)});
                                    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b){ return a.time < b.time; });
                                    if (records.size() > 10) records.resize(10);
                                    saveRecords();
                                }
                            }
                        } else if (mb->button == Mouse::Button::Right && sgrid[x][y] >= 10) {
                            sgrid[x][y] = (sgrid[x][y] == 10) ? 11 : 10;
                            logAct(sgrid[x][y] == 11 ? "Flag placed" : "Flag removed");
                        }
                    }
                } else if ((state == GAME_OVER || state == GAME_WON) && mb->button == Mouse::Button::Left) {
                    state = MENU; if (logFile.is_open()) logFile.close(); loadRecords();
                    app.setSize({800u, 600u}); app.setTitle("Minesweeper Menu");
                }
            }

            if (const auto* kp = e->getIf<Event::KeyPressed>()) {
                if (kp->code == Keyboard::Key::S && state == PLAYING) { ioGame(true); logAct("Force save."); }
            }
        }

        if (state == PLAYING && !firstClick && gameActive) elapsedTime += gameClock.restart().asSeconds();
        else if (state == PLAYING) gameClock.restart();

        app.clear(Color::White);
        if (state == MENU) {
            std::string rStr = "TOP RECORDS:\n";
            for (size_t i = 0; i < std::min(records.size(), (size_t)5); ++i)
                rStr += records[i].name + " - " + std::to_string((int)records[i].time) + "s (Lvl: " + std::to_string(records[i].level) + ")\n";
            mkText(recTxt, rStr, 20, Color::Black, 360);
            statTxt.setPosition({400.f - statTxt.getGlobalBounds().size.x / 2.f, 500.f});
            app.draw(title); app.draw(bEasy); app.draw(bMed); app.draw(bHard); app.draw(bLoad); app.draw(recTxt); app.draw(statTxt);
        } else {
            // Отрисовка поля одним циклом
            for (int i = 0; i < cols * rows; ++i) {
                int x = i % cols + 1, y = i / cols + 1;
                // SFML 3: IntRect принимает позицию и размер как Vector2i
                spr.setTextureRect(IntRect({sgrid[x][y] * 32, 0}, {32, 32}));
                spr.setPosition({(float)(x * 32), (float)(y * 32)});
                app.draw(spr);
            }
            std::string inf = "Time: " + std::to_string((int)elapsedTime) + "s   Moves: " + std::to_string(moves);
            if (state == GAME_OVER) inf += "  [ GAME OVER ]";
            if (state == GAME_WON)  inf += "  [ YOU WIN! ]";
            infoTxt.setString(inf);
            infoTxt.setPosition({32.f, (float)((rows + 1) * 32)});
            app.draw(infoTxt);
        }
        app.display();
    }
    if (logFile.is_open()) logFile.close();
    return 0;
}
