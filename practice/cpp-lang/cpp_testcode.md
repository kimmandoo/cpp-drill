# C++ 테스트 코드 작성과 실행법

C++에는 Java의 JUnit처럼 언어 표준에 포함된 테스트 프레임워크가 없다.

그래서 보통 아래 중 하나를 선택한다.

```text
아주 작은 학습 코드       -> assert
CMake 프로젝트 기본 테스트 -> CTest
실무형 단위 테스트         -> GoogleTest, Catch2, doctest
메모리 오류 탐지           -> AddressSanitizer, UndefinedBehaviorSanitizer
```

이 문서는 C++ 테스트 코드를 어떻게 작성하고, 어떻게 실행하고, 실패했을 때 어떻게 읽는지까지 정리한다.

---

## 1. 테스트 코드가 필요한 이유

C++은 컴파일러가 많은 문제를 잡아주지만, 다음 문제는 테스트 없이는 놓치기 쉽다.

- 함수 결과가 예상과 다른 경우
- 예외가 나야 하는데 안 나는 경우
- 경계값에서 틀리는 경우
- 포인터/참조 수명 문제
- vector 범위 밖 접근
- 복사/이동 중 객체 상태가 깨지는 경우
- 멀티스레드에서 data race가 생기는 경우

Java에서는 런타임이 NullPointerException, ArrayIndexOutOfBoundsException 같은 형태로 꽤 많은 문제를 명확히 보여준다. C++은 undefined behavior가 있어서 조용히 틀릴 수도 있다. 그래서 테스트와 sanitizer를 같이 쓰는 습관이 중요하다.

---

## 2. 테스트의 기본 모양: AAA

테스트 코드는 보통 세 단계로 쓴다.

```text
Arrange: 준비
Act    : 실행
Assert : 검증
```

예시:

```cpp
int add(int a, int b) {
    return a + b;
}

void testAdd() {
    // Arrange
    int a = 1;
    int b = 2;

    // Act
    int result = add(a, b);

    // Assert
    assert(result == 3);
}
```

테스트는 "출력해보고 눈으로 확인"하는 코드가 아니라, 프로그램이 자동으로 성공/실패를 판단하는 코드여야 한다.

잘못된 테스트:

```cpp
std::cout << add(1, 2) << "\n"; // 사람이 3인지 직접 봐야 함
```

좋은 테스트:

```cpp
assert(add(1, 2) == 3); // 틀리면 프로그램이 실패함
```

---

## 3. 테스트하기 쉬운 코드 구조

테스트하기 좋은 C++ 코드는 `main`에 로직을 다 넣지 않는다.

잘못된 코드:

```cpp
#include <iostream>

int main() {
    int a;
    int b;
    std::cin >> a >> b;
    std::cout << a + b << "\n";
}
```

이 코드는 입력/출력과 계산이 섞여 있어서 단위 테스트하기 어렵다.

좋은 코드:

```cpp
int add(int a, int b) {
    return a + b;
}

int main() {
    int a;
    int b;
    std::cin >> a >> b;
    std::cout << add(a, b) << "\n";
}
```

핵심은 이거다.

```text
비즈니스 로직은 함수/클래스로 빼고,
main은 입력/출력과 연결만 한다.
```

---

## 4. 예제 프로젝트 구조

테스트용으로 이런 구조를 생각하면 된다.

```text
calculator-project/
├── CMakeLists.txt
├── include/
│   └── calculator.h
├── src/
│   ├── calculator.cpp
│   └── main.cpp
└── tests/
    └── calculator_test.cpp
```

`include/calculator.h`

```cpp
#pragma once

int add(int a, int b);
int subtract(int a, int b);
int divide(int a, int b);
```

`src/calculator.cpp`

```cpp
#include "calculator.h"

#include <stdexcept>

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int divide(int a, int b) {
    if (b == 0) {
        throw std::invalid_argument("division by zero");
    }

    return a / b;
}
```

`src/main.cpp`

```cpp
#include "calculator.h"

#include <iostream>

int main() {
    std::cout << add(1, 2) << "\n";
}
```

