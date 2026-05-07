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

## multi-line log

안드로이드 로그를 보다보면 trace dump같은건 여러줄로 구성된 로그를 내뿜는데, 지금 한 줄씩 처리하는 구조로는 이게 대처가 안됨..

```log
05-05 10:30:11.296 24108 24108 D AndroidRuntime: Shutting down VM
05-05 10:30:11.297 24108 24108 E AndroidRuntime: FATAL EXCEPTION: main
05-05 10:30:11.297 24108 24108 E AndroidRuntime: Process: com.broadcom.nrdphelper, PID: 24108
05-05 10:30:11.297 24108 24108 E AndroidRuntime: java.lang.RuntimeException: Unable to start service com.broadcom.nrdphelper.HdmiCecActivenessService@8da311c with null: android.app.ForegroundServiceStartNotAllowedException: Service.startForeground() not allowed due to mAllowStartForeground false: service com.broadcom.nrdphelper/.HdmiCecActivenessService
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ActivityThread.handleServiceArgs(ActivityThread.java:4839)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ActivityThread.-$$Nest$mhandleServiceArgs(Unknown Source:0)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ActivityThread$H.handleMessage(ActivityThread.java:2289)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Handler.dispatchMessage(Handler.java:106)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Looper.loopOnce(Looper.java:205)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Looper.loop(Looper.java:294)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ActivityThread.main(ActivityThread.java:8177)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at java.lang.reflect.Method.invoke(Native Method)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:552)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:971)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: Caused by: android.app.ForegroundServiceStartNotAllowedException: Service.startForeground() not allowed due to mAllowStartForeground false: service com.broadcom.nrdphelper/.HdmiCecActivenessService
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ForegroundServiceStartNotAllowedException$1.createFromParcel(ForegroundServiceStartNotAllowedException.java:54)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ForegroundServiceStartNotAllowedException$1.createFromParcel(ForegroundServiceStartNotAllowedException.java:50)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.readParcelableInternal(Parcel.java:4891)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.readParcelable(Parcel.java:4873)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.createExceptionOrNull(Parcel.java:3073)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.createException(Parcel.java:3062)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.readException(Parcel.java:3045)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.os.Parcel.readException(Parcel.java:2987)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.IActivityManager$Stub$Proxy.setServiceForeground(IActivityManager.java:6761)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.Service.startForeground(Service.java:862)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at com.broadcom.nrdphelper.HdmiCecActivenessService.onStartCommand(HdmiCecActivenessService.kt:100)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	at android.app.ActivityThread.handleServiceArgs(ActivityThread.java:4821)
05-05 10:30:11.297 24108 24108 E AndroidRuntime: 	... 9 more
```

이런거랑,

```log
--------- beginning of crash
F libc    : Fatal signal 11 (SIGSEGV)
backtrace:
    #00 pc 000000000001234
    #01 pc 000000000004321
```

stacktreace 처럼 여러줄 줄바꿈으로 되어있는거 대응안됨. 실제로는 이벤트 하나인데, 라인수만큼 다른 이벤트로 잡힐 수 있는 위험이 있다.

stacktrace 형태로 들어오면 지금 구현으로는 ignore된다.

그리고 문제가 하나 더 있는데, 멀티스레드를 적용하면서 각 chunk를 무식하게 n등분했다. 근데 멀티라인이나 이런게 껴있으면 한가지 이벤트를 여러 chunk가 나눠 갖게 되면서 제대로 처리가 안될 것임.

pid, gid가 같은걸 사용하면 어떻게 될 거 같은데, 이러면 메모리 부담이 클 것 같음..

### 지금 로그 패턴을 로그 헤더로 두고, 다른 로그 패턴이 들어오기 전에는 이전로그를 continuation 처리

제일 핵심 로직.

일단 logrecord struct에 pid, tid를 추가해서 쓴다. 헤더로 구별할 것이기 때문에 유일성을 높여줄 조건임.

```cpp
bool isSameHeaderContinuation(const LogRecord& previous, const LogRecord& current) {
    return previous.timestamp == current.timestamp
        && previous.pid == current.pid
        && previous.tid == current.tid
        && previous.level == current.level
        && previous.tag == current.tag;
}
```
timestamp, pid, tid, level, tag를 비교해서 이전로그랑 같으면 continuation으로 처리하기. 

```cpp
if(!parseLogLine(line, record)){
   if (hasPreviousLogHeader) {
         result.continuationLines++;
         continue;
   }

   result.ignoredLines++;
   continue;
}

if (hasPreviousLogHeader && isSameHeaderContinuation(previousRecord, record)) {
   result.continuationLines++;
   previousRecord = record;
   continue;
}

hasPreviousLogHeader = true;
previousRecord = record;
analyzeRecord(record, currentOffset, result);
```

이게 이어지는 로그인지 판단해주는 곳임.

