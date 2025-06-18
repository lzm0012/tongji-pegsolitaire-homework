#pragma execution_character_set("utf-8")
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <graphics.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <stack>
#include <cmath>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <functional>
#include <thread>
#include <windows.h>
#include <tchar.h>
#include <Mmsystem.h>
#include <atomic>
#include <mutex>
#include <future>
#include <memory>
#include <chrono>

#include "board.h"
#include "ai_solver.h"

#pragma comment(lib, "winmm.lib")
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*简单的孔明棋小游戏
* powered by EasyX Graphics Library
* powered by C++11
* powered by Intel Core Ultra 5 225H
* powered by AMD Ryzen 9 9900X
* powered by NVIDIA GeForce RTX 3090
* written by lzm
*/
using namespace std;


map<string, vector<Move>> solutionCache;

Board::Board(int w, int h) : width(w), height(h) {
    grid.resize(height, vector<int>(width, -1));
    boardHistory.push_back(grid);
}
Board::~Board() {}
void Board::clearBoardHistory() { boardHistory.clear(); }
void Board::addToBoardHistory(const vector<vector<int>>& state) { boardHistory.push_back(state); }
const vector<vector<int>>& Board::getGrid() const { return grid; }
string Board::getStateHash() const {
    string hash_str;
    hash_str.reserve(width * height);
    for (int y_idx = 0; y_idx < height; ++y_idx) {
        for (int x_idx = 0; x_idx < width; ++x_idx) {
            if (grid[y_idx][x_idx] != -1) {
                hash_str += to_string(grid[y_idx][x_idx] + 1);
            }
        }
    }
    return hash_str;
}
bool Board::makeMove(const Move& move) {
    if (!isValidMove(move)) return false;
    boardHistory.push_back(grid);
    moveHistory.push_back(move);
    grid[move.from_y][move.from_x] = 0;
    grid[move.over_y][move.over_x] = 0;
    grid[move.to_y][move.to_x] = 1;
    return true;
}
bool Board::undoMove() {
    if (boardHistory.size() <= 1) return false;
    grid = boardHistory.back();
    boardHistory.pop_back();
    if (!moveHistory.empty()) moveHistory.pop_back();
    return true;
}
void Board::resetBoard() {
    moveHistory.clear();
    boardHistory.clear();
    initializeBoard();
    boardHistory.push_back(grid);
}
bool Board::isValidMove(const Move& move) const {
    if (!isValidPosition(move.from_x, move.from_y) ||
        !isValidPosition(move.over_x, move.over_y) ||
        !isValidPosition(move.to_x, move.to_y)) {
        return false;
    }
    if (getPeg(move.from_x, move.from_y) != 1) return false;
    if (getPeg(move.over_x, move.over_y) != 1) return false;
    if (getPeg(move.to_x, move.to_y) != 0) return false;
    int dx_total = move.to_x - move.from_x;
    int dy_total = move.to_y - move.from_y;
    if (dx_total == 0 && dy_total == 0) return false;
    if (dx_total % 2 != 0 || dy_total % 2 != 0) return false;
    if (move.over_x != (move.from_x + dx_total / 2) ||
        move.over_y != (move.from_y + dy_total / 2)) {
        return false;
    }
    return true;
}
int Board::getPegCount() const {
    int count = 0;
    for (const auto& row : grid) for (int cell : row) if (cell == 1) count++;
    return count;
}
bool Board::isGameWon() const { return getPegCount() == 1; }
bool Board::isGameLost() const { return getAllPossibleMoves().empty() && getPegCount() > 1; }
void Board::setPeg(int x, int y, int value) { if (isValidPosition(x, y)) grid[y][x] = value; }
int Board::getPeg(int x, int y) const { if (y >= 0 && y < height && x >= 0 && x < width) return grid[y][x]; return -1; }
int Board::getWidth() const { return width; }
int Board::getHeight() const { return height; }

TriangleBoard::TriangleBoard() : Board(5, 5) { initializeBoard(); }
void TriangleBoard::initializeBoard() {
    for (int y = 0; y < 5; y++)
        for (int x = 0; x <= y; x++) {
            grid[y][x] = 1;
        }
    grid[2][2] = 0;
}
bool TriangleBoard::isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height && x <= y;
}
vector<Move> TriangleBoard::getAllPossibleMoves() const {
    vector<Move> moves;
    for (int y_coord = 0; y_coord < height; y_coord++)
        for (int x_coord = 0; x_coord < width; x_coord++) {
            if (getPeg(x_coord, y_coord) != 1) continue;
            vector<pair<int, int>> directions = { {2,0}, {-2,0}, {0,2}, {0,-2}, {2,2}, {-2,-2} };
            for (auto& dir : directions) {
                Move move(x_coord, y_coord, x_coord + dir.first / 2, y_coord + dir.second / 2, x_coord + dir.first, y_coord + dir.second);
                if (Board::isValidMove(move)) moves.push_back(move);
            }
        }
    return moves;
}
void TriangleBoard::drawBoard(int offsetX, int offsetY) const {
    const int pegSize = 25, spacing = 60;
    for (int y_coord = 0; y_coord < 5; y_coord++)
        for (int x_coord = 0; x_coord <= y_coord; x_coord++) {
            int screenX = offsetX + x_coord * spacing + (4 - y_coord) * spacing / 2;
            int screenY = offsetY + y_coord * spacing;
            setfillcolor(RGB(139, 69, 19));
            fillcircle(screenX, screenY, pegSize + 3);
            if (getPeg(x_coord, y_coord) == 1) {
                for (int r = pegSize; r >= 0; r -= 2) {
                    float ratio = (float)r / pegSize;
                    COLORREF color = RGB((int)(255 * (0.8f + 0.2f * ratio)), (int)(215 * (0.8f + 0.2f * ratio)), (int)(0 * (0.8f + 0.2f * ratio)));
                    setfillcolor(color);
                    fillcircle(screenX, screenY, r);
                }
                setfillcolor(RGB(255, 255, 255));
                fillcircle(screenX - pegSize / 3, screenY - pegSize / 3, pegSize / 4);
                setcolor(RGB(184, 134, 11));
                circle(screenX, screenY, pegSize);
            }
            else if (getPeg(x_coord, y_coord) == 0) {
                setfillcolor(RGB(101, 67, 33));
                fillcircle(screenX, screenY, pegSize);
                setfillcolor(RGB(80, 50, 20));
                fillcircle(screenX, screenY, pegSize - 3);
            }
        }
}
Position TriangleBoard::screenToBoard(int screenX, int screenY, int offsetX, int offsetY) const {
    const int spacing = 60, pegSize = 25;
    for (int y_coord = 0; y_coord < 5; y_coord++)
        for (int x_coord = 0; x_coord <= y_coord; x_coord++) {
            int boardScreenX = offsetX + x_coord * spacing + (4 - y_coord) * spacing / 2;
            int boardScreenY = offsetY + y_coord * spacing;
            if (hypot(screenX - boardScreenX, screenY - boardScreenY) <= pegSize + 5)
                return Position(x_coord, y_coord);
        }
    return Position(-1, -1);
}
Position TriangleBoard::boardToScreen(int boardX, int boardY, int offsetX, int offsetY) const {
    if (!isValidPosition(boardX, boardY)) return Position(-1, -1);
    const int spacing = 60;
    int screenX = offsetX + boardX * spacing + (4 - boardY) * spacing / 2;
    int screenY = offsetY + boardY * spacing;
    return Position(screenX, screenY);
}
std::unique_ptr<Board> TriangleBoard::clone() const {
    return std::make_unique<TriangleBoard>(*this);
}