테스트는 `main.cpp`를 테스트하지 않고, `calculator.cpp`의 함수를 테스트한다.

---

## 5. 가장 단순한 테스트: `assert`

C++ 표준 라이브러리의 `<cassert>`를 쓰면 의존성 없이 테스트를 만들 수 있다.

`tests/calculator_test.cpp`

```cpp
#include "calculator.h"

#include <cassert>
#include <stdexcept>

void testAdd() {
    assert(add(1, 2) == 3);
    assert(add(-1, 1) == 0);
    assert(add(0, 0) == 0);
}

void testSubtract() {
    assert(subtract(5, 3) == 2);
    assert(subtract(3, 5) == -2);
}

void testDivide() {
    assert(divide(10, 2) == 5);
    assert(divide(9, 2) == 4);
}

void testDivideByZeroThrows() {
    bool thrown = false;

    try {
        divide(10, 0);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }

    assert(thrown);
}

int main() {
    testAdd();
    testSubtract();
    testDivide();
    testDivideByZeroThrows();
}
```

컴파일:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic \
    -Iinclude \
    src/calculator.cpp tests/calculator_test.cpp \
    -o calculator_test
```

실행:

```bash
./calculator_test
```

성공하면 아무 출력 없이 종료된다. 실패하면 `assert` 메시지가 나오고 프로그램이 중단된다.

주의할 점이 있다. `assert`는 `NDEBUG`가 정의되면 사라진다.

```bash
g++ -std=c++17 -DNDEBUG ...
```

그래서 `assert`는 학습용/간단한 테스트에는 좋지만, 실무 테스트에는 GoogleTest 같은 프레임워크를 쓰는 편이 낫다.

---

## 6. 실패를 보기 쉬운 작은 테스트 러너

`assert`는 어느 테스트 함수가 실패했는지 보기 불편할 수 있다. 간단한 러너를 직접 만들 수도 있다.

```cpp
#include "calculator.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    void (*func)();
};

void expectEqual(int actual, int expected) {
    if (actual != expected) {
        throw std::runtime_error(
            "expected " + std::to_string(expected) +
            ", actual " + std::to_string(actual)
        );
    }
}

void testAdd() {
    expectEqual(add(1, 2), 3);
    expectEqual(add(-1, 1), 0);
}

void testDivideByZeroThrows() {
    try {
        divide(10, 0);
    } catch (const std::invalid_argument&) {
        return;
    }

    throw std::runtime_error("expected std::invalid_argument");
}

int main() {
    std::vector<TestCase> tests{
        {"testAdd", testAdd},
        {"testDivideByZeroThrows", testDivideByZeroThrows},
    };

    int failed = 0;

    for (const auto& test : tests) {
        try {
            test.func();
            std::cout << "[PASS] " << test.name << "\n";
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[FAIL] " << test.name << ": " << e.what() << "\n";
        }
    }

    if (failed > 0) {
        return 1;
    }

    return 0;
}
```

이 방식은 테스트 프레임워크의 아주 작은 버전이다.

장점:

- 의존성이 없다.
- 성공/실패 출력이 보인다.
- exit code로 CI나 script에서 실패를 알 수 있다.

단점:

- 기능이 적다.
- 실패 위치, fixture, parameterized test 등을 직접 만들어야 한다.

---

## 7. CMake로 테스트 빌드하기

C++ 프로젝트는 보통 CMake로 빌드한다. 테스트도 CMake target으로 만드는 게 좋다.

`CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)

project(CalculatorProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator
    src/calculator.cpp
)

