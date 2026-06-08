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

# CMakeList

CMake 프로젝트의 진입점은 최상위 디렉터리의 CMakeLists.txt이고, add_subdirectory()로 추가한 하위 디렉터리도 자기 CMakeLists.txt를 가질 수 있다. CMake 언어 파일은 CMakeLists.txt 또는 .cmake 파일이고, CMake는 이 파일들로부터 Ninja, Makefile, Visual Studio 프로젝트 같은 실제 빌드 시스템을 생성한다.

CMake는 컴파일러도 링커도 빌드 도구도 아닌, 빌드 시스템 생성기다. 뭘 빌드할지 읽어서 정해진 generator로 실제 빌드 파일을 만들어내는 과정이다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
cmake --install build --prefix install # 빌드된 프로젝트의 설치 rule 실행. requirements 같은거
```

-S는 소스 디렉터리, -B는 빌드 디렉터리이다. cmake --build는 이미 생성된 빌드 트리를 빌드한다.

```cmake
cmake_minimum_required(VERSION 3.25)

project(hello
  VERSION 1.0.0
  DESCRIPTION "Tiny CMake example"
  LANGUAGES CXX
)

add_executable(hello
  src/main.cpp
)

target_compile_features(hello PRIVATE cxx_std_20)
```

cmake_minimum_required()는 최소 CMake 버전을 요구하고 policy 설정도 같이 잡는다. project()는 프로젝트 이름, 버전, 설명, 홈페이지, 언어 등을 설정하며, 최상위 CMakeLists.txt에는 직접적인 project() 호출이 있어야 하는데, 당연히 버전, policy가 project보다 먼저 호출돼야된다. add_executable()은 실행 파일 target을 만들고, target_compile_features()는 해당 target이 요구하는 컴파일 기능을 선언한다.

## target 중심으로 생각하기

```cmake
include_directories(include)
link_directories(/some/lib)
add_compile_options(-Wall)
```

이게 잘 한걸까.. 

이런 식으로 전역 설정을 뿌리면, 어떤 target이 어떤 include path, compile option, link dependency를 필요로 하는지 알기 어렵다.

```cmake
add_library(mathlib
  src/add.cpp
  src/mul.cpp
)

target_include_directories(mathlib
  PUBLIC
    include
)

target_compile_features(mathlib
  PUBLIC
    cxx_std_20
)

add_executable(app
  src/main.cpp
)

target_link_libraries(app
  PRIVATE
    mathlib
)
```

CMake의 buildsystem 모델은 target과 target property 중심이다. target_include_directories()는 target의 include directory를 설정하고, target_link_libraries()는 링크할 라이브러리와 usage requirement를 전파합니다. target_compile_features()는 필요한 언어 기능을 선언하고, 필요하면 -std=... 같은 플래그를 CMake가 추가.

target_link를 보면 접근제어자가 있다.

```cmake
target_link_libraries(my_lib
  PRIVATE   impl_dep
  PUBLIC    api_dep
  INTERFACE header_only_dep
)
```

dependency가 구현에만 쓰이면 PRIVATE, header에도 쓰이면 PUBLIC, 구현에는 없고 header usage requirement만 있으면 INTERFACE로 지정하면 된다. 

PUBLIC dependency의 usage requirement는 consumer에게 전파되고, PRIVATE dependency의 usage requirement는 consumer에게 전파되지 않는다.

조금 더 단순하게 설명하면 PRIVATE는 .cpp안에서만 쓰는 라이브러리, PUBLIC은 public header에 노출되는 타입, 헤더, INTERFACE는 header-only 라이브러리나 compile definition에 전달되는 용도다.

```cpp
#include <fmt/core.h>

namespace mylib {
class Widget {
public:
  std::string format() const;
};
}
```

fmt가 .cpp에만 쓰이면 PRIVATE, public header가 fmt 헤더를 include한다면 fmt는 PUBLIC이다.

퍼블릭 헤더는 라이브러리 사용자가 자신의 코드에서 #include 하여 사용할 클래스, 함수, 상수 등이 선언된 파일이다.

이게 잘못들어가면 라이브러리 link가 깨질 수 있다.

```cmake
add_executable(my_app
  src/main.cpp
)
```

add_executable()은 지정한 source file들로 빌드되는 executable target을 추가하는데, target 이름은 프로젝트 안에서 전역적으로 유일해야 한다.

```cmake
add_library(core STATIC
  src/core.cpp
)

add_library(plugin MODULE
  src/plugin.cpp
)

add_library/shared_lib SHARED
  src/shared.cpp
)
```

add_library()는 library target을 추가한다.

STATIC은 object archive, SHARED는 동적 라이브러리, MODULE은 dlopen류로 로드되는 plugin 성격의 라이브러리다. type을 생략하면 BUILD_SHARED_LIBS 값에 따라 STATIC 또는 SHARED가 선택됩니다.

shared로 빌드되면 나중에 해당 라이브러리만 빌드에서 갈아끼울 수 있으므로 유동성이 좋아진다고 할 수 있다. 디버깅에 용이함..

근데 이제 library도 INTERFACE가 있다.

```cmake
add_library(project_options INTERFACE)

target_compile_features(project_options
  INTERFACE
    cxx_std_20
)

target_compile_options(project_options
  INTERFACE
    -Wall
    -Wextra
)