SquareBoard::SquareBoard() : Board(7, 7) { initializeBoard(); }
void SquareBoard::initializeBoard() {
    for (int y = 0; y < 7; y++) for (int x = 0; x < 7; x++) {
        if ((x >= 2 && x <= 4) || (y >= 2 && y <= 4)) grid[y][x] = 1; else grid[y][x] = -1;
    }
    grid[3][3] = 0;
}
bool SquareBoard::isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height && ((x >= 2 && x <= 4) || (y >= 2 && y <= 4));
}
vector<Move> SquareBoard::getAllPossibleMoves() const {
    vector<Move> moves;
    for (int y_coord = 0; y_coord < height; y_coord++)
        for (int x_coord = 0; x_coord < width; x_coord++) {
            if (getPeg(x_coord, y_coord) != 1) continue;
            vector<pair<int, int>> directions = { {2,0}, {-2,0}, {0,2}, {0,-2} };
            for (auto& dir : directions) {
                Move move(x_coord, y_coord, x_coord + dir.first / 2, y_coord + dir.second / 2, x_coord + dir.first, y_coord + dir.second);
                if (Board::isValidMove(move)) moves.push_back(move);
            }
        }
    return moves;
}
void SquareBoard::drawBoard(int offsetX, int offsetY) const {
    const int pegSize = 20, spacing = 50;
    for (int y_coord = 0; y_coord < 7; y_coord++)
        for (int x_coord = 0; x_coord < 7; x_coord++) {
            if (!isValidPosition(x_coord, y_coord)) continue;
            int screenX = offsetX + x_coord * spacing;
            int screenY = offsetY + y_coord * spacing;
            setfillcolor(RGB(139, 69, 19));
            fillcircle(screenX, screenY, pegSize + 2);
            if (getPeg(x_coord, y_coord) == 1) {
                for (int r = pegSize; r >= 0; r -= 1) {
                    float ratio = (float)r / pegSize;
                    COLORREF color = RGB((int)(255 * (0.8f + 0.2f * ratio)), (int)(215 * (0.8f + 0.2f * ratio)), 0);
                    setfillcolor(color);
                    fillcircle(screenX, screenY, r);
                }
                setfillcolor(RGB(255, 255, 255));
                fillcircle(screenX - pegSize / 3, screenY - pegSize / 3, pegSize / 5);
                setcolor(RGB(184, 134, 11));
                circle(screenX, screenY, pegSize);
            }
            else if (getPeg(x_coord, y_coord) == 0) {
                setfillcolor(RGB(101, 67, 33));
                fillcircle(screenX, screenY, pegSize);
                setfillcolor(RGB(80, 50, 20));
                fillcircle(screenX, screenY, pegSize - 2);
            }
        }
}
Position SquareBoard::screenToBoard(int screenX, int screenY, int offsetX, int offsetY) const {
    const int spacing = 50, pegSize = 20;
    for (int y_coord = 0; y_coord < 7; y_coord++)
        for (int x_coord = 0; x_coord < 7; x_coord++) {
            if (!isValidPosition(x_coord, y_coord)) continue;
            int boardScreenX = offsetX + x_coord * spacing;
            int boardScreenY = offsetY + y_coord * spacing;
            if (hypot(screenX - boardScreenX, screenY - boardScreenY) <= pegSize + 5)
                return Position(x_coord, y_coord);
        }
    return Position(-1, -1);
}
Position SquareBoard::boardToScreen(int boardX, int boardY, int offsetX, int offsetY) const {
    if (!isValidPosition(boardX, boardY)) return Position(-1, -1);
    const int spacing = 50;
    int screenX = offsetX + boardX * spacing;
    int screenY = offsetY + boardY * spacing;
    return Position(screenX, screenY);
}
std::unique_ptr<Board> SquareBoard::clone() const {
    return std::make_unique<SquareBoard>(*this);
}

// --- HexagonBoard Implementations ---
HexagonBoard::HexagonBoard() : Board(9, 9) { initializeBoard(); }

void HexagonBoard::initializeBoard() {
    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            grid[y][x] = -1;
        }
    }
    int validPositions[9][9] = {
        {-1, -1, -1, -1,  1, -1, -1, -1, -1},  
        {-1, -1, -1,  1,  1,  1, -1, -1, -1},  
        {-1, -1,  1,  1,  1,  1,  1, -1, -1},  
        {-1,  1,  1,  1,  1,  1,  1,  1, -1},  
        { 1,  1,  1,  1,  0,  1,  1,  1,  1},  
        {-1,  1,  1,  1,  1,  1,  1,  1, -1},  
        {-1, -1,  1,  1,  1,  1,  1, -1, -1},  
        {-1, -1, -1,  1,  1,  1, -1, -1, -1},  
        {-1, -1, -1, -1,  1, -1, -1, -1, -1}   
    };

    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            grid[y][x] = validPositions[y][x];
        }
    }
}

bool HexagonBoard::isValidPosition(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) return false;
    return grid[y][x] != -1;
}

vector<Move> HexagonBoard::getAllPossibleMoves() const {
    vector<Move> moves;
    for (int y_coord = 0; y_coord < height; y_coord++) {
        for (int x_coord = 0; x_coord < width; x_coord++) {
            if (getPeg(x_coord, y_coord) != 1) continue;

            int directions[][2] = {
                {2, 0}, {-2, 0},    
                {0, 2}, {0, -2},    
                {1, 1}, {-1, -1},   
                {1, -1}, {-1, 1}    
            };

            for (auto& dir : directions) {
                Move move(x_coord, y_coord,
                    x_coord + dir[0] / 2, y_coord + dir[1] / 2,
                    x_coord + dir[0], y_coord + dir[1]);
                if (Board::isValidMove(move)) moves.push_back(move);
            }
        }
    }
    return moves;
}