target_include_directories(calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(calculator_app
    src/main.cpp
)

target_link_libraries(calculator_app PRIVATE
    calculator
)

add_executable(calculator_test
    tests/calculator_test.cpp
)

target_link_libraries(calculator_test PRIVATE
    calculator
)
```

빌드:

```bash
cmake -S . -B build
cmake --build build
```

테스트 실행:

```bash
./build/calculator_test
```

포인트:

- `calculator`를 library target으로 만든다.
- app과 test가 둘 다 `calculator`를 link한다.
- test executable에는 `src/main.cpp`를 넣지 않는다. main이 두 개가 되면 안 된다.

잘못된 코드:

```cmake
add_executable(calculator_test
    src/main.cpp
    src/calculator.cpp
    tests/calculator_test.cpp
)
```

`src/main.cpp`와 `tests/calculator_test.cpp` 둘 다 `main()`을 가지고 있으면 link 에러가 난다.

---

## 8. CTest로 테스트 등록하기

CTest는 CMake와 같이 쓰는 테스트 실행 도구다.

`CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)

project(CalculatorProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator
    src/calculator.cpp
)

target_include_directories(calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(calculator_app
    src/main.cpp
)

target_link_libraries(calculator_app PRIVATE
    calculator
)

enable_testing()

add_executable(calculator_test
    tests/calculator_test.cpp
)

target_link_libraries(calculator_test PRIVATE
    calculator
)

add_test(NAME calculator_test COMMAND calculator_test)
```

빌드:

```bash
cmake -S . -B build
cmake --build build
```

CTest 실행:

```bash
ctest --test-dir build --output-on-failure
```

출력 예시:

```text
Test project /path/calculator-project/build
    Start 1: calculator_test
1/1 Test #1: calculator_test ...............   Passed

100% tests passed, 0 tests failed out of 1
```

테스트가 실패했을 때 `--output-on-failure`를 붙이면 실패한 테스트의 출력이 보인다.

자주 쓰는 명령:

```bash
ctest --test-dir build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R calculator
ctest --test-dir build --verbose
```

의미:

```text
-R calculator: 이름에 calculator가 들어간 테스트만 실행
--verbose    : 더 자세한 실행 정보 출력
```

---

## 9. GoogleTest 사용하기

GoogleTest는 C++에서 가장 많이 쓰는 테스트 프레임워크 중 하나다.

장점:

- `EXPECT_EQ`, `ASSERT_EQ` 같은 검증 매크로 제공
- 예외 검증 제공
- fixture 제공
- parameterized test 제공
- CMake와 잘 붙음

### CMake에서 GoogleTest 가져오기

인터넷이 가능한 환경이면 `FetchContent`로 가져올 수 있다.

`CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)

project(CalculatorProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator
    src/calculator.cpp
)

