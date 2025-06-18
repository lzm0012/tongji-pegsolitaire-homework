#include "ai_solver.h"
#include <iostream>
#include <queue>
#include <thread>
#include <future>
#include <algorithm>
#include <memory> // [MODIFIED] Include for std::unique_ptr

using namespace std;

// AISolver 方法的实现
AISolver::AISolver(Board* board, int target_pegs)
    : initialBoard(board), max_pegs_to_solve(target_pegs),
    global_solution_found(false), is_paused(false), timed_out(false),
    force_stop(false),
    best_solution_depth(INT_MAX) {
}

void AISolver::pause() { is_paused = true; cout << "AI search paused." << endl; }
void AISolver::resume() { is_paused = false; cout << "AI search resumed." << endl; pause_cond.notify_all(); }
void AISolver::stop() {
    force_stop = true;
    cout << "AI search stopping." << endl;
    if (is_paused.load()) {
        resume();
    }
}
bool AISolver::isPaused() const { return is_paused.load(); }
bool AISolver::hasTimedOut() const { return timed_out.load(); }

int AISolver::calculateHeuristic(const Board* board) {
    int islands = 0;
    vector<vector<bool>> visited(board->getHeight(), vector<bool>(board->getWidth(), false));
    for (int y = 0; y < board->getHeight(); ++y) {
        for (int x = 0; x < board->getWidth(); ++x) {
            if (board->getPeg(x, y) == 1 && !visited[y][x]) {
                islands++;
                queue<Position> q;
                q.push({ x, y });
                visited[y][x] = true;
                while (!q.empty()) {
                    Position current = q.front(); q.pop();
                    int dx[] = { -1, 1, 0, 0, -1, -1, 1, 1, -2, 2, 0, 0, -2, -2, 2, 2 };
                    int dy[] = { 0, 0, -1, 1, -1, 1, -1, 1,  0, 0, -2, 2, -2, 2, -2, 2 };
                    for (int i = 0; i < 16; ++i) {
                        int nx = current.x + dx[i], ny = current.y + dy[i];
                        if (board->isValidPosition(nx, ny) && board->getPeg(nx, ny) == 1 && !visited[ny][nx]) {
                            visited[ny][nx] = true; q.push({ nx, ny });
                        }
                    }
                }
            }
        }
    }
    return islands > 0 ? islands - 1 : 0;
}

int AISolver::search_task(Board* board, int g_cost, int threshold,
    vector<Move>& partialSolution,
    unordered_map<string, int>& transpositionTable,
    unordered_map<string, int>& heuristicCache) {

    if (force_stop.load()) return INT_MAX;

    if (is_paused.load()) {
        std::unique_lock<std::mutex> lock(pause_mutex);
        pause_cond.wait(lock, [this] { return !is_paused.load() || force_stop.load(); });
    }

    if (timed_out.load() || global_solution_found.load() || force_stop.load()) return INT_MAX;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - search_start_time).count() > time_limit_ms) {
        timed_out = true;
        return INT_MAX;
    }
    if (g_cost >= best_solution_depth) {
        return INT_MAX;
    }

    string hash = board->getStateHash();
    int h_cost;
    auto cache_it = heuristicCache.find(hash);
    if (cache_it != heuristicCache.end()) h_cost = cache_it->second;
    else { h_cost = calculateHeuristic(board); heuristicCache[hash] = h_cost; }

    int f_cost = g_cost + h_cost;
    if (f_cost > threshold) return f_cost;

    auto tt_it = transpositionTable.find(hash);
    if (tt_it != transpositionTable.end() && tt_it->second <= f_cost) return tt_it->second;

    if (board->getPegCount() <= max_pegs_to_solve) {
        int current_best = best_solution_depth.load(std::memory_order_relaxed);
        while (g_cost < current_best) {
            if (best_solution_depth.compare_exchange_weak(current_best, g_cost, std::memory_order_release, std::memory_order_relaxed)) {
                break;
            }
        }
        return FOUND;
    }

    int min_surplus = INT_MAX;
    vector<Move> possibleMoves = board->getAllPossibleMoves();
    for (const Move& move : possibleMoves) {
        board->makeMove(move);
        int result = search_task(board, g_cost + 1, threshold, partialSolution, transpositionTable, heuristicCache);
        board->undoMove();

        if (force_stop.load()) return INT_MAX;

        if (result == FOUND) {
            partialSolution.insert(partialSolution.begin(), move); return FOUND;
        }
        if (result < min_surplus) min_surplus = result;
    }

    transpositionTable[hash] = min_surplus;
    return min_surplus;
}

