https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl08.html

이제 kaledoscope를 object 파일로 컴파일한다.

컴파일해서 cpp랑 링크해서 쓸 수 있도록 만드는 것이다.

## 타깃 선택하기

컴파일 타겟은 target triple이라는 문자열 그룹을 사용하게 된다.

`<arch><sub>-<vendor>-<sys>-<abi>`

llvm이 크로스 컴파일을 지원하고 있어서 다양한 환경에 맞춰 컴파일 타겟을 잡을 수 있다.

```bash
mgkim@DESKTOP-94NFG02:~/toy-llvm$ clang --version
Ubuntu clang version 18.1.8 (++20240731025043+3b5b5c1ec4a3-1~exp1~20240731145144.92)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin
```

Target이 target triple이다.

근데 target triple을 직접 칠 필요는 없고, llvm이 sys::getDefaultTargetTriple()을 제공하니까 가져다 쓰면 된다.

llvm은 필요한 타깃만 링크해서 쓸 수 있다.

```cpp
InitializeAllTargetInfos(); // llvm 지원 타깃 정보 등록
InitializeAllTargets(); // 실제 타겟 구현
InitializeAllTargetMCs(); // MC는 MachineCode로, 어셈블러, 오브젝트 파일 쪽
InitializeAllAsmParsers(); // 어셈블리 파서
InitializeAllAsmPrinters(); // llvm 내부나 기계어를 어셈블리 형태로 출력
```

오브젝트 코드 생성 시 타깃 초기화를 해놔야하는 데 이때 초기화의 의미는 llvm내부에 어떤 타깃들이 존재하는 지 정보를 등록하는 과정이다.

```cpp
std::string Error;
auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

// 요청한 target을 찾을 수 없다면 에러를 출력하고 종료한다.
// 보통 TargetRegistry 초기화를 잊었거나,
// 잘못된 target triple을 사용했을 때 발생한다.
if (!Target) {
  errs() << Error;
  return 1;
}
```
TargetRegistry::lookupTarget()은 target triple 문자열을 보고 LLVM 내부에 등록된 타깃 백엔드를 찾는다.

타겟 초기화에 내가 찾아야하는 타겟이 없으면 lookupTarget이 실패하고 Target이 nullptr가 될 수 있다.

## Target Machine

LLVM이 어떤 CPU와 기능을 알고 있는지 보려면 llc를 사용할 수 있다.

`llvm-as < /dev/null | llc -march=x86 -mattr=help` 이거로하면  Available CPUs for this target, Available features for this target처럼 정보가 쭉 뜸.

```text
Available CPUs for this target:

  alderlake               - Select the alderlake processor.
  amdfam10                - Select the amdfam10 processor.
  arrowlake               - Select the arrowlake processor.
  arrowlake-s             - Select the arrowlake-s processor
```

Target이 x86_64같은 아키텍쳐를 의미하면, 타겟머신은 좀 더 구체적인.

타겟머신에 구체적인 cpu 이름같은걸 넣으면 최적화가 될 수 있지만 그냥 generic으로 넣어도 된다.

```cpp
auto CPU = "generic";
auto Features = "";

TargetOptions opt;
auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, Reloc::PIC_);
```
generic은 특정 CPU 최적화를 하지 않고 일반적인 CPU를 대상으로 한다는 의미다.

PIC는 위치 독립코드로, 코드가 메모리 어디에 로드 되더라도 동작할 수 있게 만드는 방식이다. 공유 라이브러리나 동적 링크 환경에서 중요하다.

## 8.4. Module 설정하기

```cpp
TheModule->setDataLayout(TargetMachine->createDataLayout());
TheModule->setTargetTriple(TargetTriple);
```

타겟과 데이터레이아웃을 지정해주는 게 필수는 아닌데 가이드 권장사항이다. 최적화가 더 잘 된다는 장점도 있다.

LLVM의 Module은 하나의 번역 단위처럼 볼 수 있다. 코드를 컴파일해서 나온 LLVM IR묶음 같은 거라고 생각하면 됨.

참고로 데이터 레이아웃은

- 포인터 크기
- 정수/실수 타입의 정렬 방식
- 엔디언
- 구조체 필드 정렬
- 타입별 ABI 규칙

이런 걸 선언해두는데, 32비트랑 64비트의 포인터 크기가 2배차이나는 것과 같은 정보를 LLVM이 모르면 최적화나 코드 생성이 꼬일 수 있다.

