#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

std::queue<int> tasks;
std::mutex mtx;

void worker(int workerId) {
    while (true) {
        int task;

        {
            std::lock_guard<std::mutex> lock(mtx);

            if (tasks.empty()) {
                break;
            }

            task = tasks.front();
            tasks.pop();
        }

        std::cout << "worker " << workerId
                  << " processed task " << task << std::endl;
    }
}

int main() {
    for (int i = 1; i <= 20; i++) {
        tasks.push(i);
    }

    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++) {
        workers.emplace_back(worker, i);
    }

    for (auto& t : workers) {
        t.join();
    }

    return 0;
}