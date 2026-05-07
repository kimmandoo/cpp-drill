## android log 분석기

1. file로 로그 읽어옴
2. event별 빈도 측정
3. event level 별 빈도 측정
4. 어떤 error 이벤트가 언제 발생했는 지 기록

우선 싱글 스레드로 만들고, 멀티 스레드로 발전시키기

## trouble shooting

### `I ACE    : text`, `Mams:BackupManager:`와 같이 이상한 포맷이 tag위치에 숨어있음

iss로 스트림 받아서 토큰 끊으면 ACE만 읽힘.

`: `를 기준으로 한 번 끊어서 다시 처리하게

### trim함수 c11에 없음

```cpp
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}
```

만들어서 사용

### `std::istringstream` 제거

iss가 실행 될 때 내부적으로 임시 문자열 복사, 버퍼 초기화가 계속 발생하는데 이걸 수백만 줄 로그에 매번 호출하면 성능 저하 발생...

공백 위치만 따라가면서 직접 문자열 토큰화 적용

target 로그 파일은 약 707만 line의 안드로이드 로그파일

before

```bash
 Performance counter stats for './a.out target.log':

        5833261700      task-clock:u                     #    0.904 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
              7046      page-faults:u                    #    1.208 K/sec                     
       67173384544      instructions:u                   #    2.78  insn per cycle            
                                                  #    0.00  stalled cycles per insn   
       24199969892      cycles:u                         #    4.149 GHz                       
          89101891      stalled-cycles-frontend:u        #    0.37% frontend cycles idle      
          21793948      stalled-cycles-backend:u         #    0.09% backend cycles idle       
       14225179498      branches:u                       #    2.439 G/sec                     
          45729645      branch-misses:u                  #    0.32% of all branches           

       6.452690211 seconds time elapsed

       5.150092000 seconds user
       0.682432000 seconds sys
```

after

```bash
 Performance counter stats for './a.out target.log':

        4164682200      task-clock:u                     #    0.900 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
              7942      page-faults:u                    #    1.907 K/sec                     
       40759051194      instructions:u                   #    2.52  insn per cycle            
                                                  #    0.00  stalled cycles per insn   
       16186377422      cycles:u                         #    3.887 GHz                       
          47124765      stalled-cycles-frontend:u        #    0.29% frontend cycles idle      
          16300362      stalled-cycles-backend:u         #    0.10% backend cycles idle       
        8798683981      branches:u                       #    2.113 G/sec                     
          32728184      branch-misses:u                  #    0.37% of all branches           

       4.627105785 seconds time elapsed

       3.532178000 seconds user
       0.631399000 seconds sys
```

전체 실행 시간 28% 개선
instruction 개수 39% 개선

불필요한 객체 생성이 없어지면서 instruction 개수가 줄어든 것으로 보임

### vector에 문자열 추가하는 대신 offset만 저장해서 처리하기

로그 파일이 커질수록 vector에 저장되는 데이터 또한 엄청 늘어날 것. 메모리 공간 초과가 발생할 수 있기 때문에 개선해보겠음.

```cpp
for (std::streampos offset : result.errorOffsets) {
    originalFile.seekg(offset); // 해당 바이트 위치로 포인터 이동
    std::getline(originalFile, errorLine); // 그 한 줄만 읽기
    outFile << errorLine << "\n";
}
```

streampos를 쓴다. std::streampos는 파일/스트림 내부 위치를 나타내는 타입.

offset으로 에러 읽어올 때는 당연히 다른 stream으로 파일을 열어야됨.

before
```bash
 Performance counter stats for './a.out target.log':

        4098312500      task-clock:u                     #    0.901 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
              7257      page-faults:u                    #    1.771 K/sec                     
       40069985213      instructions:u                   #    2.49  insn per cycle            
                                                  #    0.00  stalled cycles per insn   
       16061188341      cycles:u                         #    3.919 GHz                       
          62378787      stalled-cycles-frontend:u        #    0.39% frontend cycles idle      
          14096448      stalled-cycles-backend:u         #    0.09% backend cycles idle       
        8612037313      branches:u                       #    2.101 G/sec                     
          34402430      branch-misses:u                  #    0.40% of all branches           

       4.550336995 seconds time elapsed

       3.444756000 seconds user
       0.652690000 seconds sys
```
after
```bash
 Performance counter stats for './a.out target.log':

        3376384900      task-clock:u                     #    0.897 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               974      page-faults:u                    #  288.474 /sec                      
       21758779563      instructions:u                   #    2.31  insn per cycle            
                                                  #    0.01  stalled cycles per insn   
        9401937609      cycles:u                         #    2.785 GHz                       
         211613645      stalled-cycles-frontend:u        #    2.25% frontend cycles idle      
          56140139      stalled-cycles-backend:u         #    0.60% backend cycles idle       
        4657979802      branches:u                       #    1.380 G/sec                     
          39074444      branch-misses:u                  #    0.84% of all branches           

       3.766160090 seconds time elapsed

       2.202098000 seconds user
       1.174452000 seconds sys
```

문자열 복사 저장을 오프셋으로 처리해서 원문 그대로 갖다가 읽음.