target_link_libraries(my_app
  PRIVATE
    project_options
)
```

INTERFACE library는 소스 파일을 컴파일하지 않고 디스크에 라이브러리 산출물도 만들지 않지만 include path, compile options, compile definitions, link options 같은 usage requirement를 다른 target에 전달할 때 쓴다.

test용 폴더에서는 빌드를 따로 돌려야할 수 있는데, 이걸 위한 중첩 방법이 있다.

```text
my_project/
  CMakeLists.txt
  CMakePresets.json
  cmake/
    MyProjectConfig.cmake.in
  include/
    my_project/
      math.hpp
  src/
    math.cpp
    main.cpp
  tests/
    CMakeLists.txt
    test_math.cpp
```

대충 이렇게 생겼다고 치면, 최상위 폴더의 CMakeLists가 아래같이 생겼을 것이다.

```cmake
cmake_minimum_required(VERSION 3.25)

project(MyProject
  VERSION 1.0.0
  DESCRIPTION "Modern CMake project"
  LANGUAGES CXX
)

option(MYPROJECT_BUILD_TESTS "Build tests" ON)

add_library(my_project
  src/math.cpp
)

add_library(MyProject::my_project ALIAS my_project)

target_include_directories(my_project
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(my_project
  PUBLIC
    cxx_std_20
)

add_executable(my_project_app
  src/main.cpp
)

target_link_libraries(my_project_app
  PRIVATE
    MyProject::my_project
)

if(MYPROJECT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
```

하위 디렉터리는 add_subdirectory()로 추가하는데 이러면 최상위 먼저 처리하고, 해당 test용 source, build 디렉토리도 생성한다.

## cmake 문법

이제 llm이 워낙 잘해주니까 문법을 꼭 알아야 하나 싶긴한데, 매번 물어볼 것도 아니고 다른 사람이 적어둔 건 어느정도 읽을 수 있어야한다고 생각한다.

```cmake
set(MY_NAME "hello")
message(STATUS "MY_NAME=${MY_NAME}")
```
CMake 변수값은 기본적으로 문자열이다. normal variable, cache variable, environment variable이 있고, normal variable은 function scope와 directory scope의 영향을 받는다. ${VAR}는 normal variable을 먼저 찾고, 없으면 cache entry를 찾아서 사용한다.

```cmake
set(SOURCES
  src/a.cpp
  src/b.cpp
  src/c.cpp
)

set(PATH_WITH_SPACE "/Users/me/My Project")
message(STATUS "${PATH_WITH_SPACE}") # 안전
```

줄바꿈으로 보이지만 내부적으로는 `;`가 붙어서 구분된다.  unquoted argument는 세미콜론에 의해 여러 인자로 나뉠 수 있으므로 공백이나 세미콜론이 섞인 값은 따옴표를 제대로 써야한다.

```cmake
if(WIN32)
  message(STATUS "Windows")
elseif(APPLE)
  message(STATUS "Apple platform")
elseif(UNIX)
  message(STATUS "Unix-like")
endif()
```

if()는 조건에 따라 명령 블록을 실행한다. ON, YES, TRUE, 1 등은 true로, OFF, NO, FALSE, 0, 빈 문자열, NOTFOUND, *-NOTFOUND 등은 false로 알아듣는다.


```cmake
foreach(src IN LISTS SOURCES)
  message(STATUS "source: ${src}")
endforeach()
```

foreach()는 list의 각 값에 대해 명령 블록을 반복하는데 IN LISTS, ITEMS, RANGE, ZIP_LISTS 같은 형태가 있다.

```cmake
function(add_warning_flags target) // 이름 인자 
  target_compile_options(${target}
    PRIVATE
      -Wall
      -Wextra
  )
endfunction()

add_warning_flags(my_app)
```

function()은 나중에 호출할 명령을 정의한다. 함수는 새 scope를 만들기 때문에 함수 안에서 set()한 변수는 기본적으로 바깥으로 영향을 주지 않는다.

```cmake
option(MYPROJECT_ENABLE_LOGGING "Enable logging" ON)
option(MYPROJECT_BUILD_TESTS "Build tests" ON)
```

option()은 사용자가 선택할 수 있는 boolean option을 제공한다. 초기값을 안 주면 기본값은 OFF인데, 사용은 아래처럼 할 수 있다.

`cmake -S . -B build -DMYPROJECT_ENABLE_LOGGING=OFF`

-D <var>=<value>는 CMake cache entry를 만들거나 갱신하며, 프로젝트의 기본값보다 우선처리된다.

include directory를 선언할 그냥 `include_directories(include)` 해버리고 싶은데 이거보다는 아래처럼 하는게 좋다.

```cmake
target_include_directories(my_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
```

target_include_directories()는 target compile 시 사용할 include directory를 지정하는데, PUBLIC은 target 자신의 INCLUDE_DIRECTORIES에 들어가고 consumer에게 전달되는 INTERFACE_INCLUDE_DIRECTORIES에도 들어간다. 

설치 가능한 패키지를 만들 때 dependency의 absolute include path를 INSTALL_INTERFACE에 박아 넣으면 relocation이 깨지므로 피해야 한다.

```cmake
add_library(my_lib)

target_sources(my_lib
  PRIVATE
    src/a.cpp
    src/b.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/my_lib/a.hpp
      include/my_lib/b.hpp
)
```

target_sources()는 이미 생성된 target에 source를 추가한다. 

## 디버깅

```cmake
message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
```

message()는 로그를 남기는 명령이고, STATUS, WARNING, FATAL_ERROR, DEBUG, TRACE 같은 mode가 있다.
fatal error는 작업 처리랑 generation을 중단시킨다.

get_target_property로 값 확인 가능하다.

```cmake
get_target_property(incs my_project INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "my_project interface includes: ${incs}")
```

get_target_property()는 target의 property 값을 변수에 저장하며, property가 없으면 <variable>-NOTFOUND로 설정한다.

