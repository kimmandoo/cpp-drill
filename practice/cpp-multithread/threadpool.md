# ThreadPool?

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this, i] {
                while (true) {
                    int task;
                    {
                        // 잠들 준비 unique_lock이 필요함
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // 작업이 있거나 종료 신호가 올 때까지 대기 (조건 변수)
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });

                        // 종료 조건 시 퇴근
                        if (this->stop && this->tasks.empty()) return;

                        task = tasks.front();
                        tasks.pop();
                    }

                    // 작업 수행 및 안전한 출력
                    // std::lock_guard를 한 번 더 써서 출력 꼬임을 방지
                    static std::mutex cout_mutex;
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "[Worker " << i << "] processed task " << task << "\n";
                    }
                }
            });
        }
    }

    void enqueue(int task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(task);
        }
        condition.notify_one(); // 잠자고 있는 워커 중 하나를 깨움
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all(); // 모든 워커를 깨워서 종료시킴
        for (std::thread &worker : workers) worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<int> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

int main() {
    ThreadPool pool(4); // 4개의 워커 스레드 생성

    for (int i = 1; i <= 20; ++i) {
        pool.enqueue(i);
    }

    // 소멸자가 호출되면서 모든 작업 완료 후 안전하게 종료됨
    return 0;
}
```

ThreadPool을 만들어서 사용하는 방식으로 Producer-Consumer패턴이다.

```text
Producer Thread
    ↓
Shared Queue
    ↓
Consumer Worker Threads
```

스레드를 생성할때,os에서는 스택메모리도 잡고, 스케줄링 대상도 늘어나고 프로세스보단 덜하지만 컨텍스트 스위칭 비용이  생기긴한다.

스레드풀은 스레드를 재사용하는 구조로 미리 스레드들을 만들어두고 놀고있던 스레드가 큐에서 작업을 꺼내 처리하는 형태로 쓴다.

작업이 없으면 스레드를 재우고, 작업이 들어오면 notify해서 잠든 스레드를 깨워 일을 시키는 방식이다.

워커 스레드는 한 번만 일하고 끝나는 게 아니라, 계속 생존해있다가 조건 변수를 보고 작업있는지확인->없으면 잠들기->작업들어오면 일어남-> 작업처리 이 과정을 반복하고있다.

람다 [this, i]는 그럼 왜 쓸까

```cpp
workers.emplace_back([this, i] {
    ...
});
```

여기는 새 스레드가 실행할 함수를 람다로 넘긴 것이다. this로 현재 클래스 멤버에 접근할 수 있도록 하고, i는 워커번호를 출력하기 위함이다.

근데 i에 &i이렇게 들어가만 같은 원본을 alias로 들고있게 되니까 출력이 이상해질 수 있다.

unique_lock은 그럼 뭘까

lock_gurad랑 unique_lock 둘다 스코프안에서 mutex 잠그고 스코프 벗어나면 풀어주는 역할 = RAII 방식인데, 차이가 좀 있다.

lock_guard는 중간에 수동 unlock안되고, condition_variable wait에 쓸 수 없다. 

```cpp
void enqueue(int task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    condition.notify_one();
}
```

그래서 enqueue정도 처럼 짧고 단순한 보호가 필요할 경우 쓴다.

unique_lock은 좀 더 유연하다. 

```cpp
std::unique_lock<std::mutex> lock(this->queue_mutex);
```

중간에 unlock가능하고, unlock했다가 다시 lock걸수도 있고, condition_variable wait에도 사용가능하다.

condition_variable::wait() 이 unique_lock만을 요구하는 이유는, 잠깐 unlock했다가 lock해야되는 구조기 때문이다.

```cpp
std::unique_lock<std::mutex> lock(this->queue_mutex);