void HexagonBoard::drawBoard(int offsetX, int offsetY) const {
    const int pegSize = 22;  
    const int cellSpacing = 55; 
    setfillcolor(RGB(160, 82, 45));  
    fillroundrect(offsetX - 50, offsetY - 50, offsetX + 450, offsetY + 450, 20, 20);

    for (int y_coord = 0; y_coord < height; y_coord++) {
        for (int x_coord = 0; x_coord < width; x_coord++) {
            if (!isValidPosition(x_coord, y_coord)) continue;

            int screenX = offsetX + x_coord * cellSpacing;
            int screenY = offsetY + y_coord * cellSpacing;

            setfillcolor(RGB(139, 69, 19));
            fillcircle(screenX, screenY, pegSize + 3);

            if (getPeg(x_coord, y_coord) == 1) {
                for (int r = pegSize; r >= 0; r -= 1) {
                    float ratio = (float)r / pegSize;
                    COLORREF color = RGB(
                        (int)(255 * (0.8f + 0.2f * ratio)),
                        (int)(215 * (0.8f + 0.2f * ratio)),
                        0
                    );
                    setfillcolor(color);
                    fillcircle(screenX, screenY, r);
                }
                setfillcolor(RGB(255, 255, 255));
                fillcircle(screenX - pegSize / 3, screenY - pegSize / 3, pegSize / 5);
                setcolor(RGB(184, 134, 11));
                circle(screenX, screenY, pegSize);
            }
            else if (getPeg(x_coord, y_coord) == 0) {
                setfillcolor(RGB(101, 67, 33));
                fillcircle(screenX, screenY, pegSize);
                setfillcolor(RGB(80, 50, 20));
                fillcircle(screenX, screenY, pegSize - 2);
            }
        }
    }

    setcolor(RGB(101, 67, 33));
    setlinestyle(PS_SOLID, 2);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width - 1; x++) {
            if (isValidPosition(x, y) && isValidPosition(x + 1, y)) {
                int x1 = offsetX + x * cellSpacing + pegSize;
                int y1 = offsetY + y * cellSpacing;
                int x2 = offsetX + (x + 1) * cellSpacing - pegSize;
                int y2 = y1;
                line(x1, y1, x2, y2);
            }
        }
    }

    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width; x++) {
            if (isValidPosition(x, y) && isValidPosition(x, y + 1)) {
                int x1 = offsetX + x * cellSpacing;
                int y1 = offsetY + y * cellSpacing + pegSize;
                int x2 = x1;
                int y2 = offsetY + (y + 1) * cellSpacing - pegSize;
                line(x1, y1, x2, y2);
            }
        }
    }
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            if (isValidPosition(x, y) && isValidPosition(x + 1, y + 1)) {
                int x1 = offsetX + x * cellSpacing + pegSize / 1.4;
                int y1 = offsetY + y * cellSpacing + pegSize / 1.4;
                int x2 = offsetX + (x + 1) * cellSpacing - pegSize / 1.4;
                int y2 = offsetY + (y + 1) * cellSpacing - pegSize / 1.4;
                line(x1, y1, x2, y2);
            }
            if (x + 1 < width && isValidPosition(x + 1, y) && isValidPosition(x, y + 1)) {
                int x1 = offsetX + (x + 1) * cellSpacing - pegSize / 1.4;
                int y1 = offsetY + y * cellSpacing + pegSize / 1.4;
                int x2 = offsetX + x * cellSpacing + pegSize / 1.4;
                int y2 = offsetY + (y + 1) * cellSpacing - pegSize / 1.4;
                line(x1, y1, x2, y2);
            }
        }
    }
}

Position HexagonBoard::screenToBoard(int screenX, int screenY, int offsetX, int offsetY) const {
    const int pegSize = 22;
    const int cellSpacing = 55;

    for (int y_coord = 0; y_coord < height; y_coord++) {
        for (int x_coord = 0; x_coord < width; x_coord++) {
            if (!isValidPosition(x_coord, y_coord)) continue;

            int boardScreenX = offsetX + x_coord * cellSpacing;
            int boardScreenY = offsetY + y_coord * cellSpacing;

            if (hypot(screenX - boardScreenX, screenY - boardScreenY) <= pegSize + 5) {
                return Position(x_coord, y_coord);
            }
        }
    }
    return Position(-1, -1);
}

Position HexagonBoard::boardToScreen(int boardX, int boardY, int offsetX, int offsetY) const {
    if (!isValidPosition(boardX, boardY)) return Position(-1, -1);

    const int cellSpacing = 55;
    int screenX = offsetX + boardX * cellSpacing;
    int screenY = offsetY + boardY * cellSpacing;

    return Position(screenX, screenY);
}

std::unique_ptr<Board> HexagonBoard::clone() const {
    return std::make_unique<HexagonBoard>(*this);
}

std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


struct UIAnimator {
    float progress = 0.0f;
    bool animating = false;
    chrono::high_resolution_clock::time_point start_time;
    int duration_ms = 200;
    void start(int duration = 200) {
        animating = true;
        progress = 0.0f;
        duration_ms = duration;
        start_time = chrono::high_resolution_clock::now();
    }
    bool update() {
        if (!animating) return false;
        auto now = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - start_time).count();
        progress = (std::min)(1.0f, (float)elapsed / duration_ms);
        if (progress >= 1.0f) {
            animating = false;
            return false;
        }
        return true;
    }
    float easeOut() const {
        return 1.0f - pow(1.0f - progress, 3.0f);
    }
};

enum ButtonState { BTN_NORMAL, BTN_HOVER, BTN_PRESSED, BTN_DISABLED };
struct Button {
    RECT rect;
    wstring text;
    ButtonState state = BTN_NORMAL;
    COLORREF normal_color = RGB(255, 215, 0);
    COLORREF hover_color = RGB(255, 235, 59);
    COLORREF pressed_color = RGB(255, 193, 7);
    COLORREF disabled_color = RGB(150, 150, 150);
    UIAnimator animator;
    bool was_hovering = false;

    Button(int x, int y, int w, int h, const wstring& txt, COLORREF color = RGB(255, 215, 0))
        : text(txt), normal_color(color) {
        rect = { x, y, x + w, y + h };
        hover_color = RGB((std::min)(255, GetRValue(color) + 20), (std::min)(255, GetGValue(color) + 20), (std::min)(255, GetBValue(color) + 20));
        pressed_color = RGB((std::max)(0, GetRValue(color) - 20), (std::max)(0, GetGValue(color) - 20), (std::max)(0, GetGValue(color) - 20));
    }
    bool contains(POINT pt) const { return PtInRect(&rect, pt); }
    void update(POINT mouse_pos) {
        bool hovering = contains(mouse_pos) && state != BTN_DISABLED;
        if (hovering != was_hovering) {
            if (hovering) {
                state = BTN_HOVER;
                animator.start(150);
            }
            else if (state != BTN_PRESSED) {
                state = BTN_NORMAL;
                animator.start(150);
            }
            was_hovering = hovering;
        }
        animator.update();
    }
    void draw() const {
        COLORREF current_color;
        float scale = 1.0f;
        int shadow_offset = 3;
        switch (state) {
        case BTN_HOVER: current_color = hover_color; scale = 1.0f + animator.easeOut() * 0.05f; break;
        case BTN_PRESSED: current_color = pressed_color; scale = 0.95f; shadow_offset = 1; break;
        case BTN_DISABLED: current_color = disabled_color; break;
        default: current_color = normal_color; break;
        }
        int center_x = (rect.left + rect.right) / 2;
        int center_y = (rect.top + rect.bottom) / 2;
        int width = (int)((rect.right - rect.left) * scale);
        int height = (int)((rect.bottom - rect.top) * scale);
        RECT scaled_rect = { center_x - width / 2, center_y - height / 2, center_x + width / 2, center_y + height / 2 };
        if (state != BTN_PRESSED) {
            setfillcolor(RGB(0, 0, 0));
            fillroundrect(scaled_rect.left + shadow_offset, scaled_rect.top + shadow_offset, scaled_rect.right + shadow_offset, scaled_rect.bottom + shadow_offset, 10, 10);
        }
        setfillcolor(current_color);
        fillroundrect(scaled_rect.left, scaled_rect.top, scaled_rect.right, scaled_rect.bottom, 10, 10);
        setcolor(RGB(0, 0, 0));
        setlinestyle(PS_SOLID, 2);
        roundrect(scaled_rect.left, scaled_rect.top, scaled_rect.right, scaled_rect.bottom, 10, 10);

        setbkmode(TRANSPARENT);
        settextcolor(RGB(0, 0, 0));
        settextstyle(24, 0, _T("楷体"));
        int text_width = textwidth(text.c_str());
        int text_height = textheight(text.c_str());
        outtextxy(center_x - text_width / 2, center_y - text_height / 2, text.c_str());
    }
    void setState(ButtonState new_state) {
        if (state != new_state) {
            state = new_state;
            animator.start(100);
        }
    }
};