지금 타겟 머신의 데이터 레이아웃으로 모듈 데이터레이아웃을 지정해주는 코드다.

## 오브젝트 코드 생성

```cpp
auto Filename = "output.o";
std::error_code EC;
raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

if (EC) {
  errs() << "Could not open file: " << EC.message();
  return 1;
}
```

raw_fd_ostream는 llvm에서 제공하는 파일 출력 스트림이다. EC는 에러코드.

```cpp
legacy::PassManager pass;
auto FileType = CodeGenFileType::ObjectFile;

if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
  errs() << "TargetMachine can't emit a file of this type";
  return 1;
}

pass.run(*TheModule);
dest.flush();
```

오브젝트 코드를 생성하는 pass를 정의하고, 실행하면 된다. 이전 단계에서 학습했듯이, pass는 IR을 분석하거나, 변환하거나 코드를 생성하는 작업 단위다.

`auto FileType = CodeGenFileType::ObjectFile;`로 출력 파일 설정해주고 타겟 머신한테 object 파일 생성하는 pass들을 pass manager에 추가해달라는 코드다.

이후 pass를 실행하고, 파일출력 버퍼를 비워서 디스크에 기록한다.

이제 코드를 실행할건데 오브젝트를 생성하기 떄문에 llvm-config에 전달하는 인자가 다르다.

target backend, object emission, asm printer 같은 게 필요해서 `--libs all` 옵션을 사용한다.

LLVM 18에서는 `Module::setTargetTriple()`과 `Target::createTargetMachine()`이 `Triple` 객체가 아니라 `StringRef` 형태의 target triple 문자열을 받는다. 그래서 `Triple(TargetTriple)`처럼 감싸서 넘기면 컴파일 에러가 나고, `TargetTriple` 문자열 자체를 직접 넘겨야 한다.


이제 실습을 해보면

`clang++ -g -O3 my_llvm.cpp `llvm-config --cxxflags --ldflags --system-libs --libs all` -o toy`

이거로 toy를 만들어서 kaleidoscope 언어로 함수를 정의한다.

```bash
mgkim@DESKTOP-94NFG02:~/toy-llvm$ ./toy
ready> def average(x y) (x + y) * 0.5;
Read function definition:define double @average(double %x, double %y) {
entry:
  %y2 = alloca double, align 8
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  store double %y, ptr %y2, align 8
  %x3 = load double, ptr %x1, align 8
  %y4 = load double, ptr %y2, align 8
  %addtmp = fadd double %x3, %y4
  %multmp = fmul double %addtmp, 5.000000e-01
  ret double %multmp
}

Wrote output.o
```
정의 하고 ctrl+d로 인터프리터를 나오면 버퍼에서 읽은 걸 디스크로 써주면서 output.o 가 생성이 된다.

이걸 외부 cpp 코드로 링크해서 써볼 것이다.

```cpp
#include <iostream>

extern "C" {
    double average(double, double);
}

int main() {
    std::cout << "average of 3.0 and 4.0: " << average(3.0, 4.0) << std::endl;
}
```

함수 스타일이 c스타일이라 extern으로 mangling을 피해줬다.

컴파일은 `clang++ main.cpp output.o -o main` 이렇게.


근데 CMakeList 만드는 거 공부했으니까 작업해봤다.

```cmake
cmake_minimum_required(VERSION 3.16)

project(toy_llvm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output.o
)
```

main 실행 파일을 만들면서 main.cpp 랑 output.o파일을 묶어버림

cmake -S . -B build && cmake --build build 로 하면 된다.

output.o까지 CMake가 직접 만들게 하려면, `toy`를 먼저 빌드하고 그 실행 결과로 `build/output.o`를 생성한 다음 `main`에 링크하면 된다. 그러면 사람이 `./toy`를 따로 실행하지 않아도 `cmake --build build` 한 번으로 끝난다.

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

add_executable(toy my_llvm.cpp)
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

add_executable(main main.cpp ${GENERATED_OBJECT})
add_dependencies(main generate_output)
```

이 버전은 `output.o`를 build 디렉터리에 만들고, 그 파일을 `main`과 링크한다. `kaleidoscope_input.txt`에 들어 있는 함수 정의를 기준으로 `toy`가 오브젝트를 생성한다.