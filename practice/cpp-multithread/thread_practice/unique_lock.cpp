#include <iostream>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

std::queue<int> q;
std::mutex mtx;
std::condition_variable cv;
bool is_finished = false;

void prod(){
    for(int i=1; i<=5; i++){
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push(i); 
        }

        cv.notify_one();
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        is_finished = true;
        // 임계구역으로 보호하기
    }

    cv.notify_all();
}

void consume(){
    while(true){
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, []{
            //q가 비어 있지 않거나 finished가 true가 될 때까지 기다림
            // 큐가 비어있지 않거나, 작업이 종료되었을 때 이 쓰레드를 재우고, 조건 충족하면 깨움
            return !q.empty() || is_finished; 
        });

        if(q.empty() && is_finished){
            // is_finished가 true면 생산자에서는 다 끝난 것인데 큐가 차있으면 cout 해야됨
            break;
        }

        int value = q.front();
        q.pop();
        lock.unlock(); // 위에서 선언한 lock

        std::cout << "consume: " << value << "\n";
    }

}

int main(){
    std::thread t1(prod); // 생산 스레드
    std::thread t2(consume); // 소비 스레드

    t1.join();
    t2.join();

    std::cout << std::thread::hardware_concurrency() << std::endl;
    
    return 0;
}