struct Particle {
    float x, y, vx, vy, life, max_life;
    COLORREF color;
    Particle(float px, float py, float pvx, float pvy, float plife, COLORREF pcolor) : x(px), y(py), vx(pvx), vy(pvy), life(plife), max_life(plife), color(pcolor) {}
    bool update(float dt) {
        x += vx * dt;
        y += vy * dt;
        life -= dt;
        vy += 100 * dt;
        return life > 0;
    }
    void draw() const {
        float alpha = life / max_life;
        int size = (int)(5 * alpha);
        if (size > 0) {
            setfillcolor(color);
            fillcircle((int)x, (int)y, size);
        }
    }
};

class ParticleSystem {
private:
    vector<Particle> particles;
public:
    void addBurst(float x, float y, COLORREF color, int count = 10) {
        for (int i = 0; i < count; ++i) {
            float angle = (float)(i * 2 * M_PI / count);
            float speed = 50 + rand() % 100;
            particles.emplace_back(x, y, cos(angle) * speed, sin(angle) * speed - 50, 1.0f, color);
        }
    }
    void update(float dt) {
        particles.erase(remove_if(particles.begin(), particles.end(), [dt](Particle& p) { return !p.update(dt); }), particles.end());
    }
    void draw() const { for (const auto& p : particles) p.draw(); }
    void clear() { particles.clear(); }
    bool empty() const { return particles.empty(); }
    size_t size() const { return particles.size(); }
};

enum GameState { MENU, GAME_PLAYING, GAME_WIN, GAME_LOSE, RULES_DISPLAY, BOARD_SELECT, LEVEL_SELECT };
enum BoardType { TRIANGLE, SQUARE, HEXAGON };

class UICache {
private:
    static IMAGE* background_cache;
    static bool cache_valid;
public:
    static void invalidate() { cache_valid = false; }
    static IMAGE* getBackground() {
        if (!cache_valid || !background_cache) {
            if (!background_cache) background_cache = new IMAGE(800, 600);
            SetWorkingImage(background_cache);
            setbkcolor(RGB(245, 245, 220));
            cleardevice();
            for (int y = 0; y < 600; ++y) {
                float ratio = (float)y / 600;
                COLORREF color = RGB((int)(245 * (1 - ratio * 0.1f)), (int)(245 * (1 - ratio * 0.1f)), (int)(220 * (1 - ratio * 0.1f)));
                setfillcolor(color);
                fillrectangle(0, y, 800, y + 1);
            }
            cache_valid = true;
            SetWorkingImage(NULL);
        }
        return background_cache;
    }
    static void cleanup() {
        delete background_cache;
        background_cache = nullptr;
        cache_valid = false;
    }
};
IMAGE* UICache::background_cache = nullptr;
bool UICache::cache_valid = false;

struct Level {
    string name;
    BoardType type;
    vector<vector<int>> initialState;
    string description;
};

class HiQGame {
private:
    GameState currentState;
    std::unique_ptr<Board> currentBoard;
    Position selectedPos;
    vector<Move> highlightedMoves;
    bool showAIHints;
    vector<Move> solutionSteps;
    vector<Level> levels;
    int currentLevel;
    vector<unique_ptr<Button>> buttons;
    ParticleSystem particles;
    UIAnimator moveAnimator;
    Position animatingFrom, animatingTo;
    bool isAnimatingMove = false;
    POINT lastMousePos = { 0, 0 };
    bool mouseStateChanged = true;
    bool needsRedraw = true;
    chrono::high_resolution_clock::time_point lastFrameTime;
    bool isSolving = false;
    float solveProgress = 0.0f;
    std::shared_ptr<AISolver> solver_instance;
    bool aiFoundNoSolution = false;

public:
    HiQGame();
    ~HiQGame();
    void run();

private:
    void initGraphics();
    void initializeLevels();
    void setupButtons();
    void updateAnimations(float deltaTime);
    void drawCurrentState();
    void drawMenu();
    void drawBoardSelection();
    void drawLevelSelection();
    void drawGame();
    void drawGameHighlights();
    void drawMoveAnimation();
    void drawGameInfo();
    void drawWinScreen();
    void drawLoseScreen();
    void drawRules();
    void updateAIProgress(int current_val, int max_val);
    bool processMouseEvents();
    void handleMouseDown(POINT pt);
    void handleMouseUp(POINT pt);
    void handleButtonClick(int buttonIndex);
    void handleBoardClick(int screen_x, int screen_y);
    void startNewGame(BoardType type);
    void startLevel(int levelIndex);
    void updateHighlightedMoves();
    void startAISolving();
    void interruptAI();
};

