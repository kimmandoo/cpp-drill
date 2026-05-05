# memory쪽 더 깊게 파기

```cpp
int flag = 0;
int data = 0;

void writer() {
    data = 42;
    flag = 1;
}

void reader() {
    while (flag == 0) {
    }

    std::cout << data << std::endl;
}
```

이전에 우리는 thread, mutex, lock_guard로 해결할 수 있었다.

동기화가 없던 코드에 동기화를 넣어서 안전하게 만든 것이다.

mutex는 상호배제, 메모리 동기화 문제를 해결한다.

상호배제는 한번에 하나의 스레드만 임계구역에 들어가게 해서 해결하고, 메모리 동기화는 이전 스레드가 lockdㅔ서 변경한 값을 다음 lock 획득 스레드가 볼 수 있게 해서 처리한다.

```cpp
int data = 0;
bool ready = false;
std::mutex mtx;

void writer() {
    std::lock_guard<std::mutex> lock(mtx);
    data = 42;
    ready = true;
}

void reader() {
    std::lock_guard<std::mutex> lock(mtx);

    if (ready) {
        std::cout << data << std::endl;
    }
}
```

쓸 때도, 읽을 때도 같은 mutex를 사용해서 처리하기 때문에 ready가 true면 data가 42임이 보장된다.

같은 공유 상태는 같은 mutex로 보호해야 보장이된다.

```cpp
int data = 0;
bool ready = false;

std::mutex dataMtx;
std::mutex readyMtx;
```

논리적으로 한 상태를 나타내는 두 값이 있다면 한 mutex로 처리해줘야지, 각각의 mutex로 나누게 되면 data race는 발생하지 않더라도 race condition은 발생한다.

atomic같은 cas 타입은 mutex 없이도 특정 연산을 안전하게 처리해준다. 근데 memory_order 문제가 있다.

```cpp
std::atomic<bool> ready = false;
std::atomic<int> data = 0;

void writer() {
    data.store(42, std::memory_order_relaxed);
    ready.store(true, std::memory_order_release);
}

void reader() {
    while (!ready.load(std::memory_order_acquire)) {
    }

    std::cout << data.load(std::memory_order_relaxed) << std::endl;
}
```

writer가 data = 42를 쓴 뒤 ready = true를 release로 저장한다. 그러고나서 reader가 ready == true를 acquire로 읽고, reader는 writer가 ready 이전에 쓴 data도 볼 수 있다.

## memory_order

memory_order_relaxed는 원자성만 보장하고, 순서보장은 되지않는다.

한 가지 데이터의 연산만 생각하면 최종 값이 올바르게 계산되기 때문에 단순 카운터로 쓸 수는 있겠지만 다른 데이터와의 순서관계가 고려되지않는다.

```cpp
std::atomic<int> requestCount = 0;

void onRequest() {
    requestCount.fetch_add(1, std::memory_order_relaxed);
}
```

memory_order_release / acquire는 스레드 간 데이터 전달에 사용한다.

```cpp
std::atomic<bool> ready = false;
int data = 0;

void writer() {
    data = 42;
    ready.store(true, std::memory_order_release);
}

void reader() {
    while (!ready.load(std::memory_order_acquire)) {
    }

    std::cout << data << std::endl;
}
```

release이전에 있던 data는 acquire 시점에서 읽어올 때까지 동일하다. store, load로 사용한다.

memory_order_seq_cst는 가장 강한 기본값이다. std::atomic<int>를 사용했을때 ++연산은 기본적으로 seq_cst다.

## mutex를 쓰더라도 생각할게 있음

mutex를 쓰면 여러 위험에서는 안전해지지만, 여러 스레드가 같은 mutex를 잡으려고 하는 과정에서 병목이 생길 수 있기 때문에 무조건 빠른 건 아니다.

이걸 lock contention이라고 한다.

```cpp
std::mutex mtx;
int counter = 0;

void work() {
    for (int i = 0; i < 1000000; i++) {
        std::lock_guard<std::mutex> lock(mtx);
        counter++;
    }
}
```
이 코드는 병렬성은 고려하지않고, counter++가 한번에 하나씩만 실행되는 코드다.

여러 방법이 있겠지만 local counter 사용 후 합치는 방법이 있다.

```cpp
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
```

공유 자원을 자주 건드리지 않고 스레드 별 local 데이터로 처리한 다음 마지막에 합친다.

counter를 10개의 스레드가 매번 하나씩 더하는 것에서, 각 스레드 별로 1000씩 병렬계산해서 처리하는 거로 바뀐 거다.

## false sharing을 아는지?

```cpp
struct Counters {
    std::atomic<int> a;
    std::atomic<int> b;
};
```

각 쓰레드가 a,b 각각 증가시킨다고 하면, 겉으로는 공유 문제가 없어보인다.

cpu캐시는 cache line 단위로 움직이는데 만약 a, b가 같은 캐시라인에 있으면 서로 다른 변수를 건드리고 있지만 cpu는 같은 캐시라인 끼리 값을 주고 받게 되는 것이다.

```text
Thread A: a 수정
CPU Core A가 cache line 소유

Thread B: b 수정
CPU Core B가 같은 cache line 소유권 가져감

Thread A: 다시 a 수정
Core A가 다시 cache line 가져감
```

서로 다른 변수를 쓰고 있지만 같은 캐시라인을 쓰고 있어서 불필요하게 소유권 교체가 계속 일어난다.. 그래서 보통 64kb단위로 잡히는 캐시라인을 분리해주기 위해 padding이나 alignment를 사용해서 다른 캐시라인을 잡도록 처리해주는 스킬이 있다고 함.

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<int> value;
};
```

성능 분석은 리눅스 도구 인 perf로 수행할 수 있다.

나는 orbstack을 써서 테스트하고 있는데, 커널버전이 확실하지않아서 버전명이 명시된 패키지를 설치하려고 하면 뻗는다.

```bash
sudo apt-get update
sudo apt-get install linux-tools-generic linux-cloud-tools-generic
```

basic counter랑 local counter 소스를 비교해보면 아래와 같다. 


```bash
perf stat ./a.out
Total: 10

 Performance counter stats for './a.out':

           1637667      task-clock:u                     #    0.718 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               134      page-faults:u                    #   81.824 K/sec                     
   <not supported>      cycles:u                                                              

       0.002280036 seconds time elapsed

       0.000000000 seconds user
       0.002858000 seconds sys

perf stat ./b.out
Total: 10000

 Performance counter stats for './b.out':

           1052331      task-clock:u                     #    0.823 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               132      page-faults:u                    #  125.436 K/sec                     
   <not supported>      cycles:u                                                              

       0.001277900 seconds time elapsed

       0.000476000 seconds user
       0.001429000 seconds sys
```

a가 basic, b가 local counter를 사용한 소스다.

이제 이론은 어느정도 했으니까 진짜 로그 분석기 만들어볼 시간임.