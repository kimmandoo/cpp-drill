#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int main(){
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    int counter = 0;
    std::mutex mtx;

    for(int i=0; i<num_threads; i++){
        threads.emplace_back([&, i]{
            std::lock_guard<std::mutex> lock(mtx);
            counter++;
        });
    }

    for(auto& t : threads){
        t.join();
    }

    std::cout << "Total: " << counter << "\n";

    return 0;
}