HiQGame::HiQGame() : currentState(MENU), currentBoard(nullptr), selectedPos(-1, -1), showAIHints(false), currentLevel(0), aiFoundNoSolution(false) {
    initializeLevels();
    initGraphics();
    setupButtons();
    lastFrameTime = chrono::high_resolution_clock::now();
}
HiQGame::~HiQGame() {
    if (isSolving && solver_instance) {
        solver_instance->stop();
    }
    UICache::cleanup();
    EndBatchDraw();
    closegraph();
}
void HiQGame::initGraphics() {
    initgraph(800, 600);
    setbkmode(TRANSPARENT);
    setbkcolor(RGB(245, 245, 220));
    BeginBatchDraw();
}
void HiQGame::initializeLevels() {
    Level level1;
    level1.name = "三角残局"; level1.type = TRIANGLE; level1.description = "只剩5个棋子，试试能否解决！";
    level1.initialState = { {1,-1,-1,-1,-1}, {1,1,-1,-1,-1}, {0,1,0,-1,-1}, {0,0,1,0,-1}, {0,0,0,0,0} };
    levels.push_back(level1);
    Level level2;
    level2.name = "十字困境"; level2.type = SQUARE; level2.description = "十字棋盘的经典残局";
    level2.initialState = { {-1,-1,0,1,0,-1,-1}, {-1,-1,1,1,1,-1,-1}, {0,1,1,0,1,1,0}, {1,1,0,1,0,1,1}, {0,1,1,0,1,1,0}, {-1,-1,1,1,1,-1,-1}, {-1,-1,0,1,0,-1,-1} };
    levels.push_back(level2);
}
void HiQGame::setupButtons() {
    buttons.clear();
    switch (currentState) {
    case MENU:
        buttons.push_back(make_unique<Button>(300, 200, 200, 50, L"开始游戏", RGB(255, 215, 0)));
        buttons.push_back(make_unique<Button>(300, 280, 200, 50, L"残局模式", RGB(255, 165, 0)));
        buttons.push_back(make_unique<Button>(300, 360, 200, 50, L"游戏规则", RGB(255, 140, 0)));
        buttons.push_back(make_unique<Button>(300, 440, 200, 50, L"退出游戏", RGB(255, 99, 71)));
        break;
    case BOARD_SELECT:
        buttons.push_back(make_unique<Button>(125, 150, 180, 80, L"三角形棋盘", RGB(255, 215, 0)));
        buttons.push_back(make_unique<Button>(310, 150, 180, 80, L"方形棋盘", RGB(255, 165, 0)));
        buttons.push_back(make_unique<Button>(495, 150, 180, 80, L"六边形棋盘", RGB(255, 140, 0)));
        buttons.push_back(make_unique<Button>(310, 420, 180, 50, L"返回主菜单", RGB(200, 200, 200)));
        break;
    case LEVEL_SELECT:
        for (size_t i = 0; i < levels.size(); i++) {
            wstring buttonText = L"关卡 " + to_wstring(i + 1) + L": " + StringToWstring(levels[i].name);
            COLORREF color = (i % 3 == 0) ? RGB(255, 215, 0) : (i % 3 == 1) ? RGB(255, 165, 0) : RGB(255, 140, 0);
            buttons.push_back(make_unique<Button>(250, 150 + i * 100, 300, 50, buttonText, color));
        }
        buttons.push_back(make_unique<Button>(310, 500, 180, 50, L"返回主菜单", RGB(200, 200, 200)));
        break;
    case GAME_PLAYING:
        buttons.push_back(make_unique<Button>(520, 380, 80, 30, L"撤销", RGB(255, 215, 0)));
        buttons.push_back(make_unique<Button>(620, 380, 80, 30, L"重置", RGB(255, 165, 0)));
        if (isSolving) {
            if (solver_instance && solver_instance->isPaused()) {
                buttons.push_back(make_unique<Button>(520, 420, 80, 30, L"继续", RGB(0, 220, 0)));
            }
            else {
                buttons.push_back(make_unique<Button>(520, 420, 80, 30, L"暂停", RGB(255, 165, 0)));
            }
        }
        else {
            COLORREF aiColor = showAIHints ? RGB(0, 255, 0) : RGB(200, 200, 200);
            buttons.push_back(make_unique<Button>(520, 420, 80, 30, L"AI求解", aiColor));
        }
        buttons.push_back(make_unique<Button>(620, 420, 80, 30, L"返回", RGB(255, 99, 71)));
        break;
    case GAME_WIN: case GAME_LOSE:
        buttons.push_back(make_unique<Button>(250, 340, 120, 40, L"再玩一次", RGB(255, 215, 0)));
        buttons.push_back(make_unique<Button>(430, 340, 120, 40, L"返回菜单", RGB(200, 200, 200)));
        break;
    case RULES_DISPLAY:
        buttons.push_back(make_unique<Button>(310, 520, 180, 50, L"返回主菜单", RGB(255, 215, 0)));
        break;
    }
    needsRedraw = true;
}
void HiQGame::run() {
    while (true) {
        auto currentTime = chrono::high_resolution_clock::now();
        auto deltaTime = chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        bool eventsProcessed = processMouseEvents();
        if (eventsProcessed || needsRedraw || isAnimatingMove || !particles.empty()) {
            updateAnimations(deltaTime);
            cleardevice();
            putimage(0, 0, UICache::getBackground());
            drawCurrentState();
            FlushBatchDraw();
            needsRedraw = false;
        }
        if (isSolving) { Sleep(50); }
        else { Sleep(16); }
    }
}
void HiQGame::updateAnimations(float deltaTime) {
    if (isAnimatingMove) {
        if (!moveAnimator.update()) {
            isAnimatingMove = false;
            needsRedraw = true;
        }
    }
    particles.update(deltaTime);
    for (auto& button : buttons) {
        button->update(lastMousePos);
    }
}
void HiQGame::drawCurrentState() {
    switch (currentState) {
    case MENU: drawMenu(); break;
    case BOARD_SELECT: drawBoardSelection(); break;
    case LEVEL_SELECT: drawLevelSelection(); break;
    case GAME_PLAYING: drawGame(); break;
    case GAME_WIN: drawWinScreen(); break;
    case GAME_LOSE: drawLoseScreen(); break;
    case RULES_DISPLAY: drawRules(); break;
    }
    particles.draw();
}
void HiQGame::drawMenu() {
    setfillcolor(RGB(139, 69, 19));
    fillroundrect(-10, -10, 810, 110, 20, 20);
    settextcolor(RGB(255, 215, 0));
    settextstyle(60, 0, _T("楷体"));
    outtextxy(250, 20, _T("孔明棋游戏"));
    settextcolor(RGB(255, 255, 255));
    settextstyle(24, 0, _T("楷体"));
    outtextxy(280, 120, _T("Chinese Checkers - HiQ"));
    for (auto& button : buttons) button->draw();
    setfillcolor(RGB(255, 215, 0));
    for (int i = 0; i < 5; i++) {
        fillcircle(100 + i * 30, 300 + (i % 2) * 20, 8);
        fillcircle(600 + i * 30, 300 + (i % 2) * 20, 8);
    }
}
void HiQGame::drawBoardSelection() {
    settextcolor(RGB(0, 0, 0));
    settextstyle(40, 0, _T("楷体"));
    outtextxy(280, 50, _T("选择棋盘类型"));
    for (auto& button : buttons) button->draw();
    settextstyle(20, 0, _T("楷体"));
    outtextxy(160, 240, _T("经典三角"));
    outtextxy(345, 240, _T("十字形状"));
    outtextxy(530, 240, _T("六边蜂窝"));
}
void HiQGame::drawLevelSelection() {
    settextcolor(RGB(0, 0, 0));
    settextstyle(40, 0, _T("楷体"));
    outtextxy(260, 50, _T("选择残局关卡"));
    for (auto& button : buttons) button->draw();
    settextstyle(18, 0, _T("楷体"));
    for (size_t i = 0; i < levels.size() && i < buttons.size() - 1; i++) {
        wstring desc = StringToWstring(levels[i].description);
        outtextxy(255, 205 + (long)(i * 100), desc.c_str());
    }
}
void HiQGame::drawGame() {
    if (!currentBoard) return;
    currentBoard->drawBoard(100, 150);
    drawGameHighlights();
    drawGameInfo();
    for (auto& button : buttons) button->draw();
    if (isAnimatingMove) drawMoveAnimation();
}
void HiQGame::drawGameHighlights() {
    const int highlightOffsetX = 100, highlightOffsetY = 150;
    const int selectedPegRadius = 27, targetPegRadius = 22;
    if (selectedPos.x != -1 && selectedPos.y != -1) {
        Position screenCoords = currentBoard->boardToScreen(selectedPos.x, selectedPos.y, highlightOffsetX, highlightOffsetY);
        if (screenCoords.x != -1) {
            float pulse = sin(GetTickCount() * 0.01f) * 0.5f + 0.5f;
            setcolor(RGB(255, (int)(100 * pulse), 0));
            setlinestyle(PS_SOLID, 3);
            circle(screenCoords.x, screenCoords.y, selectedPegRadius + (int)(pulse * 5));
        }
    }
    setcolor(RGB(0, 255, 0));
    setlinestyle(PS_SOLID, 2);
    for (const Move& move : highlightedMoves) {
        Position targetScreenCoords = currentBoard->boardToScreen(move.to_x, move.to_y, highlightOffsetX, highlightOffsetY);
        if (targetScreenCoords.x != -1) {
            circle(targetScreenCoords.x, targetScreenCoords.y, targetPegRadius);
            Position fromCoords = currentBoard->boardToScreen(move.from_x, move.from_y, highlightOffsetX, highlightOffsetY);
            if (fromCoords.x != -1) {
                setcolor(RGB(0, 200, 0));
                setlinestyle(PS_DOT, 1);
                line(fromCoords.x, fromCoords.y, targetScreenCoords.x, targetScreenCoords.y);
            }
        }
    }
    if (showAIHints && !solutionSteps.empty()) {
        Move nextMove = solutionSteps.front();
        Position fromScreenCoords = currentBoard->boardToScreen(nextMove.from_x, nextMove.from_y, highlightOffsetX, highlightOffsetY);
        Position toScreenCoords = currentBoard->boardToScreen(nextMove.to_x, nextMove.to_y, highlightOffsetX, highlightOffsetY);
        if (fromScreenCoords.x != -1 && toScreenCoords.x != -1) {
            setcolor(RGB(0, 0, 255));
            setlinestyle(PS_DASH, 3);
            circle(fromScreenCoords.x, fromScreenCoords.y, selectedPegRadius);
            circle(toScreenCoords.x, toScreenCoords.y, targetPegRadius);
            line(fromScreenCoords.x, fromScreenCoords.y, toScreenCoords.x, toScreenCoords.y);
        }
    }
}
void HiQGame::drawMoveAnimation() {
    if (!currentBoard || animatingFrom.x == -1 || animatingTo.x == -1) return;
    Position fromScreen = currentBoard->boardToScreen(animatingFrom.x, animatingFrom.y, 100, 150);
    Position toScreen = currentBoard->boardToScreen(animatingTo.x, animatingTo.y, 100, 150);
    if (fromScreen.x != -1 && toScreen.x != -1) {
        float progress = moveAnimator.easeOut();
        int currentX = (int)(fromScreen.x + (toScreen.x - fromScreen.x) * progress);
        int currentY = (int)(fromScreen.y + (toScreen.y - fromScreen.y) * progress);
        setfillcolor(RGB(255, 215, 0));
        fillcircle(currentX, currentY, 25);
        setcolor(RGB(184, 134, 11));
        circle(currentX, currentY, 25);
        setfillcolor(RGB(255, 215, 0));
        fillcircle(currentX - (int)((toScreen.x - fromScreen.x) * 0.1f), currentY - (int)((toScreen.y - fromScreen.y) * 0.1f), 20);
    }
}
void HiQGame::drawGameInfo() {
    setfillcolor(RGB(240, 240, 240));
    fillroundrect(500, 50, 780, 500, 10, 10);
    setcolor(RGB(0, 0, 0));
    roundrect(500, 50, 780, 500, 10, 10);
    settextcolor(RGB(0, 0, 0));
    settextstyle(28, 0, _T("楷体"));
    outtextxy(520, 70, _T("游戏信息"));
    settextstyle(20, 0, _T("楷体"));
    TCHAR pegInfo[50];
    swprintf_s(pegInfo, _T("剩余棋子: %d"), currentBoard ? currentBoard->getPegCount() : 0);
    outtextxy(520, 110, pegInfo);
    TCHAR moveInfo[50];
    swprintf_s(moveInfo, _T("可移动: %d"), currentBoard ? (int)currentBoard->getAllPossibleMoves().size() : 0);
    outtextxy(520, 140, moveInfo);
    if (currentBoard) {
        if (currentBoard->isGameWon()) { settextcolor(RGB(0, 200, 0)); outtextxy(520, 170, _T("🎉 恭喜获胜！")); }
        else if (currentBoard->isGameLost()) { settextcolor(RGB(255, 0, 0)); outtextxy(520, 170, _T("❌ 游戏失败")); }
        else { settextcolor(RGB(0, 100, 200)); outtextxy(520, 170, _T("🎮 游戏进行中")); }
    }
    settextcolor(RGB(100, 100, 100));
    settextstyle(16, 0, _T("楷体"));
    outtextxy(520, 220, _T("💡 操作说明:"));
    outtextxy(520, 240, _T("1. 点击棋子选择"));
    outtextxy(520, 260, _T("2. 点击目标位置移动"));
    if (isSolving) {
        if (solver_instance && solver_instance->isPaused()) {
            settextcolor(RGB(255, 165, 0)); settextstyle(18, 0, _T("楷体")); outtextxy(520, 330, _T("⏸️ AI 思考已暂停..."));
        }
        else {
            settextcolor(RGB(0, 150, 0)); settextstyle(18, 0, _T("楷体")); outtextxy(520, 330, _T("🤖 AI 正在求解..."));
        }
        setfillcolor(RGB(220, 220, 220)); fillroundrect(520, 350, 760, 370, 5, 5);
        setfillcolor(RGB(76, 175, 80));
        int progressWidth = (int)(240 * solveProgress);
        if (progressWidth > 0) fillroundrect(520, 350, 520 + progressWidth, 370, 5, 5);
        setcolor(RGB(100, 100, 100)); roundrect(520, 350, 760, 370, 5, 5);
        TCHAR progressText[16];
        swprintf_s(progressText, _T("%.0f%%"), solveProgress * 100);
        settextcolor(RGB(0, 0, 0)); settextstyle(16, 0, _T("Arial")); outtextxy(630, 352, progressText);
    }
    else if (showAIHints) {
        settextcolor(RGB(0, 100, 255)); settextstyle(16, 0, _T("楷体")); outtextxy(520, 330, _T("🎯 AI解法：请按提示移动"));
    }
    else if (aiFoundNoSolution) {
        settextcolor(RGB(255, 0, 0)); settextstyle(18, 0, _T("楷体")); outtextxy(520, 330, _T("⚠️ AI判断当前局面无解！"));
    }
}
void HiQGame::drawWinScreen() {
    setfillcolor(RGB(220, 255, 220)); fillroundrect(200, 200, 600, 400, 20, 20);
    setcolor(RGB(0, 150, 0)); setlinestyle(PS_SOLID, 3); roundrect(200, 200, 600, 400, 20, 20);
    settextcolor(RGB(0, 150, 0)); settextstyle(52, 0, _T("楷体")); outtextxy(260, 240, _T("🎉 恭喜获胜！"));
    settextstyle(28, 0, _T("楷体"));
    TCHAR winInfo[100];
    swprintf_s(winInfo, _T("最终剩余棋子: %d"), currentBoard ? currentBoard->getPegCount() : 0);
    outtextxy(280, 300, winInfo);
    for (auto& button : buttons) button->draw();
}
void HiQGame::drawLoseScreen() {
    setfillcolor(RGB(255, 220, 220)); fillroundrect(200, 200, 600, 400, 20, 20);
    setcolor(RGB(200, 0, 0)); setlinestyle(PS_SOLID, 3); roundrect(200, 200, 600, 400, 20, 20);
    settextcolor(RGB(200, 0, 0)); settextstyle(52, 0, _T("楷体")); outtextxy(280, 240, _T("❌ 游戏失败"));
    settextstyle(28, 0, _T("楷体"));
    TCHAR loseInfo[100];
    swprintf_s(loseInfo, _T("剩余棋子: %d"), currentBoard ? currentBoard->getPegCount() : 0);
    outtextxy(320, 300, loseInfo);
    for (auto& button : buttons) button->draw();
}
void HiQGame::drawRules() {
    settextcolor(RGB(0, 0, 0)); settextstyle(40, 0, _T("楷体")); outtextxy(300, 30, _T("📖 游戏规则"));
    settextstyle(24, 0, _T("楷体")); outtextxy(50, 100, _T("孔明棋（HiQ）游戏规则："));
    settextstyle(20, 0, _T("楷体"));
    outtextxy(50, 140, _T("🎯 1. 目标：通过跳跃移动，使棋盘上剩余棋子越少越好"));
    outtextxy(50, 170, _T("🚫 2. 移动规则：棋子只能跳跃移动，不能直接移动到相邻位置"));
    outtextxy(50, 200, _T("⭐ 3. 跳跃规则：必须跳过一个相邻的棋子到达空位"));
    outtextxy(50, 230, _T("❌ 4. 消除规则：被跳过的棋子会从棋盘上移除"));
    outtextxy(50, 260, _T("🏆 5. 胜利条件：最理想情况是只剩下1个棋子"));
    outtextxy(50, 290, _T("💀 6. 失败条件：无法继续移动且剩余棋子数>1"));
    outtextxy(50, 340, _T("✨ 游戏特色："));
    outtextxy(50, 370, _T("• 三种棋盘类型：三角形、方形十字、六边形"));
    outtextxy(50, 400, _T("• 🤖 AI智能求解：显示通关路径"));
    outtextxy(50, 430, _T("• 🎮 残局挑战：预设的困难关卡"));
    for (auto& button : buttons) button->draw();
}
void HiQGame::updateAIProgress(int current_val, int max_val) {
    if (max_val > 0) solveProgress = (std::min)(1.0f, (float)current_val / max_val);
    else if (current_val == 1 && max_val == 1) solveProgress = 1.0f;
    else solveProgress = 0.0f;
    needsRedraw = true;
}
bool HiQGame::processMouseEvents() {
    ExMessage mouseMsg;
    bool eventsProcessed = false;
    while (peekmessage(&mouseMsg, EM_MOUSE, true)) {
        eventsProcessed = true;
        POINT pt = { mouseMsg.x, mouseMsg.y };
        if (pt.x != lastMousePos.x || pt.y != lastMousePos.y) {
            lastMousePos = pt;
            mouseStateChanged = true;
            needsRedraw = true;
        }
        if (mouseMsg.message == WM_LBUTTONDOWN) handleMouseDown(pt);
        else if (mouseMsg.message == WM_LBUTTONUP) handleMouseUp(pt);
    }
    return eventsProcessed;
}
void HiQGame::handleMouseDown(POINT pt) {
    for (auto& button : buttons) {
        if (button->contains(pt)) {
            button->setState(BTN_PRESSED);
            needsRedraw = true;
            return;
        }
    }
    if (currentState == GAME_PLAYING) {
        Position clickedPos = currentBoard->screenToBoard(pt.x, pt.y, 100, 150);
        if (clickedPos.x != -1) {
            particles.addBurst(pt.x, pt.y, RGB(255, 215, 0), 5);
            needsRedraw = true;
        }
    }
}
void HiQGame::handleMouseUp(POINT pt) {
    bool buttonClicked = false;
    for (int i = 0; i < (int)buttons.size(); ++i) {
        auto& button = buttons[i];
        if (button->state == BTN_PRESSED) {
            if (button->contains(pt)) {
                handleButtonClick(i);
                buttonClicked = true;
                RECT rect = button->rect;
                int centerX = (rect.left + rect.right) / 2;
                int centerY = (rect.top + rect.bottom) / 2;
                particles.addBurst(centerX, centerY, button->normal_color, 8);
            }
            button->setState(button->contains(pt) ? BTN_HOVER : BTN_NORMAL);
            needsRedraw = true;
        }
    }
    if (!buttonClicked && currentState == GAME_PLAYING) {
        handleBoardClick(pt.x, pt.y);
    }
}