int AISolver::threshold_worker(int initial_threshold, int step, ProgressCallback onProgress, int max_depth_estimate) {
    int threshold = initial_threshold;
    while (!global_solution_found.load() && !timed_out.load() && !force_stop.load()) {
        if (threshold > max_depth_estimate + 2) {
            cout << "Search depth exceeded maximum estimate. No solution likely." << endl;
            return -1;
        }

        if (onProgress) {
            onProgress(threshold, max_depth_estimate);
        }

        std::atomic<int> next_threshold_local = INT_MAX;

        vector<Move> rootMoves = initialBoard->getAllPossibleMoves();
        if (rootMoves.empty()) return -1;

        vector<future<void>> futures;
        for (const auto& rootMove : rootMoves) {
            futures.push_back(std::async(std::launch::async, [this, rootMove, threshold, &next_threshold_local]() {
                if (this->global_solution_found.load() || this->timed_out.load() || this->force_stop.load()) return;

                std::unique_ptr<Board> boardCopy = this->initialBoard->clone();
                if (!boardCopy) return;

                boardCopy->makeMove(rootMove);
                vector<Move> partialSolution;
                unordered_map<string, int> tt, hc;
                int result = this->search_task(boardCopy.get(), 1, threshold, partialSolution, tt, hc);
                if (result == this->FOUND) {
                    std::lock_guard<std::mutex> lock(this->solution_path_mutex);
                    if (this->force_stop.load()) return;

                    int new_solution_depth = (int)partialSolution.size() + 1;
                    int current_best_depth = this->best_solution_depth.load();

                    if (new_solution_depth <= current_best_depth) {
                        this->final_solution_path = partialSolution;
                        this->final_solution_path.insert(this->final_solution_path.begin(), rootMove);
                        this->global_solution_found = true;
                    }
                }
                else if (result < INT_MAX) {
                    int current_min = next_threshold_local.load();
                    while (result < current_min) {
                        if (next_threshold_local.compare_exchange_weak(current_min, result)) break;
                    }
                }
                }));
        }
        for (auto& f : futures) f.get();

        if (global_solution_found.load()) {
            return best_solution_depth.load();
        }

        if (timed_out.load() || force_stop.load()) return -1;

        int next_t = next_threshold_local.load();
        if (next_t == INT_MAX) {
            return -1;
        }

        threshold = (std::max)(threshold + step, next_t);
    }
    return -1;
}

vector<Move> AISolver::findSolution(ProgressCallback onProgress) {
    cout << "Starting AI solver with advanced parallel search..." << endl;

    string initialHash = initialBoard->getStateHash();
    if (solutionCache.count(initialHash)) {
        cout << "Solution found in cache!" << endl;
        if (onProgress) onProgress(1, 1);
        return solutionCache[initialHash];
    }

    global_solution_found = false;
    timed_out = false;
    force_stop = false;
    final_solution_path.clear();
    best_solution_depth = INT_MAX;

    search_start_time = std::chrono::high_resolution_clock::now();
    int base_threshold = calculateHeuristic(initialBoard);

    int max_depth_estimate = initialBoard->getPegCount() - 1;
    int num_supervisor_threads = (std::max)(1u, std::thread::hardware_concurrency() / 2);

    vector<future<int>> supervisor_futures;
    for (int i = 0; i < num_supervisor_threads; ++i) {
        supervisor_futures.push_back(std::async(std::launch::async, &AISolver::threshold_worker, this, base_threshold + i, num_supervisor_threads, onProgress, max_depth_estimate));
    }

    for (auto& f : supervisor_futures) f.get();

    if (global_solution_found.load()) {
        cout << "Optimal solution found with depth: " << final_solution_path.size() << endl;
        solutionCache[initialHash] = final_solution_path;
        if (onProgress) onProgress(1, 1);
        return final_solution_path;
    }

    if (timed_out.load()) cout << "AI Search Timed Out!" << endl;
    else if (force_stop.load()) cout << "AI Search was interrupted by user." << endl;
    else cout << "No solution found." << endl;
    if (onProgress) onProgress(1, 1);
    return {};
}