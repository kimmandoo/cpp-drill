#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int counter = 0;
std::mutex mtx;

void inc(){
    for(int i=0; i<10000; i++){
        // mtx.lock();
        // counter++;
        // mtx.unlock();
        std::lock_guard<std::mutex> lock(mtx);
        //중간에 return, 예외, 에러가 있어도 블록을 벗어나면 자동으로 unlock
        counter++;
    }
}

int main(){
    std::vector<std::thread> t;

    for(int i=0; i< 10; i++){
        t.emplace_back(inc);
    }

    for(auto& t: t){
        t.join();
    }

    std::cout << counter << "\n";

    return 0;
}