void HiQGame::interruptAI() {
    if (isSolving && solver_instance) {
        cout << "User interrupted AI search." << endl;
        solver_instance->stop();
        solver_instance.reset();
    }
    isSolving = false;
    showAIHints = false;
    aiFoundNoSolution = false;
    setupButtons();
    needsRedraw = true;
}

void HiQGame::handleButtonClick(int buttonIndex) {
    PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
    switch (currentState) {
    case MENU:
        switch (buttonIndex) {
        case 0: currentState = BOARD_SELECT; break;
        case 1: currentState = LEVEL_SELECT; break;
        case 2: currentState = RULES_DISPLAY; break;
        case 3: exit(0); break;
        }
        break;
    case BOARD_SELECT:
        switch (buttonIndex) {
        case 0: startNewGame(TRIANGLE); return;
        case 1: startNewGame(SQUARE); return;
        case 2: startNewGame(HEXAGON); return;
        case 3: currentState = MENU; break;
        }
        break;
    case LEVEL_SELECT:
        if (buttonIndex < (int)levels.size()) {
            startLevel(buttonIndex);
            return;
        }
        else {
            currentState = MENU;
        }
        break;
    case GAME_PLAYING:
        if (buttonIndex == 2) {
            if (isSolving) {
                if (solver_instance) {
                    if (solver_instance->isPaused()) solver_instance->resume();
                    else solver_instance->pause();
                }
                setupButtons();
            }
            else {
                startAISolving();
            }
        }
        else {
            if (isSolving) {
                interruptAI();
            }
            switch (buttonIndex) {
            case 0:
                if (currentBoard->undoMove()) { selectedPos = { -1, -1 }; highlightedMoves.clear(); solutionSteps.clear(); showAIHints = false; aiFoundNoSolution = false; needsRedraw = true; }
                break;
            case 1:
                currentBoard->resetBoard(); selectedPos = { -1, -1 }; highlightedMoves.clear(); solutionSteps.clear(); showAIHints = false; aiFoundNoSolution = false; needsRedraw = true;
                break;
            case 3:
                currentState = MENU; if (currentBoard) currentBoard->resetBoard(); solutionSteps.clear(); showAIHints = false; aiFoundNoSolution = false;
                setupButtons();
                return;
            }
        }
        return;
    case GAME_WIN: case GAME_LOSE:
        switch (buttonIndex) {
        case 0: if (currentBoard) currentBoard->resetBoard(); solutionSteps.clear(); showAIHints = false; aiFoundNoSolution = false; currentState = GAME_PLAYING; break;
        case 1: currentState = MENU; break;
        }
        break;
    case RULES_DISPLAY:
        currentState = MENU;
        break;
    }
    setupButtons();
}
void HiQGame::startAISolving() {
    isSolving = true;
    solveProgress = 0.0f;
    aiFoundNoSolution = false;
    setupButtons();
    if (currentBoard) {
        solver_instance = std::make_shared<AISolver>(currentBoard.get(), 1);
        thread([this]() {
            auto progress_callback = [this](int cur, int max) { this->updateAIProgress(cur, max); };
            solutionSteps = solver_instance->findSolution(progress_callback);
            bool timed_out = solver_instance->hasTimedOut();
            isSolving = false;
            if (solutionSteps.empty() && !timed_out) {
                showAIHints = false;
                aiFoundNoSolution = true;
            }
            else {
                showAIHints = !solutionSteps.empty();
                aiFoundNoSolution = false;
            }
            solver_instance.reset();
            setupButtons();
            needsRedraw = true;
            }).detach();
    }
    else {
        isSolving = false;
        setupButtons();
    }
}
void HiQGame::handleBoardClick(int screen_x, int screen_y) {
    if (!currentBoard) return;

    if (isSolving) {
        interruptAI();
        return;
    }

    aiFoundNoSolution = false;
    Position clickedBoardPos = currentBoard->screenToBoard(screen_x, screen_y, 100, 150);
    if (clickedBoardPos.x == -1) {
        selectedPos = { -1, -1 };
        highlightedMoves.clear();
        needsRedraw = true;
        return;
    }
    if (selectedPos.x != -1) {
        bool moved = false;
        for (const Move& legalMove : highlightedMoves) {
            if (legalMove.to_x == clickedBoardPos.x && legalMove.to_y == clickedBoardPos.y) {
                animatingFrom = selectedPos;
                animatingTo = Position(legalMove.to_x, legalMove.to_y);
                isAnimatingMove = true;
                moveAnimator.start(300);
                PlaySound(TEXT("SystemAsterisk"), NULL, SND_ALIAS | SND_ASYNC);
                Position toScreen = currentBoard->boardToScreen(legalMove.to_x, legalMove.to_y, 100, 150);
                if (toScreen.x != -1) particles.addBurst(toScreen.x, toScreen.y, RGB(0, 255, 0), 12);
                currentBoard->makeMove(legalMove);
                if (!solutionSteps.empty()) {
                    Move nextSol = solutionSteps.front();
                    if (legalMove.from_x == nextSol.from_x && legalMove.from_y == nextSol.from_y &&
                        legalMove.to_x == nextSol.to_x && legalMove.to_y == nextSol.to_y) {
                        solutionSteps.erase(solutionSteps.begin());
                    }
                    else {
                        solutionSteps.clear();
                        showAIHints = false;
                    }
                }
                moved = true;
                break;
            }
        }
        selectedPos = { -1, -1 };
        highlightedMoves.clear();
        if (!moved && currentBoard->getPeg(clickedBoardPos.x, clickedBoardPos.y) == 1) {
            selectedPos = clickedBoardPos;
            updateHighlightedMoves();
        }
    }
    else {
        if (currentBoard->getPeg(clickedBoardPos.x, clickedBoardPos.y) == 1) {
            selectedPos = clickedBoardPos;
            updateHighlightedMoves();
        }
    }
    if (currentBoard->isGameWon()) {
        currentState = GAME_WIN;
        setupButtons();
        particles.addBurst(400, 300, RGB(255, 215, 0), 20);
        PlaySound(TEXT("SystemExit"), NULL, SND_ALIAS | SND_ASYNC);
    }
    else if (currentBoard->isGameLost()) {
        currentState = GAME_LOSE;
        setupButtons();
        PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
    }
    needsRedraw = true;
}
void HiQGame::startNewGame(BoardType type) {
    switch (type) {
    case TRIANGLE: currentBoard = std::make_unique<TriangleBoard>(); break;
    case SQUARE: currentBoard = std::make_unique<SquareBoard>(); break;
    case HEXAGON: currentBoard = std::make_unique<HexagonBoard>(); break;
    }
    selectedPos = { -1, -1 };
    highlightedMoves.clear();
    showAIHints = false;
    solutionSteps.clear();
    aiFoundNoSolution = false;
    currentState = GAME_PLAYING;
    setupButtons();
    needsRedraw = true;
}
void HiQGame::startLevel(int levelIndex) {
    if (levelIndex < 0 || levelIndex >= (int)levels.size()) return;
    Level& level = levels[levelIndex];
    switch (level.type) {
    case TRIANGLE: currentBoard = std::make_unique<TriangleBoard>(); break;
    case SQUARE: currentBoard = std::make_unique<SquareBoard>(); break;
    case HEXAGON: currentBoard = std::make_unique<HexagonBoard>(); break;
    }
    if (currentBoard) {
        currentBoard->resetBoard();
        for (size_t r = 0; r < level.initialState.size(); r++) {
            for (size_t c = 0; c < level.initialState[r].size(); c++) {
                if (r < (size_t)currentBoard->getHeight() && c < (size_t)currentBoard->getWidth()) {
                    if (level.initialState[r][c] != -2) {
                        if (currentBoard->isValidPosition((int)c, (int)r)) {
                            currentBoard->setPeg((int)c, (int)r, level.initialState[r][c]);
                        }
                    }
                }
            }
        }
        currentBoard->clearBoardHistory();
        currentBoard->addToBoardHistory(currentBoard->getGrid());
    }
    selectedPos = { -1, -1 };
    highlightedMoves.clear();
    showAIHints = false;
    solutionSteps.clear();
    aiFoundNoSolution = false;
    currentLevel = levelIndex;
    currentState = GAME_PLAYING;
    setupButtons();
    needsRedraw = true;
}
void HiQGame::updateHighlightedMoves() {
    highlightedMoves.clear();
    if (selectedPos.x == -1 || !currentBoard) return;
    vector<Move> allMoves = currentBoard->getAllPossibleMoves();
    for (const Move& move : allMoves) {
        if (move.from_x == selectedPos.x && move.from_y == selectedPos.y) {
            highlightedMoves.push_back(move);
        }
    }
    needsRedraw = true;
}


int main() {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);

    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IONBF, 0);

    cout << "HiQ Game Console - Optimized UI Version (Modular)" << endl;
    cout << "Features: Fast response, smooth animations, better usability, fixed AI logic, no-solution-found message" << endl;
    try {
        HiQGame game;
        game.run();
    }
    catch (const exception& e) {
        MessageBox(NULL, _T("Game runtime error"), _T("Error"), MB_OK);
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    catch (...) {
        MessageBox(NULL, _T("An unknown game runtime error occurred."), _T("Error"), MB_OK);
        cerr << "Unknown exception caught." << endl;
        return -1;
    }
    return 0;
}
