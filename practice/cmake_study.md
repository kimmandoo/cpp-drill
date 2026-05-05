CMake는 g++에게 넘길 빌드 명령을 만들어주는 도구.

원래 cpp파일을 g++로 컴파일해서 -o로 나온 실행파일을 실행했다면, CMake는 흐름이 이럼

```text
CMakeLists.txt 작성
   ↓
cmake -S . -B build
   ↓
build 디렉토리에 빌드 시스템 생성
   ↓
cmake --build build
   ↓
실행 파일 생성
```

인데, 

```bash
g++ main.cpp calculator.cpp user.cpp network.cpp -o app
g++ main.cpp calculator.cpp -Iinclude -Llib -lsomething -o app
```

만약에 파일이 여러개가 되고, 라이브러리를 붙여서 써야되면 힘들어진다.

솔직히 sanitizer 쓸 때도 옵션 너무 길다고 생각했다.

그런걸 미리 설정해서 쉽게 실행할 수 있도록하는게 cmake가 하는 일이다.

CMakeList를 만들고, 

```cmake
cmake_minimum_required(VERSION 3.16)

project(HelloCMake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hello main.cpp)
```

빌드하면 된다.

```bash
cmake -S . -B build
cmake --build build
./build/hello
```

맨 윗 줄부터 알아보면

- 이 프로젝트가 요구하는 최소 CMake 버전
- 프로젝트이름
- cpp 17 사용
- cpp 17로 빌드 안하면 에러내기
- main.cpp를 컴파일해서 hello라는 실행파일 만들기

이다.

명령어는 

```text
-S .        source directory는 현재 폴더
-B build    build directory는 build 폴더
```

라서, cmake하고 나면 build폴더가 생성되고 --build로 그 폴더를 빌드한다.

02-multi-file의 CMakeLists파일을 보면 헤더파일은 executable에 안들어가있다.

```cmake
cmake_minimum_required(VERSION 3.16)

project(MultiFile)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
    main.cpp
    calculator.cpp
)
```

실제로 컴파일 되는 건 cpp파일이라서 그런듯.

03-include가 중요함.

```cmake
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

이게 app 파일을 컴파일할 때, includㄷ 폴더를 헤더 검색 경로에 추가하라는 의미임.

PRIVATE같은 scope 키워드가 있는데, 내 실행 파일에서만 쓸 include 경로면 PRIVATE, 다른 라이브러리 사용자에게도 알려야 하면 PUBLIC, 헤더만 전달해야 하면 INTERFACE이다.

이걸 보면, CMake에서 실행파일, 라이브러리 모두 target으로 치는데, 이번엔 라이브러리를 만들어보자

```cmake
add_library(calculator
    src/calculator.cpp
)