target_include_directories(calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(calculator_app
    src/main.cpp
)

target_link_libraries(calculator_app PRIVATE
    calculator
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(calculator_gtest
    tests/calculator_gtest.cpp
)

target_link_libraries(calculator_gtest PRIVATE
    calculator
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(calculator_gtest)
```

빌드:

```bash
cmake -S . -B build
cmake --build build
```

테스트 실행:

```bash
ctest --test-dir build --output-on-failure
```

또는 테스트 실행 파일 직접 실행:

```bash
./build/calculator_gtest
```

---

## 10. GoogleTest 기본 테스트 코드

`tests/calculator_gtest.cpp`

```cpp
#include "calculator.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(CalculatorTest, AddPositiveNumbers) {
    EXPECT_EQ(add(1, 2), 3);
    EXPECT_EQ(add(10, 20), 30);
}

TEST(CalculatorTest, AddNegativeNumbers) {
    EXPECT_EQ(add(-1, -2), -3);
    EXPECT_EQ(add(-1, 1), 0);
}

TEST(CalculatorTest, SubtractNumbers) {
    EXPECT_EQ(subtract(5, 3), 2);
    EXPECT_EQ(subtract(3, 5), -2);
}

TEST(CalculatorTest, DivideNumbers) {
    EXPECT_EQ(divide(10, 2), 5);
    EXPECT_EQ(divide(9, 2), 4);
}

TEST(CalculatorTest, DivideByZeroThrows) {
    EXPECT_THROW(divide(10, 0), std::invalid_argument);
}
```

GoogleTest의 `TEST`는 이렇게 읽으면 된다.

```cpp
TEST(테스트그룹이름, 테스트케이스이름) {
    검증 코드
}
```

실행하면 대략 이런 출력이 나온다.

```text
[ RUN      ] CalculatorTest.AddPositiveNumbers
[       OK ] CalculatorTest.AddPositiveNumbers
[ RUN      ] CalculatorTest.DivideByZeroThrows
[       OK ] CalculatorTest.DivideByZeroThrows
```

---

## 11. `EXPECT_*`와 `ASSERT_*` 차이

GoogleTest에는 `EXPECT_*`와 `ASSERT_*`가 있다.

```text
EXPECT_*: 실패해도 현재 테스트 함수를 계속 실행
ASSERT_*: 실패하면 현재 테스트 함수를 즉시 중단
```

예시:

```cpp
TEST(UserTest, FindUser) {
    const User* user = findUser(1);

    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->id(), 1);
    EXPECT_EQ(user->name(), "kim");
}
```

여기서 `ASSERT_NE(user, nullptr)`를 쓰는 이유는, user가 null이면 아래에서 `user->id()`를 호출하면 안 되기 때문이다.

잘못된 코드:

```cpp
TEST(UserTest, FindUser) {
    const User* user = findUser(1);

    EXPECT_NE(user, nullptr);
    EXPECT_EQ(user->id(), 1); // user가 nullptr이면 테스트 중 crash 가능
}
```

포인터가 null인지 확인한 뒤 계속 역참조해야 한다면 `ASSERT_*`를 쓰는 게 좋다.

---

## 12. 자주 쓰는 GoogleTest 매크로

```cpp
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

EXPECT_EQ(actual, expected);
EXPECT_NE(actual, expected);
EXPECT_LT(a, b);
EXPECT_LE(a, b);
EXPECT_GT(a, b);
EXPECT_GE(a, b);

EXPECT_THROW(statement, ExceptionType);
EXPECT_NO_THROW(statement);

EXPECT_NEAR(actual, expected, abs_error);
```

문자열:

```cpp
std::string name = "kim";
EXPECT_EQ(name, "kim");
```

부동소수점:

```cpp
double result = 0.1 + 0.2;
EXPECT_NEAR(result, 0.3, 1e-9);
```

잘못된 코드:

```cpp
EXPECT_EQ(0.1 + 0.2, 0.3); // 부동소수점 오차 때문에 실패할 수 있음
```

좋은 코드:

```cpp
EXPECT_NEAR(0.1 + 0.2, 0.3, 1e-9);
```

---

## 13. 예외 테스트

예외가 발생해야 하는 경우:

```cpp
TEST(CalculatorTest, DivideByZeroThrows) {
    EXPECT_THROW(divide(10, 0), std::invalid_argument);
}
```

예외가 발생하면 안 되는 경우:

```cpp
TEST(CalculatorTest, DivideNormalValueDoesNotThrow) {
    EXPECT_NO_THROW(divide(10, 2));
}
```

예외 메시지까지 확인하고 싶으면 직접 try-catch를 쓴다.

```cpp
TEST(CalculatorTest, DivideByZeroMessage) {
    try {
        divide(10, 0);
        FAIL() << "expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "division by zero");
    }
}
```

`FAIL()`은 여기까지 오면 테스트 실패라는 뜻이다.

---

## 14. fixture

여러 테스트가 같은 준비 코드를 공유하면 fixture를 쓴다.

예시 클래스:

```cpp
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

struct User {
    int id;
    std::string name;
};

class UserRepository {
public:
    void add(User user) {
        users_.push_back(std::move(user));
    }

    std::optional<User> findById(int id) const {
        for (const auto& user : users_) {
            if (user.id == id) {
                return user;
            }
        }

        return std::nullopt;
    }

private:
    std::vector<User> users_;
};
```

fixture 테스트:

```cpp
#include <gtest/gtest.h>

class UserRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo.add(User{1, "kim"});
        repo.add(User{2, "lee"});
    }

    UserRepository repo;
};

TEST_F(UserRepositoryTest, FindExistingUser) {
    auto user = repo.findById(1);

    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->id, 1);
    EXPECT_EQ(user->name, "kim");
}

