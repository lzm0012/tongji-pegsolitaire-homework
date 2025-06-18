// board.h
#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <string>
#include <cmath>
#include <memory> // [MODIFIED] Added for std::unique_ptr

// Forward-declare the Move struct, as Board methods use it
struct Move;

// Struct for a position on the board
struct Position {
    int x, y;
    Position(int px = -1, int py = -1) : x(px), y(py) {}
    bool operator==(const Position& other) const { return x == other.x && y == other.y; }
};

// Abstract base class Board
class Board {
protected:
    int width, height;
    std::vector<std::vector<int>> grid;
    std::vector<Move> moveHistory;
    std::vector<std::vector<std::vector<int>>> boardHistory;

public:
    Board(int w, int h);
    virtual ~Board();

    // Pure virtual functions, must be implemented by derived classes
    virtual void initializeBoard() = 0;
    virtual bool isValidPosition(int x, int y) const = 0;
    virtual std::vector<Move> getAllPossibleMoves() const = 0;
    virtual void drawBoard(int offsetX = 100, int offsetY = 150) const = 0;
    virtual Position screenToBoard(int screenX, int screenY, int offsetX = 100, int offsetY = 150) const = 0;
    virtual Position boardToScreen(int boardX, int boardY, int offsetX = 100, int offsetY = 150) const = 0;

    // [MODIFIED] The return type of clone() is now std::unique_ptr<Board>
    virtual std::unique_ptr<Board> clone() const = 0;

    // Common methods implemented in the base class
    void clearBoardHistory();
    void addToBoardHistory(const std::vector<std::vector<int>>& state);
    const std::vector<std::vector<int>>& getGrid() const;
    std::string getStateHash() const;
    bool makeMove(const Move& move);
    bool undoMove();
    void resetBoard();
    bool isValidMove(const Move& move) const;
    int getPegCount() const;
    bool isGameWon() const;
    bool isGameLost() const;
    void setPeg(int x, int y, int value);
    int getPeg(int x, int y) const;
    int getWidth() const;
    int getHeight() const;
};

// TriangleBoard class declaration
class TriangleBoard : public Board {
public:
    TriangleBoard();
    void initializeBoard() override;
    bool isValidPosition(int x, int y) const override;
    std::vector<Move> getAllPossibleMoves() const override;
    void drawBoard(int offsetX = 100, int offsetY = 150) const override;
    Position screenToBoard(int screenX, int screenY, int offsetX = 100, int offsetY = 150) const override;
    Position boardToScreen(int boardX, int boardY, int offsetX = 100, int offsetY = 150) const override;

    // [MODIFIED] The override matches the base class change
    std::unique_ptr<Board> clone() const override;
};

// SquareBoard class declaration
class SquareBoard : public Board {
public:
    SquareBoard();
    void initializeBoard() override;
    bool isValidPosition(int x, int y) const override;
    std::vector<Move> getAllPossibleMoves() const override;
    void drawBoard(int offsetX = 100, int offsetY = 150) const override;
    Position screenToBoard(int screenX, int screenY, int offsetX = 100, int offsetY = 150) const override;
    Position boardToScreen(int boardX, int boardY, int offsetX = 100, int offsetY = 150) const override;

    // [MODIFIED] The override matches the base class change
    std::unique_ptr<Board> clone() const override;
};

// HexagonBoard class declaration
class HexagonBoard : public Board {
public:
    HexagonBoard();
    void initializeBoard() override;
    bool isValidPosition(int x, int y) const override;
    std::vector<Move> getAllPossibleMoves() const override;
    void drawBoard(int offsetX = 100, int offsetY = 150) const override;
    Position screenToBoard(int screenX, int screenY, int offsetX = 100, int offsetY = 150) const override;
    Position boardToScreen(int boardX, int boardY, int offsetX = 100, int offsetY = 150) const override;

    // [MODIFIED] The override matches the base class change
    std::unique_ptr<Board> clone() const override;
};

#endif // BOARD_H