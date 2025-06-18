// ai_solver.h
#ifndef AI_SOLVER_H
#define AI_SOLVER_H
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include "board.h" 
// ... (Move 结构体和 ProgressCallback 类型定义保持不变) ...
struct Move {
    int from_x, from_y, over_x, over_y, to_x, to_y;
    Move() : from_x(-1), from_y(-1), over_x(-1), over_y(-1), to_x(-1), to_y(-1) {}
    Move(int fx, int fy, int ox, int oy, int tx, int ty) : from_x(fx), from_y(fy), over_x(ox), over_y(oy), to_x(tx), to_y(ty) {}
};
using ProgressCallback = std::function<void(int current_cost, int max_possible_cost)>;
extern std::map<std::string, std::vector<Move>> solutionCache;
// AI 求解器类
class AISolver {
private:
    Board* initialBoard;
    int max_pegs_to_solve;
    const int FOUND = -1;
    std::atomic<bool> global_solution_found;
    std::atomic<bool> is_paused;
    std::mutex pause_mutex;
    std::condition_variable pause_cond;
    std::atomic<bool> timed_out;
    std::atomic<bool> force_stop; // [ADDED] 添加强制停止标志
    std::atomic<int> best_solution_depth;
    std::mutex solution_path_mutex;
    std::vector<Move> final_solution_path;
    std::chrono::time_point<std::chrono::high_resolution_clock> search_start_time;
    const long long time_limit_ms = 600000; // 10分钟超时
    int calculateHeuristic(const Board* board);
    int search_task(Board* board, int g_cost, int threshold,
        std::vector<Move>& partialSolution,
        std::unordered_map<std::string, int>& transpositionTable,
        std::unordered_map<std::string, int>& heuristicCache);
    int threshold_worker(int initial_threshold, int step, ProgressCallback onProgress, int max_depth_estimate);
public:
    AISolver(Board* board, int target_pegs = 1);
    void pause();
    void resume();
    void stop(); // [ADDED] 添加停止方法
    bool isPaused() const;
    bool hasTimedOut() const;
    std::vector<Move> findSolution(ProgressCallback onProgress = nullptr);
};
#endif // AI_SOLVER_H