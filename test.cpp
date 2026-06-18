// test.cpp — тесты методом белого и чёрного ящика (Google Test)
// Сборка: g++ -std=c++17 test.cpp -lgtest -lgtest_main -o test_runner && ./test_runner

#include <gtest/gtest.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cmath>

// ===== Глобальные переменные (копия из main.cpp без SFML) =====

const int MAX_C = 32, MAX_R = 32;
int grid[MAX_C][MAX_R], sgrid[MAX_C][MAX_R];
int cols = 9, rows = 9, totalMines = 10, moves = 0;
bool firstClick = true, gameActive = false, logOn = false;
float elapsedTime = 0.f;

enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON } state = MENU;

struct Record { std::string name; float time; int level; };
std::vector<Record> records;

// ===== Функции логики (копия из main.cpp без SFML) =====

void loadRecords() {
    records.clear();
    std::ifstream f("records.txt");
    if (f) { Record r; while (f >> r.name >> r.time >> r.level) records.push_back(r); }
}

void saveRecords() {
    std::ofstream f("records.txt");
    for (auto& r : records) f << r.name << " " << r.time << " " << r.level << "\n";
}

void logAct(const std::string&) {}

void resetGame() {
    firstClick = true; gameActive = false; elapsedTime = 0.f; moves = 0;
    for (int i = 0; i < cols * rows; ++i) {
        grid[i % cols + 1][i / cols + 1] = 0;
        sgrid[i % cols + 1][i / cols + 1] = 10;
    }
}

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
        if (!(f >> tmpCols >> tmpRows >> tmpMines >> tmpTime >> tmpMoves >> tmpFirst)) return -1;
        if (tmpCols < 1 || tmpCols > MAX_C || tmpRows < 1 || tmpRows > MAX_R) return -1;
        if (tmpMines < 1 || tmpMines >= tmpCols * tmpRows) return -1;
        int tmpGrid[MAX_C][MAX_R], tmpSgrid[MAX_C][MAX_R];
        for (int i = 0; i < tmpCols * tmpRows; ++i)
            if (!(f >> tmpGrid[i % tmpCols + 1][i / tmpCols + 1])) return -1;
        for (int i = 0; i < tmpCols * tmpRows; ++i)
            if (!(f >> tmpSgrid[i % tmpCols + 1][i / tmpCols + 1])) return -1;
        for (int i = 0; i < tmpCols * tmpRows; ++i) {
            int v = tmpGrid[i % tmpCols + 1][i / tmpCols + 1];
            if (v < 0 || v > 9) return -1;
        }
        cols = tmpCols; rows = tmpRows; totalMines = tmpMines;
        elapsedTime = tmpTime; moves = tmpMoves; firstClick = tmpFirst;
        for (int i = 0; i < cols * rows; ++i) {
            grid[i % cols + 1][i / cols + 1]  = tmpGrid[i % cols + 1][i / cols + 1];
            sgrid[i % cols + 1][i / cols + 1] = tmpSgrid[i % cols + 1][i / cols + 1];
        }
        gameActive = true; state = PLAYING;
    }
    return 1;
}

// ===== Вспомогательные функции =====

void cleanFiles() {
    std::remove("records.txt");
    std::remove("savegame.txt");
}

// ===== ЧЕРНЫЙ ЯЩИК: records.txt =====

TEST(BlackBox_Records, LoadFromMissingFile) {
    cleanFiles();
    loadRecords();
    EXPECT_TRUE(records.empty());
}

TEST(BlackBox_Records, SaveAndLoadName) {
    cleanFiles();
    records = {{"Alice", 42.5f, 0}};
    saveRecords();
    records.clear();
    loadRecords();
    ASSERT_FALSE(records.empty());
    EXPECT_EQ(records[0].name, "Alice");
}

TEST(BlackBox_Records, SaveAndLoadTime) {
    cleanFiles();
    records = {{"Alice", 42.5f, 0}};
    saveRecords();
    records.clear();
    loadRecords();
    ASSERT_FALSE(records.empty());
    EXPECT_NEAR(records[0].time, 42.5f, 1.0f);
}

TEST(BlackBox_Records, SaveAndLoadLevel) {
    cleanFiles();
    records = {{"Alice", 42.5f, 2}};
    saveRecords();
    records.clear();
    loadRecords();
    ASSERT_FALSE(records.empty());
    EXPECT_EQ(records[0].level, 2);
}

TEST(BlackBox_Records, MultipleRecordsCount) {
    cleanFiles();
    records = {{"Bob", 10.f, 1}, {"Carol", 55.f, 2}, {"Dave", 30.f, 0}};
    saveRecords();
    records.clear();
    loadRecords();
    EXPECT_EQ(records.size(), 3u);
}

TEST(BlackBox_Records, MultipleRecordsOrder) {
    cleanFiles();
    records = {{"Bob", 10.f, 1}, {"Carol", 55.f, 2}, {"Dave", 30.f, 0}};
    saveRecords();
    records.clear();
    loadRecords();
    ASSERT_GE(records.size(), 1u);
    EXPECT_EQ(records[0].name, "Bob");
}