page-fault 86%감소 복사, 저장이 안일어나니까 heap 할당도 확 줄어든다.
instruction도 반토막
user타임은 줄고, sys타임은 늘었음, 근데 전체 시간은 줄었음.
-> 문자열 연산 안하니까 앱 내 연산 시간은 확 줄고, 시스템 함수로 offset 처리해야되서 sys시간이 늘어났다. 시스템 콜은 늘었지만 문자열 복사가 없어져서 전체 실행시간이 줄었음.

# multi-thread 적용

지금은 겨우 수백만 줄인데, 수천만 줄을 대상으로 하면 더 오랜시간이 걸릴 것으로 판단.

cpu 코어 개수만큼 나눠서 처리를 나눠놨더니 시간이 확 줄었다. 병렬 계산의 장점

single
```bash
 Performance counter stats for './single_analyzer target.log':

        1640145300      task-clock:u                     #    0.909 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               971      page-faults:u                    #  592.021 /sec                      
       11672982755      instructions:u                   #    2.80  insn per cycle            
                                                  #    0.02  stalled cycles per insn   
        4171844792      cycles:u                         #    2.544 GHz                       
         204008129      stalled-cycles-frontend:u        #    4.89% frontend cycles idle      
          52795928      stalled-cycles-backend:u         #    1.27% backend cycles idle       
        2633212301      branches:u                       #    1.605 G/sec                     
          30357161      branch-misses:u                  #    1.15% of all branches           

       1.804770269 seconds time elapsed

       1.072711000 seconds user
       0.563628000 seconds sys
```

multi
```bash
 Performance counter stats for './multi_analyzer target.log':

        4947830100      task-clock:u                     #    8.094 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
              3228      page-faults:u                    #  652.407 /sec                      
       21721996561      instructions:u                   #    1.93  insn per cycle            
                                                  #    0.03  stalled cycles per insn   
       11259169441      cycles:u                         #    2.276 GHz                       
         584661449      stalled-cycles-frontend:u        #    5.19% frontend cycles idle      
         141380731      stalled-cycles-backend:u         #    1.26% backend cycles idle       
        4909704443      branches:u                       #  992.294 M/sec                     
          56120091      branch-misses:u                  #    1.14% of all branches           

       0.611267262 seconds time elapsed

       2.886787000 seconds user
       1.966233000 seconds sys
```

테스트 환경이 8코어라, cpu사용이 8배 늘어났음. 작업 코어가 늘어나면서 user, sys 처리시간도 같이 늘어났으나 전체 시간은 확 줄음

## trouble shooting

### 작업 분할

```cpp
size_t numThreads = std::thread::hardware_concurrency();
if (numThreads == 0) numThreads = 4;

std::streampos chunkSize = fileSize / numThreads;
std::vector<std::future<AnalysisResult>> futures;

for (size_t i = 0; i < numThreads; ++i) {
   std::streampos start = i * chunkSize;
   std::streampos end = (i == numThreads - 1) ? fileSize : start + chunkSize;
   
   futures.push_back(std::async(std::launch::async, analyzeChunk, filePath, start, end));
}
```
코어 개수만큼 덩어리를 나누고, 처음부터 그 덩어리 크기대로 offset잡아서 돌리기

이거 제외하면 나머지는 거의 동일.

병렬 작업을 시키기 위해서 future header를 사용한다.

`std::async`, `std::launch::async`, `std::future` 이거 세개 쓸 때 필요함.

지금 병렬작업은 thread를 직접 생성해서 하는 게 아니라 std::async로 작업함. std::launch::async를 값으로 줬기 때문에 각 analyzeChunk 작업을 별도의 비동기작업으로 실행한다.

그리고 이 작업을 std::future를 타입으로 갖는 vector에 push_back 하는데, future는 나중에 결과가 도착할 것임을 기대하는 역할이다.

analyzeChunk가 AnalysisResult를 반환하는데, 이걸 비동기로 받을 수있게 future로 감싸서 vector로 만든 것.

`chunkResults.push_back(f.get());`여기서 f.get()은 future의 멤버 함수를 쓰는 것으로, 각 작업이 끝날때까지 기다렸다가 값을 반환한다.

그리고 스레드작업이 있기 때문에, 빌드할 때 `-pthread` 옵션을 추가해준다.

wsl에서 테스트할 때는 일단 빼도 문제가 없었다.

pthread 관련 심볼이 통합돼서 링크에러가 해결된 것으로 추정되는데, 일단 스레드 옵션을 켜서 빌드하면 컴파일러한테 스레드 관련 설정을 적용하라는 의미가 명시적이므로 붙여서 실행하는 걸 권장한다고 함.



### 오프셋 정렬

```cpp
std::sort(global.errorOffsets.begin(), global.errorOffsets.end());
```

파일에서 읽어오기 전에, offset을 정렬해주는 작업이 필요함.

ifstream이 내부버퍼에 저장해둔 내용을 재사용할 수 있는 확률이 높아지기 때문인데, 만약 offset이 널뛰면 내부 버퍼도 거기에 맞춰 계속 invalidate 될 것.

offset 기준으로 정렬하면 원본 파일 접근 시 seek 이동이 줄어들어서 순차읽기에 가까워져서 스트림 캐싱 효율이 좋아짐...