this->condition.wait(lock, [this] {
    return this->stop || !this->tasks.empty();
});
```

1. mutex를 잡은 상태로 조건 확인
2. 조건이 안 맞으면 mutex를 풀고 잠듦
3. notify를 받으면 깨어남
4. 다시 mutex를 잡음
5. 조건을 다시 확인

condition_variable는 그러면 뭐냐

워커 스레드는 작업이 없을 때 계속 확인하면 안 된다.

그래서 wait을 쓰면 stop이 true이거나, tasks가 비어 있지 않을 때까지, 현재 스레드를 재워두라는 의미가 된다.

wait부분만 좀 쉽게 풀어쓰면

```cpp
while (!(this->stop || !this->tasks.empty())) {
    condition.wait(lock);
}
```
이건데, stop은 현재 클래스를 종료시킬 경우, 다른 조건은 처리할 작업이 있는 경우다. 둘 중 하나라도 true면 워커가 깨어난다.

조건 검사를 람다로 하는 이유??

condition_variable에서 스레드가 notify_one()이나 notify_all() 없이도 깨어날 수 있다.

그래서 그냥 `condition.wait(lock);`만 하면 작업을 하기 위해 깨어난건지 아니면 가짜로 깼는지 모른다. 조건을 추가해주면 그 조건을 람다로 타면서 진짜 작업을 하기위한 쓰레드만 깨게 된다.

```cpp
while (true) {
    int task;
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);

        this->condition.wait(lock, [this] {
            return this->stop || !this->tasks.empty();
        });

        if (this->stop && this->tasks.empty()) return;

        task = tasks.front();
        tasks.pop();
    }

    ...
}
```

1. queue_mutex 잠금
2. 작업이 있거나 종료 신호가 올 때까지 대기
3. 깨어남
4. stop == true이고 tasks가 비어 있으면 스레드 종료
5. 아니면 tasks에서 작업 하나 꺼냄
6. queue_mutex 해제 // 스코프 벗어난 거
7. 작업 처리 // 스코프 밖

작업을 꺼낸 다음 unique_lock의 mutex를 해제하게 된다.

일단 처음 코드로 돌아가서, main 함수가 끝나면 선언해뒀던 ThreadPool 객체가 소멸하면서, stop이 true가 되고, condition_variable에 notify_all()을 호출해 모두 깨워버린다.

그 다음 worker.join으로 워커가 끝나는 걸 기다리는데, 워커 생성할 때 종료신호가 와도 task가 비어있지않으면 남은 작업을 계속 처리하도록 했기 때문에 큐가 완전히 비면 그때 워커가 종료된다.

enqueue는 mutex걸고, task리스트에 작업 넣고, condition.notify_one() 호출한다.

notify_one은 잠자고있는 워커중 하나를 꺠워서 처리하는 것으로, 만약에 작업을 여러개 처리하고 싶으면 작업 개수만큼 notify 하던가 notify_all하면 된다.

notify시키는 곳이 mutex unlock 이후 시점인 이유는, 워커가 꺠어나도 바로 queue_mutex를 잡아야하는데, enque쪽에서 아직 lock을 들고 있는 상황이 발생할 수 있기 때문이다.
-> Hurry up and wait라고 부르는 듯?

출력에도 뮤텍스 있는 이유는 아래처럼 나오지 말라고

```bash
[Worker [Worker 1] processed task 2
0] processed task 1
```

참고로 글로벌 lock이 아니라 queue 용 뮤텍스, 출력용 뮤텍스가 나뉘어져있다.

소멸자에서, 

```cpp
~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();

    for (std::thread &worker : workers) worker.join();
}
```

join()은 메인 스레드가 워커 스레드 종료를 기다리는 코드다. C++에서 std::thread 객체가 아직 join 또는 detach 되지 않은 상태로 소멸되면 std::terminate()가 호출된다.

종료는 될 건데 비정상 종료된다.

소멸할 때 notify_all하는 이유도 단순하다. 자고있는 것일 뿐이지 워커가 종료된 게 아니다. 자고있어서 stop이 true로 바뀌어도 모른다.

그래서 일단 깨워서 stop 플래그가 true인걸 먹여줘서 워커를 종료시키는 것이다.

일명 graceful shutdown이다.

예시에서는 큐에 들어갈 작업이 int였는데, 좀 더 제대로 하려면 `std::queue<std::function<void()>> tasks;` 이렇게 쓰는 게 맞다..

그러면 이게 가능함

```cpp
pool.enqueue([] {
    std::cout << "hello\n";
});

pool.enqueue([] {
    heavyWork();
});
```

함수로 enqueue를 바꾸면, `task = std::move(tasks.front());`가 더 좋다. 

큐 안에 있던 작업 객체를 로컬 변수 task로 이동시킨다.

복사하기 어려운 객체일 수 있으니 그냥 주소값을 줘버리는 것. 이거 말고도 포인터를 가진 객체가 들어올 수도 있으니 큐에서 작업 꺼낼때는 std::move를 쓰는 것 같다.