TEST(BlackBox_Records, OverwriteOnSave) {
    cleanFiles();
    records = {{"Old", 99.f, 0}};
    saveRecords();
    records = {{"New", 1.f, 0}};
    saveRecords();
    records.clear();
    loadRecords();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].name, "New");
}

// ===== ЧЕРНЫЙ ЯЩИК: ioGame =====

TEST(BlackBox_IoGame, LoadMissingFileReturns0) {
    cleanFiles();
    EXPECT_EQ(ioGame(false), 0);
}

TEST(BlackBox_IoGame, SaveReturns1) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    EXPECT_EQ(ioGame(true), 1);
    cleanFiles();
}

TEST(BlackBox_IoGame, FileCreatedAfterSave) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    ioGame(true);
    std::ifstream f("savegame.txt");
    EXPECT_TRUE(f.good());
    cleanFiles();
}

TEST(BlackBox_IoGame, LoadAfterSaveReturns1) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    ioGame(true);
    EXPECT_EQ(ioGame(false), 1);
    cleanFiles();
}

TEST(BlackBox_IoGame, ColsRestoredCorrectly) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    ioGame(true);
    cols = 0;
    ioGame(false);
    EXPECT_EQ(cols, 9);
    cleanFiles();
}

TEST(BlackBox_IoGame, RowsRestoredCorrectly) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    ioGame(true);
    rows = 0;
    ioGame(false);
    EXPECT_EQ(rows, 9);
    cleanFiles();
}

TEST(BlackBox_IoGame, MinesRestoredCorrectly) {
    cols = 9; rows = 9; totalMines = 10;
    resetGame();
    ioGame(true);
    totalMines = 0;
    ioGame(false);
    EXPECT_EQ(totalMines, 10);
    cleanFiles();
}

// ===== ЧЕРНЫЙ ЯЩИК: generateField =====

TEST(BlackBox_Generate, FirstClickNotMine) {
    srand(42);
    cols = 9; rows = 9; totalMines = 10;
    generateField(5, 5);
    EXPECT_NE(grid[5][5], 9);
}

TEST(BlackBox_Generate, SafeZoneAroundFirstClick) {
    srand(42);
    cols = 9; rows = 9; totalMines = 10;
    generateField(5, 5);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = 5+dx, ny = 5+dy;
            if (nx >= 1 && nx <= cols && ny >= 1 && ny <= rows)
                EXPECT_NE(grid[nx][ny], 9)
                    << "Мина в зоне безопасности: (" << nx << "," << ny << ")";
        }
}

// ===== ЧЕРНЫЙ ЯЩИК: checkWin =====

TEST(BlackBox_CheckWin, AllCellsClosedNoWin) {
    cols = 3; rows = 3; totalMines = 1;
    resetGame();
    EXPECT_FALSE(checkWin());
}

TEST(BlackBox_CheckWin, AllNonMinesOpenedWin) {
    cols = 3; rows = 3; totalMines = 1;
    resetGame();
    grid[1][1] = 9;
    for (int i = 0; i < cols * rows; ++i) {
        int x = i % cols + 1, y = i / cols + 1;
        if (grid[x][y] != 9) sgrid[x][y] = grid[x][y];
    }
    EXPECT_TRUE(checkWin());
}

TEST(BlackBox_CheckWin, OneNonMineClosedNoWin) {
    cols = 3; rows = 3; totalMines = 1;
    resetGame();
    grid[1][1] = 9;
    for (int i = 0; i < cols * rows; ++i) {
        int x = i % cols + 1, y = i / cols + 1;
        if (grid[x][y] != 9) sgrid[x][y] = grid[x][y];
    }
    sgrid[2][1] = 10;
    EXPECT_FALSE(checkWin());
}

// ===== ЧЕРНЫЙ ЯЩИК: openCells =====

TEST(BlackBox_OpenCells, NumberedCellOpensOnlyItself) {
    cols = 5; rows = 5; totalMines = 1;
    resetGame();
    for (int i = 0; i < cols*rows; ++i) grid[i%cols+1][i/cols+1] = 0;
    grid[5][5] = 9;
    grid[4][5] = 1;
    sgrid[4][5] = 10;
    openCells(4, 5);
    EXPECT_EQ(sgrid[4][5], 1);
    EXPECT_EQ(sgrid[3][5], 10);
}

TEST(BlackBox_OpenCells, ZeroCellOpensNeighbors) {
    cols = 5; rows = 5; totalMines = 1;
    resetGame();
    for (int i = 0; i < cols*rows; ++i) {
        grid[i%cols+1][i/cols+1] = 0;
        sgrid[i%cols+1][i/cols+1] = 10;
    }
    grid[5][5] = 9;
    openCells(1, 1);
    int openCount = 0;
    for (int i = 0; i < cols*rows; ++i)
        if (sgrid[i%cols+1][i/cols+1] < 9) openCount++;
    EXPECT_GT(openCount, 1);
}