TEST_F(UserRepositoryTest, ReturnNulloptWhenUserDoesNotExist) {
    auto user = repo.findById(999);

    EXPECT_FALSE(user.has_value());
}
```

`TEST_F`는 fixture를 쓰는 테스트다.

```text
TEST   : fixture 없는 일반 테스트
TEST_F : fixture를 사용하는 테스트
```

각 테스트마다 fixture 객체가 새로 만들어진다. 한 테스트에서 바꾼 상태가 다른 테스트로 새지 않는 게 기본이다.

---

## 15. parameterized test

입력/기대값만 다르고 같은 테스트를 반복하고 싶으면 parameterized test를 쓴다.

```cpp
#include "calculator.h"

#include <gtest/gtest.h>

struct AddCase {
    int a;
    int b;
    int expected;
};

class AddTest : public ::testing::TestWithParam<AddCase> {};

TEST_P(AddTest, AddValues) {
    AddCase param = GetParam();

    EXPECT_EQ(add(param.a, param.b), param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    CalculatorCases,
    AddTest,
    ::testing::Values(
        AddCase{1, 2, 3},
        AddCase{-1, 1, 0},
        AddCase{0, 0, 0}
    )
);
```

테스트 케이스가 많아질수록 중복을 줄일 수 있다.

---

## 16. private 함수는 테스트해야 할까

보통 private 함수 자체를 직접 테스트하지 않는다.

```text
public 함수의 동작을 테스트해서 private 구현도 간접 검증한다.
```

private 함수를 억지로 테스트하고 싶어진다면 둘 중 하나일 수 있다.

- private 함수가 너무 복잡하다.
- 별도 클래스로 분리할 수 있는 로직이다.

예를 들어 문자열 파싱 로직이 private에 너무 길게 있다면:

```cpp
class ConfigLoader {
public:
    Config load(const std::string& path);

private:
    Config parseLine(const std::string& line);
};
```

이런 경우 `parseLine`을 별도 함수나 클래스로 빼서 테스트하기 쉽게 만들 수 있다.

```cpp
ConfigEntry parseConfigLine(const std::string& line);
```

테스트는 설계를 좋게 만드는 압력으로 쓰면 된다.

---

## 17. dependency injection과 test double

현재 시간, 파일 시스템, 네트워크처럼 외부 환경에 의존하면 테스트가 어려워진다.

잘못된 코드:

```cpp
#include <chrono>

class Coupon {
public:
    bool isExpired() const {
        auto now = std::chrono::system_clock::now();
        return now > expiresAt_;
    }

private:
    std::chrono::system_clock::time_point expiresAt_;
};
```

테스트할 때 현재 시간을 제어할 수 없다.

좋은 코드:

```cpp
#include <chrono>

class Clock {
public:
    virtual ~Clock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
};

class SystemClock : public Clock {
public:
    std::chrono::system_clock::time_point now() const override {
        return std::chrono::system_clock::now();
    }
};

class Coupon {
public:
    Coupon(std::chrono::system_clock::time_point expiresAt, const Clock& clock)
        : expiresAt_(expiresAt), clock_(clock) {}

    bool isExpired() const {
        return clock_.now() > expiresAt_;
    }

private:
    std::chrono::system_clock::time_point expiresAt_;
    const Clock& clock_;
};
```

테스트용 fake:

```cpp
class FakeClock : public Clock {
public:
    explicit FakeClock(std::chrono::system_clock::time_point value)
        : value_(value) {}

    std::chrono::system_clock::time_point now() const override {
        return value_;
    }

private:
    std::chrono::system_clock::time_point value_;
};
```

테스트:

```cpp
TEST(CouponTest, ExpiredWhenNowIsAfterExpireTime) {
    using ClockType = std::chrono::system_clock;

    auto expiresAt = ClockType::time_point{} + std::chrono::hours(24);
    FakeClock clock{ClockType::time_point{} + std::chrono::hours(25)};

    Coupon coupon{expiresAt, clock};

    EXPECT_TRUE(coupon.isExpired());
}
```

이런 식으로 외부 의존성을 밖에서 주입하면 테스트가 쉬워진다.

---

## 18. 파일을 다루는 테스트

파일 테스트는 임시 디렉토리를 쓰고, 테스트가 끝나면 지우는 게 좋다.

예제 함수:

```cpp
#include <fstream>
#include <string>

void writeText(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    out << text;
}
```

테스트:

```cpp
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

TEST(FileTest, WriteTextCreatesFile) {
    namespace fs = std::filesystem;

    fs::path path = fs::temp_directory_path() / "cpp_test_write_text.txt";

    writeText(path.string(), "hello");

    std::ifstream in(path);
    std::string content;
    std::getline(in, content);

    EXPECT_EQ(content, "hello");

    fs::remove(path);
}
```

C++17의 `std::filesystem`을 사용한다.

테스트 파일 이름이 겹치면 병렬 테스트에서 문제가 될 수 있다. 실제 프로젝트에서는 랜덤 이름이나 테스트별 고유 디렉토리를 쓰는 게 좋다.

---

## 19. 메모리 오류를 잡는 테스트: sanitizer

테스트가 통과해도 메모리 오류가 숨어 있을 수 있다. AddressSanitizer를 켜면 use-after-free, heap-buffer-overflow 같은 문제를 잘 잡는다.

예제 버그:

```cpp
int readOutOfBounds() {
    int* values = new int[3]{1, 2, 3};
    int result = values[10];
    delete[] values;
    return result;
}
```

컴파일:

```bash
g++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer \
    bug.cpp -o bug_test
```

실행:

```bash
./bug_test
```

CMake에서 sanitizer 켜기:

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"

cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

자주 쓰는 sanitizer:

```text
address   : heap/stack buffer overflow, use after free 등
undefined : undefined behavior 일부 탐지
thread    : data race 탐지
```

ThreadSanitizer:

```bash
cmake -S . -B build-tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread"

cmake --build build-tsan
ctest --test-dir build-tsan --output-on-failure
```

AddressSanitizer와 ThreadSanitizer는 보통 같이 켜지 않는다. 빌드 디렉토리를 따로 만드는 게 좋다.

---

## 20. 테스트 빌드 타입

디버깅 가능한 테스트:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

최적화 빌드 테스트:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

학습할 때는 보통 Debug + sanitizer 조합이 제일 좋다.

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
```

---

## 21. 경계값 테스트

테스트는 정상 케이스만 쓰면 부족하다.

예를 들어 clamp 함수가 있다고 하자.

```cpp
int clamp(int value, int min, int max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}
```

테스트:

```cpp
TEST(ClampTest, ReturnValueWhenInsideRange) {
    EXPECT_EQ(clamp(5, 0, 10), 5);
}

TEST(ClampTest, ReturnMinWhenBelowRange) {
    EXPECT_EQ(clamp(-1, 0, 10), 0);
}

TEST(ClampTest, ReturnMaxWhenAboveRange) {
    EXPECT_EQ(clamp(11, 0, 10), 10);
}

TEST(ClampTest, ReturnBoundaryValues) {
    EXPECT_EQ(clamp(0, 0, 10), 0);
    EXPECT_EQ(clamp(10, 0, 10), 10);
}
```

경계값은 버그가 많이 나는 자리다.

```text
빈 값
하나만 있는 값
최솟값
최댓값
범위 바로 안쪽
범위 바로 바깥쪽
0
음수
중복 값
정렬된 값
정렬되지 않은 값
```

---

## 22. 테스트 이름 짓기

좋은 테스트 이름은 실패했을 때 무슨 문제가 났는지 바로 보인다.

나쁜 이름:

```cpp
TEST(CalculatorTest, Test1) {}
TEST(CalculatorTest, Add) {}
```

좋은 이름:

```cpp
TEST(CalculatorTest, AddReturnsSumOfTwoPositiveNumbers) {}
TEST(CalculatorTest, DivideThrowsWhenDivisorIsZero) {}
```

너무 길어도 문제지만, 실패 메시지만 보고 의도를 알 수 있을 정도면 좋다.

---

## 23. 테스트에서 피해야 할 것

### 1. 테스트끼리 순서에 의존

잘못된 코드:

```cpp
int globalValue = 0;

TEST(OrderTest, First) {
    globalValue = 10;
}

TEST(OrderTest, Second) {
    EXPECT_EQ(globalValue, 10); // First가 먼저 실행된다는 보장이 없음
}
```

좋은 테스트는 각각 독립적이어야 한다.

### 2. 너무 많은 것을 한 테스트에서 검증

잘못된 코드:

```cpp
TEST(UserServiceTest, Everything) {
    // create
    // update
    // delete
    // search
    // file write
    // network call
}
```

실패했을 때 원인을 찾기 어렵다.

### 3. 랜덤 값을 고정하지 않음

```cpp
std::random_device rd;
std::mt19937 gen(rd());
```

테스트가 매번 달라지면 실패 재현이 어렵다.

좋은 코드:

```cpp
std::mt19937 gen(1234);
```

### 4. 실제 시간에 의존

```cpp
std::this_thread::sleep_for(std::chrono::seconds(1));
```

테스트가 느리고 불안정해진다. 가능하면 fake clock을 쓴다.

---

## 24. 테스트 실행 자동화 스크립트

간단히 script로 묶을 수 있다.

`run_tests.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

sanitizer까지:

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"

cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

실행 권한:

```bash
chmod +x run_tests.sh
./run_tests.sh
```

---

## 25. 현재 프로젝트에서 적용하는 방식

이 repo처럼 CMake 학습과 C++ 학습 코드가 섞여 있다면, 테스트 대상 코드를 `main.cpp`에서 분리하는 것이 첫 단계다.

예를 들어:

```text
practice/cpp-cmake-study/02-multi-file/
├── calculator.h
├── calculator.cpp
├── main.cpp
└── tests/
    └── calculator_test.cpp
```

테스트 CMake 예시:

```cmake
cmake_minimum_required(VERSION 3.16)

project(MultiFileWithTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator
    calculator.cpp
)

target_include_directories(calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(app
    main.cpp
)

target_link_libraries(app PRIVATE
    calculator
)

enable_testing()

add_executable(calculator_test
    tests/calculator_test.cpp
)

target_link_libraries(calculator_test PRIVATE
    calculator
)

add_test(NAME calculator_test COMMAND calculator_test)
```

테스트 파일:

```cpp
#include "calculator.h"

#include <cassert>

int main() {
    assert(add(1, 2) == 3);
    assert(add(-1, 1) == 0);
}
```

실행:

```bash
cmake -S practice/cpp-cmake-study/02-multi-file \
    -B practice/cpp-cmake-study/02-multi-file/build-test

cmake --build practice/cpp-cmake-study/02-multi-file/build-test

ctest --test-dir practice/cpp-cmake-study/02-multi-file/build-test \
    --output-on-failure
```

---

## 26. 테스트 작성 체크리스트

테스트를 만들 때 아래를 확인한다.

```text
main에 로직이 몰려 있지 않은가?
테스트 대상 함수/클래스가 입력과 출력을 명확히 갖는가?
정상 케이스가 있는가?
경계값 케이스가 있는가?
예외/실패 케이스가 있는가?
테스트끼리 순서에 의존하지 않는가?
외부 시간/파일/네트워크에 직접 의존하지 않는가?
실패하면 exit code가 0이 아닌가?
CTest에서 실행 가능한가?
sanitizer 빌드로도 돌려봤는가?
```

---

## 27. 정리

C++ 테스트는 단계적으로 익히면 된다.

```text
1. 로직을 함수/클래스로 분리
2. assert로 가장 작은 테스트 작성
3. CMake target으로 test executable 추가
4. CTest에 등록
5. GoogleTest로 EXPECT/ASSERT 기반 테스트 작성
6. Debug + sanitizer 빌드로 메모리 오류까지 확인
```

처음부터 거대한 테스트 프레임워크를 완벽히 쓰려고 하기보다, "내 코드가 자동으로 맞다/틀리다를 말하게 만든다"는 감각을 잡는 게 먼저다.