1. 만약 이전 로그가 정상 파싱되었는데 지금 로그가 이상하면 continuation으로 간주
2. 새 라인이 이전 로그 헤더랑 동일할 때 continuation으로 간주
3. 두 가지 조건에 안걸리면 renew.

continuation의 경우 analyzeRecord를 넘겨 eventCount에 들어가지않게 해서 과대 측정을 막는다.

출력 때도 마찬가지

```cpp
outFile << "\nError Events:\n";
if (result.errorOffsets.empty()) {
   outFile << "No error events found.\n";
} else {
   std::ifstream originalFile(originalFilePath);
   if (originalFile.is_open()) {
      std::string errorLine;
      std::streampos consumedUntil = 0;
      for (std::streampos offset : result.errorOffsets) {
            if (offset < consumedUntil) {
               continue;
            }

            originalFile.seekg(offset);
            bool isFirstLine = true;
            LogRecord previousRecord;

            while (std::getline(originalFile, errorLine)) {
               std::streampos nextOffset = originalFile.tellg();
               LogRecord record;
               bool parsed = parseLogLine(errorLine, record);

               if (isFirstLine) {
                  if (!parsed) {
                        break;
                  }

                  previousRecord = record;
               } else if (parsed) {
                  if (!isSameHeaderContinuation(previousRecord, record)) {
                        break;
                  }

                  previousRecord = record;
               }

               outFile << errorLine << "\n";
               if (nextOffset != std::streampos(-1)) {
                  consumedUntil = nextOffset;
               }
               isFirstLine = false;
            }
      }
   }
}
```

파일에 쓰는, 그러니까 출력 쪽인데, errorOffsets를 사용해서 실제 에러문단을 잘라내는 로직이다.

1. 첫 줄이면 error 오프셋으로 한줄 읽어서 멀티라인여부 판단 시작
2. 이전 로그랑 같으면 출력하고 계속 while 돌기
3. 다르면 while 종료하고 다음 error오프셋 찾아서 계속 진행

## chunk 문제 아직 남음

analyzeChunk를 보면 chunk 끝에 도달했을 때 루프를 끝낸다. 그래서 멀티라인 에러 파싱 도중에도 끊길수가 있음..

```cpp
if (currentOffset == std::streampos(-1) || currentOffset >= endOffset) {
   // break;
   if (!hasPreviousLogHeader) { 
         break; 
   }
}
```

단순하게 생각해서 이렇게 했더니 700만 줄이 아니라 6000만줄을 읽었다고 나왔다..

chunk가 안끊겨서 모든 스레드가 파일을 끝까지 읽었나봄..

상태값을 더 추가한다.

각 청크 별로 첫 로그, 마지막 로그가 parse되는지 여부, 그리고 그때의 LogRecord를 기록해두고 그걸 맞춰서 쓰는 방향으로 발전시키기.

```cpp
struct AnalysisResult {
    size_t totalLines = 0;
    size_t parsedLines = 0;
    size_t continuationLines = 0;
    size_t ignoredLines = 0;

    bool hasFirstRecord = false;
    LogRecord firstRecord;
    std::streampos firstRecordOffset = 0;
    bool hasLastRecord = false;
    LogRecord lastRecord;

...

void decrementCount(std::unordered_map<std::string, size_t>& counts, const std::string& key) {
    auto it = counts.find(key);
    if (it == counts.end()) return;

    if (it->second <= 1) {
        counts.erase(it);
        return;
    }

    it->second--;
}

void removeOffset(std::vector<std::streampos>& offsets, std::streampos offset) {
    auto it = std::find(offsets.begin(), offsets.end(), offset);
    if (it != offsets.end()) {
        offsets.erase(it);
    }
}
```

decrementCount와 removeOffset은 merge 과정에서 chunk 경계 continuation을 보정하기 위해 사용된다. 

현재 chunk의 첫 로그가 실제로는 이전 chunk 마지막 로그의 continuation이어도 분석 당시에는 새 이벤트로 집계될 수 있다. 

merge에서 이전 chunk의 마지막 로그와 현재 chunk의 첫 로그의 timestamp, pid, tid, level, tag가 같으면 같은 묶음으로 판단하고, 이미 증가한 parsedLines, levelCount, eventCount를 하나씩 되돌린다. 

해당 줄은 삭제되는 것이 아니라 continuation으로 재분류되는 것이므로 continuationLines는 증가시킨다. 또한 ERROR 로그였다면 errorOffsets에서도 현재 chunk 첫 로그 offset을 제거해서 결과 출력 시 같은 에러 묶음이 중복 출력되지 않도록 한다.


chunk문제를 해결하면 비교과정이 생기므로 당연히 오버헤드가 증가한다. 처음이 chunk처리 없는 것, 두번째가 chunk처리 있는거다.

성능 개선이 아니라 정합성에 가까운 처리였다.