TEST(BlackBox_OpenCells, AlreadyOpenCellUnchanged) {
    cols = 5; rows = 5; totalMines = 1;
    resetGame();
    for (int i = 0; i < cols*rows; ++i) grid[i%cols+1][i/cols+1] = 0;
    grid[5][5] = 9;
    openCells(1, 1);
    int before = sgrid[1][1];
    openCells(1, 1);
    EXPECT_EQ(sgrid[1][1], before);
}

// ===== БЕЛЫЙ ЯЩИК: generateField =====

TEST(WhiteBox_Generate, ExactMineCount) {
    srand(1234);
    cols = 9; rows = 9; totalMines = 10;
    generateField(5, 5);
    int count = 0;
    for (int i = 0; i < cols*rows; ++i)
        if (grid[i%cols+1][i/cols+1] == 9) count++;
    EXPECT_EQ(count, totalMines);
}

TEST(WhiteBox_Generate, CellValuesInRange) {
    srand(1234);
    cols = 9; rows = 9; totalMines = 10;
    generateField(5, 5);
    for (int i = 0; i < cols*rows; ++i) {
        int v = grid[i%cols+1][i/cols+1];
        EXPECT_GE(v, 0) << "Клетка имеет значение < 0";
        EXPECT_LE(v, 9) << "Клетка имеет значение > 9";
    }
}

TEST(WhiteBox_Generate, NeighborCountIsCorrect) {
    srand(1234);
    cols = 9; rows = 9; totalMines = 10;
    generateField(5, 5);
    for (int i = 0; i < cols*rows; ++i) {
        int x = i%cols+1, y = i/cols+1;
        if (grid[x][y] == 9) continue;
        int expected = 0;
        for (int d = 0; d < 9; ++d) {
            int nx = x+(d%3-1), ny = y+(d/3-1);
            if (nx>0 && nx<=cols && ny>0 && ny<=rows && grid[nx][ny]==9) expected++;
        }
        EXPECT_EQ(grid[x][y], expected)
            << "Неверный счётчик мин в клетке (" << x << "," << y << ")";
    }
}

// ===== БЕЛЫЙ ЯЩИК: ioGame — валидация испорченных данных =====

TEST(WhiteBox_IoGame, ColsZeroReturnsMinusOne) {
    std::ofstream f("savegame.txt");
    f << "0 9 10 0.0 0 1\n";
    for (int i = 0; i < 81; ++i) f << "0 ";
    f << "\n";
    for (int i = 0; i < 81; ++i) f << "10 ";
    f << "\n";
    f.close();
    EXPECT_EQ(ioGame(false), -1);
    cleanFiles();
}

TEST(WhiteBox_IoGame, ColsOverMaxReturnsMinusOne) {
    std::ofstream f("savegame.txt");
    f << "999 9 10 0.0 0 1\n";
    f.close();
    EXPECT_EQ(ioGame(false), -1);
    cleanFiles();
}

TEST(WhiteBox_IoGame, MinesEqualsTotalCellsReturnsMinusOne) {
    std::ofstream f("savegame.txt");
    f << "9 9 81 0.0 0 1\n";
    for (int i = 0; i < 81; ++i) f << "0 ";
    f << "\n";
    for (int i = 0; i < 81; ++i) f << "10 ";
    f << "\n";
    f.close();
    EXPECT_EQ(ioGame(false), -1);
    cleanFiles();
}

TEST(WhiteBox_IoGame, InvalidCellValueReturnsMinusOne) {
    std::ofstream f("savegame.txt");
    f << "3 3 1 0.0 0 1\n";
    for (int i = 0; i < 9; ++i) f << (i==0 ? 99 : 0) << " ";
    f << "\n";
    for (int i = 0; i < 9; ++i) f << "10 ";
    f << "\n";
    f.close();
    EXPECT_EQ(ioGame(false), -1);
    cleanFiles();
}

// ===== БЕЛЫЙ ЯЩИК: resetGame =====

TEST(WhiteBox_Reset, FirstClickIsTrue) {
    cols = 9; rows = 9;
    firstClick = false;
    resetGame();
    EXPECT_TRUE(firstClick);
}

TEST(WhiteBox_Reset, MovesIsZero) {
    cols = 9; rows = 9;
    moves = 42;
    resetGame();
    EXPECT_EQ(moves, 0);
}

TEST(WhiteBox_Reset, ElapsedTimeIsZero) {
    cols = 9; rows = 9;
    elapsedTime = 100.f;
    resetGame();
    EXPECT_FLOAT_EQ(elapsedTime, 0.f);
}

TEST(WhiteBox_Reset, AllSgridIs10) {
    cols = 9; rows = 9;
    resetGame();
    for (int i = 0; i < cols*rows; ++i) {
        int x = i%cols+1, y = i/cols+1;
        EXPECT_EQ(sgrid[x][y], 10)
            << "sgrid[" << x << "][" << y << "] != 10 после resetGame";
    }
}
