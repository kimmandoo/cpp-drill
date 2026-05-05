#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int main(){
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    constexpr int loopCount = 1000;
    std::vector<int> counters(num_threads, 0); // 각 스레드마다 카운터를 저장할 벡터

    for(int i=0; i<num_threads; i++){
        threads.emplace_back([&, i]{
            int local_counter = 0; // 각 스레드마다 로컬 카운터
            for(int j=0; j<loopCount; j++){
                local_counter++; // 로컬 카운터 증가
            }
            counters[i] = local_counter; // 최종 로컬 카운터 값을 벡터에 저장
        });
    }

    for(auto& t : threads){
        t.join();
    }

    int total = 0;

    for(int i=0; i<num_threads; i++){
        total += counters[i]; // 각 스레드의 로컬 카운터 값을 합산
    }
    std::cout << "Total: " << total << "\n";

    return 0;
}