target_include_directories(calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(app PRIVATE
    calculator
)
```

add_library로 라이브러리 화 할 코드를 등록하고, 그걸 app이 사용한다고 link 한다.

근데 calculator에 include 경로를 PUBLIC으로 줬기 때문에 app은 include 경로 몰라도 된다.

## PRIVATE / PUBLIC / INTERFACE

```cmake
target_include_directories(my_lib PRIVATE include)
```

이건 include 폴더가 my_lib을 빌드할 때만 필요하고, my_lib 사용하는 쪽으로 전파하지 않는다.

```cmake
target_include_directories(my_lib PUBLIC include)
```

이건 my_lib을 사용하는 쪽에도 전파된다.

```cmake
target_include_directories(my_lib INTERFACE include)
```

my_lib 자체 빌드시에는 필요없늗네, my_lib 사용하는 쪽이 필요함

.cpp에서만 필요하다 -> PRIVATE
.h에 노출된다(헤더파일의 공개가 필요하다) -> PUBLIC
헤더-only 라이브러리다 -> INTERFACE

디버깅하려면 debug 빌드를 써야된다.

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

release 빌드에는 최적화가 적용되어있어서 디버깅이 어려움.

디버그로 만들어진 실행 파일에 gdb를 켜서 디버깅 가능


# llvm 링크하는 거 추가 공부

kaleidoscope 학습 하면서 CMakeLists도 만들어봤는데, 각 줄이 뭘 의미하는 지 보겠다.

```cmake
cmake_minimum_required(VERSION 3.16)

project(toy_llvm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(LLVM_CONFIG llvm-config REQUIRED)

execute_process(
    COMMAND ${LLVM_CONFIG} --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CXXFLAGS NATIVE_COMMAND ${LLVM_CXXFLAGS})

execute_process(
    COMMAND ${LLVM_CONFIG} --ldflags --system-libs --libs all
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_LDFLAGS NATIVE_COMMAND ${LLVM_LDFLAGS})

add_executable(toy
    my_llvm.cpp
)
target_compile_options(toy PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(toy PRIVATE ${LLVM_LDFLAGS})

set(GENERATED_OBJECT ${CMAKE_BINARY_DIR}/output.o)
set(KALEIDOSCOPE_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/kaleidoscope_input.txt)

add_custom_command(
    OUTPUT ${GENERATED_OBJECT}
    COMMAND /bin/bash -c "$<TARGET_FILE:toy> < ${KALEIDOSCOPE_INPUT}"
    DEPENDS toy ${KALEIDOSCOPE_INPUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating output.o from Kaleidoscope source"
    VERBATIM
)

add_custom_target(generate_output DEPENDS ${GENERATED_OBJECT})

add_executable(main
    main.cpp
    ${GENERATED_OBJECT}
)
add_dependencies(main generate_output)
```

한 줄씩 보면 다음 뜻이다.

- `cmake_minimum_required(VERSION 3.16)`
    - 이 프로젝트를 빌드하려면 최소 CMake 3.16 이상이 필요하다는 뜻이다.
- `project(toy_llvm LANGUAGES CXX)`
    - 프로젝트 이름을 `toy_llvm`로 정하고, C++만 사용하겠다고 선언한다.
- `set(CMAKE_CXX_STANDARD 17)`
    - C++17 표준으로 컴파일하도록 설정한다.
- `set(CMAKE_CXX_STANDARD_REQUIRED ON)`
    - C++17이 아니면 다른 버전으로 대충 넘어가지 말고 오류를 내도록 한다.
- `find_program(LLVM_CONFIG llvm-config REQUIRED)`
    - 시스템에서 `llvm-config` 실행 파일을 찾고, 없으면 설정 단계에서 바로 실패한다.
- `execute_process(...)`
    - CMake 설정 단계에서 `llvm-config --cxxflags`를 실행해서 LLVM 컴파일 옵션을 가져온다.
- `OUTPUT_VARIABLE LLVM_CXXFLAGS`
    - 실행 결과를 `LLVM_CXXFLAGS` 변수에 저장한다.
- `OUTPUT_STRIP_TRAILING_WHITESPACE`
    - 출력 끝의 불필요한 공백과 줄바꿈을 제거한다.
- `separate_arguments(LLVM_CXXFLAGS NATIVE_COMMAND ${LLVM_CXXFLAGS})`
    - 문자열로 들어온 플래그들을 CMake가 다룰 수 있는 인자 목록으로 쪼갠다.
- 두 번째 `execute_process(...)`
    - `llvm-config --ldflags --system-libs --libs all`을 실행해서 링크에 필요한 옵션들을 가져온다.
- `OUTPUT_VARIABLE LLVM_LDFLAGS`
    - 그 결과를 `LLVM_LDFLAGS`에 저장한다.
- `separate_arguments(LLVM_LDFLAGS NATIVE_COMMAND ${LLVM_LDFLAGS})`
    - 링크 옵션 문자열도 인자 목록으로 분리한다.
- `add_executable(toy my_llvm.cpp)`
    - `my_llvm.cpp`를 컴파일해서 `toy` 실행 파일을 만든다.
- `target_compile_options(toy PRIVATE ${LLVM_CXXFLAGS})`
    - `toy`를 컴파일할 때만 LLVM 컴파일 옵션을 붙인다.
- `target_link_libraries(toy PRIVATE ${LLVM_LDFLAGS})`
    - `toy`를 링크할 때만 LLVM 라이브러리와 시스템 라이브러리를 붙인다.
- `set(GENERATED_OBJECT ${CMAKE_BINARY_DIR}/output.o)`
    - 생성될 결과 파일 `output.o`의 경로를 빌드 디렉토리 기준으로 저장한다.
- `set(KALEIDOSCOPE_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/kaleidoscope_input.txt)`
    - 입력용 Kaleidoscope 소스 파일의 경로를 현재 소스 디렉토리 기준으로 저장한다.
- `add_custom_command(...)`
    - `output.o`를 만드는 사용자 정의 빌드 명령을 등록한다.
- `OUTPUT ${GENERATED_OBJECT}`
    - 이 명령이 최종적으로 만들어내는 산출물을 `output.o`로 지정한다.
- `COMMAND /bin/bash -c "$<TARGET_FILE:toy> < ${KALEIDOSCOPE_INPUT}"`
    - `toy` 실행 파일을 실행하고, `kaleidoscope_input.txt`를 표준 입력으로 넘긴다.
- `DEPENDS toy ${KALEIDOSCOPE_INPUT}`
    - `toy`나 입력 파일이 바뀌면 `output.o`를 다시 만들도록 한다.
- `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}`
    - 커스텀 명령을 빌드 디렉토리에서 실행한다.
- `COMMENT "Generating output.o from Kaleidoscope source"`
    - 빌드 중에 어떤 작업을 하는지 메시지를 보여준다.
- `VERBATIM`
    - 명령 인자를 CMake가 안전하게 그대로 전달하도록 한다.
- `add_custom_target(generate_output DEPENDS ${GENERATED_OBJECT})`
    - `output.o` 생성을 전담하는 빌드 타겟을 만든다.
- `add_executable(main main.cpp ${GENERATED_OBJECT})`
    - `main.cpp`와 생성된 `output.o`를 합쳐서 최종 실행 파일 `main`을 만든다.
- `add_dependencies(main generate_output)`
    - `main`을 만들기 전에 먼저 `generate_output`이 실행되도록 순서를 보장한다.

이 흐름을 한 문장으로 정리하면, `my_llvm.cpp`로 Kaleidoscope 코드를 `output.o`로 먼저 만들고, 그 `output.o`를 `main.cpp`와 함께 링크해서 최종 프로그램을 만드는 구조다.