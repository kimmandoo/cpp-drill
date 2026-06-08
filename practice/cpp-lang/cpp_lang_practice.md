# C++ 문법 전체 정리 - Java 개발자 관점

C++은 Java랑 비슷해 보이는 문법이 많지만, 실제 사고방식은 꽤 다르다.

Java는 객체가 대부분 heap에 있고 GC가 수명을 관리한다. C++은 객체가 stack에도, heap에도, 전역 영역에도 직접 존재할 수 있고, 객체 수명이 스코프와 생성자/소멸자로 결정된다.

그래서 C++을 배울 때 가장 중요한 기준은 이거다.

```text
Java: 참조(reference)를 주고받고, GC가 나중에 정리한다.
C++ : 값(value) 자체를 주고받을 수 있고, 소유권과 수명을 코드가 명확히 정한다.
```

이 문서는 C++ 문법을 Java와 계속 비교하면서 본다. 예시 코드는 "좋은 코드"와 "잘못된 코드"를 같이 둔다.

주의: "잘못된 코드"는 컴파일 에러, 메모리 오류, undefined behavior를 설명하기 위한 코드다. 그대로 실행하지 않는 게 좋다.

---

## 1. C++ 프로그램의 기본 구조

### Java와 비교

Java는 보통 class 안에 `main`이 있다.

```java
public class Main {
    public static void main(String[] args) {
        System.out.println("hello");
    }
}
```

C++은 전역 함수 `main`이 프로그램 시작점이다.

```cpp
#include <iostream>

int main() {
    std::cout << "hello\n";
    return 0;
}
```

### 핵심 문법

- `#include <iostream>`: Java의 `import`와 비슷해 보이지만 다르다. C++의 include는 파일 내용을 가져오는 전처리 기능이다.
- `std::cout`: 표준 라이브러리의 출력 객체다.
- `::`: namespace 안의 이름을 접근하는 범위 지정 연산자다.
- `return 0`: 프로그램이 정상 종료됐다는 뜻이다. `main`에서는 생략해도 0으로 처리된다.

### 잘못된 코드

```cpp
#include <iostream>

public class Main { // C++에는 Java식 public class Main 문법이 없다.
    public static void main(String[] args) {
        std::cout << "hello\n";
    }
}
```

```cpp
#include <iostream>

int main() {
    cout << "hello\n"; // std::cout이거나 using 선언이 필요하다.
}
```

좋은 방식은 명확하게 `std::`를 붙이는 것이다.

```cpp
#include <iostream>

int main() {
    std::cout << "hello\n";
}
```

---

## 2. 컴파일과 실행

Java는 보통 `javac`로 bytecode를 만들고 JVM에서 실행한다.

```bash
javac Main.java
java Main
```

C++은 컴파일러가 native 실행 파일을 만든다.

```bash
g++ -std=c++17 main.cpp -o app
./app
```

여러 파일이면 직접 다 넘겨야 한다.

```bash
g++ -std=c++17 main.cpp calculator.cpp user.cpp -o app
```

파일이 많아지면 CMake 같은 빌드 도구를 쓴다.

### 잘못된 코드/명령

```bash
g++ main.cpp -o app
```

위 명령은 단순한 경우에는 되지만, 사용하는 C++ 표준이 애매하다. 학습할 때는 표준을 명시하는 게 좋다.

```bash
g++ -std=c++17 main.cpp -o app
```

---

## 3. 주석, 세미콜론, 블록

C++ 주석은 Java와 거의 같다.

```cpp
// 한 줄 주석

/*
여러 줄 주석
*/

int main() {
    int x = 10;
    return 0;
}
```

문장 끝에는 세미콜론을 붙인다.

### 잘못된 코드

```cpp
int main() {
    int x = 10 // 세미콜론이 없다.
    return 0;
}
```

클래스/구조체 선언 뒤에도 세미콜론이 필요하다.

```cpp
struct Person {
    int age;
}; // 필요함
```

잘못된 코드:

```cpp
struct Person {
    int age;
} // 세미콜론 없음
```

---

## 4. 변수와 기본 타입

### Java와 비교

Java:

```java
int age = 20;
long count = 100L;
double score = 3.14;
boolean ok = true;
char ch = 'A';
String name = "kim";
```

C++:

```cpp
#include <string>

int age = 20;
long long count = 100LL;
double score = 3.14;
bool ok = true;
char ch = 'A';
std::string name = "kim";
```

### C++ 기본 타입

```cpp
bool b = true;
char c = 'A';
int i = 10;
long l = 10L;
long long ll = 10LL;
float f = 3.14f;
double d = 3.14;
```

C++에서 `long`의 크기는 플랫폼마다 다를 수 있다. 정확한 크기가 필요하면 `<cstdint>`를 쓴다.

```cpp
#include <cstdint>

std::int32_t a = 10;
std::int64_t b = 20;
std::uint64_t c = 30;
```

### 초기화 방식

C++은 초기화 문법이 여러 개 있다.

```cpp
int a = 10;     // copy initialization
int b(10);      // direct initialization
int c{10};      // brace initialization
int d{};        // 0으로 초기화
```

학습할 때는 `{}` 초기화가 안전하다. 좁아지는 변환을 막아준다.

```cpp
int a = 3.14;  // 가능하지만 3으로 잘림
int b{3.14};   // 컴파일 에러
```

### Java와 다른 매우 중요한 점

C++ 지역 변수는 자동으로 0이 되지 않는다.

```cpp
int main() {
    int x; // 초기화 안 됨
    std::cout << x << "\n"; // 잘못된 코드: 쓰레기 값, undefined behavior 가능
}
```

Java에서는 필드는 기본값이 있지만, 지역 변수는 초기화 없이 사용하면 컴파일 에러다.

C++에서는 primitive 지역 변수를 읽는 순간 위험해진다.

좋은 코드:

```cpp
int main() {
    int x{}; // 0
    std::cout << x << "\n";
}
```

---

## 5. `auto`

Java의 `var`와 비슷하게 타입을 추론한다.

```cpp
auto age = 20;              // int
auto name = std::string{"kim"};
auto pi = 3.14;             // double
```

하지만 `auto`는 반드시 초기값이 있어야 한다.

잘못된 코드:

```cpp
auto x; // 어떤 타입인지 알 수 없다.
```

주의할 점:

```cpp
const int n = 10;
auto a = n;        // int, const가 복사 과정에서 떨어짐
const auto b = n;  // const int
```

참조를 유지하려면 `auto&`를 써야 한다.

```cpp
int x = 10;
auto y = x;   // 복사
auto& r = x;  // 참조

y = 20;       // x는 그대로 10
r = 30;       // x가 30으로 바뀜
```

---

## 6. `const`, `constexpr`

Java의 `final`과 비슷하게 "바꿀 수 없음"을 표현한다.

```cpp
const int maxCount = 10;
// maxCount = 20; // 컴파일 에러
```

`constexpr`는 컴파일 시간에 계산 가능한 상수라는 뜻이다.

```cpp
constexpr int square(int x) {
    return x * x;
}

constexpr int n = square(5); // 컴파일 시간에 25
```

### `const` 포인터 문법

C++에서 `const`는 포인터와 만나면 헷갈린다.

```cpp
int x = 10;
int y = 20;

const int* p1 = &x; // p1이 가리키는 값을 못 바꿈
p1 = &y;            // p1 자체는 다른 주소를 가리킬 수 있음

int* const p2 = &x; // p2 자체가 다른 주소를 못 가리킴
*p2 = 30;           // 가리키는 값은 바꿀 수 있음

const int* const p3 = &x; // 주소도 못 바꾸고 값도 못 바꿈
```

잘못된 코드:

```cpp
int x = 10;
const int* p = &x;
*p = 20; // 컴파일 에러: const int를 바꾸려고 함
```

---

## 7. 값, 포인터, 참조

이 부분이 Java 개발자에게 제일 중요하다.

Java 객체 변수는 객체 자체가 아니라 참조다.

```java
Person p = new Person();
```

C++은 객체 자체를 변수에 둘 수 있다.

```cpp
Person p;                  // 객체가 여기 직접 존재
Person* ptr = new Person;  // heap 객체의 주소
```

### 값

```cpp
int x = 10;
int y = x; // 복사
y = 20;

std::cout << x << "\n"; // 10
```

### 포인터

포인터는 주소를 저장하는 변수다.

```cpp
int x = 10;
int* p = &x;

std::cout << p << "\n";   // 주소
std::cout << *p << "\n";  // 주소가 가리키는 값, 10
```

포인터는 `nullptr`이 될 수 있고, 다른 대상을 가리킬 수 있다.

```cpp
int x = 10;
int y = 20;

int* p = &x;
p = &y;
p = nullptr;
```

잘못된 코드:

```cpp
int* p = nullptr;
std::cout << *p << "\n"; // nullptr 역참조, undefined behavior
```

### 참조

참조는 기존 객체의 별명이다.

```cpp
int x = 10;
int& r = x;

r = 20;
std::cout << x << "\n"; // 20
```

참조는 반드시 초기화해야 하고, 나중에 다른 대상을 가리키도록 바꿀 수 없다.

잘못된 코드:

```cpp
int& r; // 컴파일 에러: 참조는 반드시 초기화해야 함
```

헷갈리는 코드:

```cpp
int x = 10;
int y = 20;

int& r = x;
r = y; // r이 y를 가리키게 되는 게 아니라, x에 y 값을 대입함

std::cout << x << "\n"; // 20
```

### 함수 인자에서 값/참조/포인터 차이

값 전달:

```cpp
void change(int x) {
    x = 20;
}

int main() {
    int a = 10;
    change(a);
    std::cout << a << "\n"; // 10
}
```

참조 전달:

```cpp
void change(int& x) {
    x = 20;
}

int main() {
    int a = 10;
    change(a);
    std::cout << a << "\n"; // 20
}
```

읽기 전용 참조 전달:

```cpp
#include <iostream>
#include <string>

void printName(const std::string& name) {
    std::cout << name << "\n";
}
```

큰 객체는 `const T&`로 받으면 복사를 피하면서도 수정은 막을 수 있다.

포인터 전달:

```cpp
void change(int* p) {
    if (p != nullptr) {
        *p = 20;
    }
}

int main() {
    int a = 10;
    change(&a);
}
```

실무 감각:

```text
작은 값(int, double 등): 값으로 전달
큰 객체를 읽기만 함: const T&
원본을 수정해야 함: T&
없을 수도 있음/null 가능: T*
소유권 이동: std::unique_ptr<T>
공유 소유: std::shared_ptr<T>
```

---

## 8. 함수

### 기본 함수

```cpp
int add(int a, int b) {
    return a + b;
}
```

Java의 method와 달리 C++ 함수는 클래스 밖에도 존재할 수 있다.

### 선언과 정의

C++은 함수를 사용하기 전에 선언이 필요하다.

```cpp
#include <iostream>

int add(int a, int b); // 선언

int main() {
    std::cout << add(1, 2) << "\n";
}

int add(int a, int b) { // 정의
    return a + b;
}
```

잘못된 코드:

```cpp
int main() {
    std::cout << add(1, 2) << "\n"; // add 선언을 아직 모름
}

int add(int a, int b) {
    return a + b;
}
```

### 함수 오버로딩

Java처럼 같은 이름의 함수를 인자 타입/개수로 구분할 수 있다.

```cpp
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}
```

반환 타입만 다른 오버로딩은 안 된다.

잘못된 코드:

```cpp
int value();
double value(); // 컴파일 에러: 반환 타입만으로는 구분 불가
```

### 기본 인자

Java에는 기본 인자가 없어서 오버로딩으로 해결한다. C++은 기본 인자를 줄 수 있다.

```cpp
void print(int value, int repeat = 1) {
    for (int i = 0; i < repeat; ++i) {
        std::cout << value << "\n";
    }
}

print(10);
print(10, 3);
```

잘못된 코드:

```cpp
void f(int a = 1, int b); // 기본 인자는 오른쪽부터 채워야 함
```

좋은 코드:

```cpp
void f(int a, int b = 1);
```

### 지역 변수의 참조를 반환하면 안 된다

잘못된 코드:

```cpp
int& bad() {
    int x = 10;
    return x; // x는 함수가 끝나면 사라짐
}
```

좋은 코드:

```cpp
int good() {
    int x = 10;
    return x; // 값으로 반환
}
```

---

## 9. 조건문

Java와 거의 같다.

```cpp
if (score >= 90) {
    std::cout << "A\n";
} else if (score >= 80) {
    std::cout << "B\n";
} else {
    std::cout << "C\n";
}
```

C++에서는 0은 false, 0이 아닌 값은 true처럼 쓰일 수 있다.

```cpp
int count = 3;
if (count) {
    std::cout << "not zero\n";
}
```

명확하게 쓰는 게 더 좋다.

```cpp
if (count != 0) {
    std::cout << "not zero\n";
}
```

잘못된 코드:

```cpp
int x = 0;
if (x = 10) { // 비교가 아니라 대입. x가 10이 되고 true
    std::cout << "bug\n";
}
```

좋은 코드:

```cpp
if (x == 10) {
    std::cout << "ok\n";
}
```

---

## 10. `switch`

C++의 `switch`는 정수형, enum 같은 타입에 주로 쓴다. Java처럼 `String` switch는 안 된다.

```cpp
int menu = 2;

switch (menu) {
case 1:
    std::cout << "new\n";
    break;
case 2:
    std::cout << "open\n";
    break;
default:
    std::cout << "unknown\n";
    break;
}
```

잘못된 코드:

```cpp
std::string command = "open";

switch (command) { // 컴파일 에러: std::string switch 불가
case "open":
    break;
}
```

`break`를 빼먹으면 다음 case로 흐른다.

```cpp
switch (menu) {
case 1:
    std::cout << "one\n";
    // break 없음. 의도하지 않았다면 버그
case 2:
    std::cout << "two\n";
    break;
}
```

의도한 fallthrough라면 표시해주는 게 좋다.

```cpp
switch (menu) {
case 1:
    std::cout << "one\n";
    [[fallthrough]];
case 2:
    std::cout << "two\n";
    break;
}
```

---

## 11. 반복문

### `for`

```cpp
for (int i = 0; i < 5; ++i) {
    std::cout << i << "\n";
}
```

`++i`와 `i++`는 primitive에서는 큰 차이가 없지만, iterator에서는 `++i`가 불필요한 복사를 피할 수 있다.

### `while`

```cpp
int i = 0;
while (i < 5) {
    std::cout << i << "\n";
    ++i;
}
```

### range-for

Java의 enhanced for와 비슷하다.

```cpp
#include <vector>

std::vector<int> numbers{1, 2, 3};

for (int n : numbers) {
    std::cout << n << "\n";
}
```

복사를 피하려면 참조를 쓴다.

```cpp
for (const auto& name : names) {
    std::cout << name << "\n";
}
```

수정하려면 non-const 참조를 쓴다.

```cpp
for (auto& n : numbers) {
    n *= 2;
}
```

잘못된 코드:

```cpp
for (auto n : numbers) {
    n *= 2; // 복사본만 바뀜. numbers는 그대로
}
```

---

## 12. 배열

### C-style 배열

```cpp
int arr[3] = {1, 2, 3};
std::cout << arr[0] << "\n";
```

범위 검사를 해주지 않는다.

잘못된 코드:

```cpp
int arr[3] = {1, 2, 3};
std::cout << arr[3] << "\n"; // 범위 밖 접근, undefined behavior
```

### `std::array`

크기가 고정된 배열이 필요하면 `std::array`가 낫다.

```cpp
#include <array>

std::array<int, 3> arr{1, 2, 3};
std::cout << arr.at(0) << "\n";
```

`at()`은 범위 밖이면 예외를 던진다.

```cpp
std::cout << arr.at(3) << "\n"; // std::out_of_range
```

### `std::vector`

Java의 `ArrayList`와 가장 비슷하다.

```cpp
#include <vector>

std::vector<int> numbers;
numbers.push_back(10);
numbers.push_back(20);

std::cout << numbers[0] << "\n";
std::cout << numbers.at(1) << "\n";
```

`operator[]`는 빠르지만 범위 검사를 하지 않는다. `at()`은 검사한다.

잘못된 코드:

```cpp
std::vector<int> numbers{1, 2, 3};
std::cout << numbers[10] << "\n"; // undefined behavior 가능
```

---

## 13. 문자열

Java의 `String`은 불변 객체다. C++의 `std::string`은 값을 수정할 수 있는 객체다.

```cpp
#include <string>

std::string name = "kim";
name += " min";
name[0] = 'K';
```

출력:

```cpp
std::cout << name << "\n";
```

문자열 길이:

```cpp
std::cout << name.size() << "\n";
```

비교:

```cpp
if (name == "Kim min") {
    std::cout << "same\n";
}
```

### C 문자열

```cpp
const char* s = "hello";
```

문자열 리터럴은 수정하면 안 된다.

잘못된 코드:

```cpp
char* s = "hello"; // 최신 C++에서는 위험/불가
s[0] = 'H';        // 문자열 리터럴 수정 시도
```

좋은 코드:

```cpp
std::string s = "hello";
s[0] = 'H';
```

### `std::string_view`

`std::string_view`는 문자열을 소유하지 않는 view다.

```cpp
#include <string_view>

void print(std::string_view s) {
    std::cout << s << "\n";
}
```

잘못된 코드:

```cpp
std::string_view bad() {
    std::string local = "hello";
    return local; // local은 함수 끝나면 사라짐. dangling view
}
```

---

## 14. 구조체와 클래스

C++에는 `struct`와 `class`가 둘 다 있다.

차이는 기본 접근 제어다.

```text
struct: 기본 public
class : 기본 private
```

### struct

```cpp
struct Point {
    int x;
    int y;
};

int main() {
    Point p{1, 2};
    std::cout << p.x << ", " << p.y << "\n";
}
```

### class

```cpp
#include <string>

class Person {
public:
    Person(std::string name, int age)
        : name_(std::move(name)), age_(age) {}

    const std::string& name() const {
        return name_;
    }

    int age() const {
        return age_;
    }

private:
    std::string name_;
    int age_;
};
```

Java와 달리 파일 이름과 클래스 이름이 같을 필요가 없다.

### 잘못된 코드

```cpp
class Person {
    std::string name; // class는 기본 private
};

int main() {
    Person p;
    p.name = "kim"; // 컴파일 에러
}
```

좋은 코드:

```cpp
class Person {
public:
    std::string name;
};
```

또는 getter/setter를 둔다.

---

## 15. 생성자

Java:

```java
class Person {
    private String name;

    Person(String name) {
        this.name = name;
    }
}
```

C++:

```cpp
class Person {
public:
    Person(std::string name)
        : name_(std::move(name)) {}

private:
    std::string name_;
};
```

`:` 뒤의 부분은 member initializer list다. C++에서는 생성자 본문에서 대입하는 것보다 initializer list가 더 중요하다.

좋은 코드:

```cpp
class User {
public:
    User(int id, std::string name)
        : id_(id), name_(std::move(name)) {}

private:
    const int id_;
    std::string name_;
};
```

잘못된 코드:

```cpp
class User {
public:
    User(int id, std::string name) {
        id_ = id; // 컴파일 에러: const 멤버는 이미 초기화가 끝났음
        name_ = name;
    }

private:
    const int id_;
    std::string name_;
};
```

참조 멤버도 initializer list에서 초기화해야 한다.

```cpp
class Holder {
public:
    Holder(int& value)
        : value_(value) {}

private:
    int& value_;
};
```

---

## 16. 소멸자와 RAII

Java에는 `finalize`가 있었지만 믿고 쓰는 방식이 아니고, 현재는 거의 쓰지 않는다. Java는 보통 try-with-resources로 리소스를 닫는다.

C++은 객체가 스코프를 벗어날 때 소멸자가 즉시 호출된다.

```cpp
class File {
public:
    File() {
        std::cout << "open\n";
    }

    ~File() {
        std::cout << "close\n";
    }
};

void work() {
    File file;
} // 여기서 ~File() 호출
```

이 패턴을 RAII라고 한다.

```text
Resource Acquisition Is Initialization
리소스 획득을 객체 초기화와 묶고, 객체 소멸 시 리소스를 해제한다.
```

좋은 코드:

```cpp
#include <fstream>

void writeLog() {
    std::ofstream out("log.txt");
    out << "hello\n";
} // 파일 자동 close
```

잘못된 코드:

```cpp
void writeLog() {
    FILE* f = std::fopen("log.txt", "w");
    std::fprintf(f, "hello\n");
    // fclose를 까먹으면 리소스 누수
}
```

---

## 17. `this`

Java의 `this`와 비슷하다. 다만 C++의 `this`는 포인터다.

```cpp
class Counter {
public:
    void set(int value) {
        this->value_ = value;
    }

private:
    int value_{};
};
```

보통 이름을 다르게 지으면 `this->`를 자주 쓰지 않아도 된다.

```cpp
class Counter {
public:
    void set(int value) {
        value_ = value;
    }

private:
    int value_{};
};
```

---

## 18. 멤버 접근: `.` 과 `->`

객체는 `.`로 접근한다.

```cpp
Person p;
p.sayHello();
```

포인터는 `->`로 접근한다.

```cpp
Person* p = &person;
p->sayHello();
```

`p->sayHello()`는 `(*p).sayHello()`와 같다.

잘못된 코드:

```cpp
Person p;
p->sayHello(); // p는 포인터가 아님
```

```cpp
Person* p = &person;
p.sayHello(); // p는 포인터라서 . 사용 불가
```

---

## 19. `const` 멤버 함수

객체를 수정하지 않는 멤버 함수에는 `const`를 붙인다.

```cpp
class Person {
public:
    const std::string& name() const {
        return name_;
    }

private:
    std::string name_;
};
```

Java에는 이 문법이 없다. C++은 타입 시스템으로 "이 함수는 객체 상태를 바꾸지 않는다"를 표현한다.

잘못된 코드:

```cpp
class Counter {
public:
    int value() const {
        ++value_; // 컴파일 에러: const 함수에서 멤버 수정
        return value_;
    }

private:
    int value_{};
};
```

예외적으로 캐시 같은 내부 구현을 수정해야 하면 `mutable`을 쓸 수 있지만, 남용하면 안 된다.

---

## 20. static

### static 지역 변수

함수가 끝나도 값이 유지된다.

```cpp
int nextId() {
    static int id = 0;
    return ++id;
}
```

### static 멤버

Java의 static field/method와 비슷하다.

```cpp
class Counter {
public:
    static int count;
};

int Counter::count = 0;
```

C++17부터는 `inline static`으로 클래스 안에서 정의할 수 있다.

```cpp
class Counter {
public:
    inline static int count = 0;
};
```

잘못된 코드:

```cpp
class Counter {
public:
    static int count = 0; // C++17 이전 스타일로는 일반 static int 초기화 불가
};
```

---

## 21. 객체 복사

C++은 객체가 기본적으로 값처럼 복사된다.

```cpp
struct Point {
    int x;
    int y;
};

Point a{1, 2};
Point b = a; // 복사
b.x = 10;

std::cout << a.x << "\n"; // 1
```

Java 객체 변수 대입은 참조 복사다.

```java
Point a = new Point(1, 2);
Point b = a; // 같은 객체를 가리킴
b.x = 10;
System.out.println(a.x); // 10
```

C++에서 Java식 공유 참조를 원하면 포인터나 스마트 포인터를 써야 한다.

---

## 22. 복사 생성자, 복사 대입

```cpp
class Person {
public:
    Person(std::string name)
        : name_(std::move(name)) {}

private:
    std::string name_;
};

Person a{"kim"};
Person b = a; // 복사 생성
b = a;        // 복사 대입
```

직접 리소스를 소유하지 않으면 컴파일러가 만든 복사 동작으로 충분하다.

이걸 Rule of Zero라고 생각하면 된다.

```cpp
class Person {
public:
    Person(std::string name)
        : name_(std::move(name)) {}

private:
    std::string name_; // 알아서 복사/해제
};
```

### 잘못된 코드: raw pointer 소유

```cpp
class Buffer {
public:
    Buffer(std::size_t size)
        : size_(size), data_(new int[size]) {}

    ~Buffer() {
        delete[] data_;
    }

private:
    std::size_t size_;
    int* data_;
};

int main() {
    Buffer a(10);
    Buffer b = a; // 얕은 복사. data_ 주소가 둘 다 같아짐
} // 둘 다 같은 포인터를 delete[] 하려고 해서 double free
```

좋은 코드:

```cpp
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(size) {}

private:
    std::vector<int> data_;
};
```

---

## 23. 이동 semantics

C++에는 "복사" 말고 "이동"이 있다. 큰 리소스를 새로 복사하지 않고 소유권을 넘기는 개념이다.

```cpp
#include <string>
#include <utility>

std::string a = "hello";
std::string b = std::move(a);
```

`std::move`는 실제로 옮기는 함수라기보다 "이 객체는 이동해도 된다"고 캐스팅하는 표현이다.

이동 후 객체는 유효하지만 값은 기대하지 않는 게 좋다.

```cpp
std::string a = "hello";
std::string b = std::move(a);

// a는 여전히 파괴 가능하고 대입 가능하지만,
// 내용이 "hello"라고 기대하면 안 된다.
a = "new value";
```

잘못된 코드:

```cpp
std::string a = "hello";
std::string b = std::move(a);
std::cout << a << "\n"; // 문법상 가능하지만 내용에 기대면 안 됨
```

---

## 24. Rule of Zero, Three, Five

리소스를 직접 소유하지 않으면 아무것도 직접 만들지 않는다.

```cpp
class User {
public:
    User(std::string name)
        : name_(std::move(name)) {}

private:
    std::string name_;
};
```

이게 Rule of Zero다.

직접 포인터를 소유해서 소멸자를 만들었다면 보통 복사 생성자, 복사 대입도 필요하다. C++11 이후에는 이동 생성자, 이동 대입까지 고려해야 한다. 그래서 Rule of Five라고 부른다.

가능하면 직접 구현하지 말고 `std::vector`, `std::string`, `std::unique_ptr` 같은 타입에게 맡기는 게 좋다.

---

## 25. 동적 할당과 `new/delete`

Java:

```java
Person p = new Person();
```

C++:

```cpp
Person* p = new Person();
delete p;
```

하지만 현대 C++에서는 직접 `new/delete`를 거의 쓰지 않는 게 좋다.

잘못된 코드:

```cpp
void leak() {
    int* p = new int(10);
    // delete p; 없음. memory leak
}
```

잘못된 코드:

```cpp
int x = 10;
delete &x; // stack 객체를 delete하면 안 됨
```

잘못된 코드:

```cpp
int* p = new int(10);
delete p;
delete p; // double delete
```

잘못된 코드:

```cpp
int* arr = new int[10];
delete arr; // new[]에는 delete[]가 필요
```

좋은 코드:

```cpp
#include <memory>

auto p = std::make_unique<Person>();
```

---

## 26. 스마트 포인터

### `std::unique_ptr`

소유자가 하나뿐인 포인터다.

```cpp
#include <memory>

std::unique_ptr<Person> p = std::make_unique<Person>();
p->sayHello();
```

복사는 안 되고 이동만 된다.

```cpp
auto a = std::make_unique<Person>();
auto b = std::move(a); // 소유권 이동
```

잘못된 코드:

```cpp
auto a = std::make_unique<Person>();
auto b = a; // 컴파일 에러: unique_ptr은 복사 불가
```

### `std::shared_ptr`

여러 곳이 공동 소유할 때 쓴다.

```cpp
#include <memory>

auto a = std::make_shared<Person>();
auto b = a; // reference count 증가
```

참조 카운트가 0이 되면 객체가 삭제된다.

### `std::weak_ptr`

`shared_ptr`의 순환 참조를 끊을 때 쓴다.

잘못된 코드:

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->prev = a; // 서로 잡고 있어서 해제되지 않을 수 있음
```

좋은 코드:

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;
};
```

실무 감각:

```text
대부분은 값으로 둔다.
동적 할당과 단독 소유가 필요하면 unique_ptr.
정말 공동 소유가 필요할 때만 shared_ptr.
소유하지 않고 관찰만 하면 raw pointer, reference, weak_ptr 중 의미에 맞게 선택.
```

---

## 27. enum과 enum class

C 스타일 enum:

```cpp
enum Color {
    Red,
    Green,
    Blue
};
```

이름이 바깥 scope로 새어 나가고 int로 쉽게 변환된다.

더 좋은 방식은 `enum class`다.

```cpp
enum class Color {
    Red,
    Green,
    Blue
};

Color color = Color::Red;
```

잘못된 코드:

```cpp
enum class Color {
    Red,
    Green
};

int x = Color::Red; // 컴파일 에러: 암시적 int 변환 안 됨
```

필요하면 명시적으로 변환한다.

```cpp
int x = static_cast<int>(Color::Red);
```

---

## 28. namespace

Java의 package와 비슷하게 이름 충돌을 줄인다.

```cpp
namespace math {
    int add(int a, int b) {
        return a + b;
    }
}

int main() {
    std::cout << math::add(1, 2) << "\n";
}
```

`using namespace std;`는 작은 예제에서는 편하지만 header에서는 피해야 한다.

잘못된 코드:

```cpp
// my_header.h
#include <vector>
using namespace std; // 헤더를 include한 모든 파일에 영향을 줌
```

좋은 코드:

```cpp
// my_header.h
#include <vector>

std::vector<int> makeNumbers();
```

함수 안이나 cpp 파일의 좁은 범위에서만 using을 쓰는 건 괜찮다.

```cpp
void print() {
    using std::cout;
    cout << "hello\n";
}
```

---

## 29. 헤더 파일과 cpp 파일

Java는 클래스 하나가 파일 하나에 들어가는 경우가 많다. C++은 선언과 구현을 나눌 수 있다.

`calculator.h`

```cpp
#pragma once

int add(int a, int b);
```

`calculator.cpp`

```cpp
#include "calculator.h"

int add(int a, int b) {
    return a + b;
}
```

`main.cpp`

```cpp
#include <iostream>
#include "calculator.h"

int main() {
    std::cout << add(1, 2) << "\n";
}
```

### include guard

`#pragma once` 대신 표준적인 include guard도 가능하다.

```cpp
#ifndef CALCULATOR_H
#define CALCULATOR_H

int add(int a, int b);

#endif
```

### 잘못된 코드: 헤더에 일반 함수 정의

```cpp
// util.h
int add(int a, int b) {
    return a + b;
}
```

이 헤더가 여러 cpp에서 include되면 link 단계에서 multiple definition 에러가 날 수 있다.

헤더에 함수 정의를 두려면 `inline`을 붙이거나 template처럼 헤더에 있어야 하는 경우여야 한다.

```cpp
inline int add(int a, int b) {
    return a + b;
}
```

### 잘못된 코드: 헤더에 전역 변수 정의

```cpp
// config.h
int globalCount = 0; // 여러 cpp에서 include하면 multiple definition
```

좋은 코드:

```cpp
// config.h
extern int globalCount;
```

```cpp
// config.cpp
int globalCount = 0;
```

C++17부터는 헤더에 `inline` 변수를 둘 수 있다.

```cpp
inline int globalCount = 0;
```

---

## 30. 전처리기와 매크로

`#include`, `#define`, `#if` 같은 것은 컴파일 전에 처리된다.

```cpp
#define PI 3.14
```

매크로는 타입 검사도 없고 단순 치환이라 위험하다.

잘못된 코드:

```cpp
#define SQUARE(x) x * x

int result = SQUARE(1 + 2); // 1 + 2 * 1 + 2 = 5
```

좋은 코드:

```cpp
constexpr int square(int x) {
    return x * x;
}
```

상수도 매크로보다 `constexpr`가 좋다.

```cpp
constexpr double Pi = 3.141592;
```

---

## 31. 형변환

Java:

```java
double d = 3.14;
int i = (int) d;
```

C++에도 C 스타일 캐스트가 있지만, 의도를 드러내는 캐스트를 쓰는 게 좋다.

```cpp
double d = 3.14;
int i = static_cast<int>(d);
```

### 캐스트 종류

```text
static_cast     일반적인 명시 변환
dynamic_cast    polymorphic 타입의 안전한 downcast
const_cast      const 제거
reinterpret_cast 비트 수준 재해석, 매우 위험
```

잘못된 코드:

```cpp
double d = 3.14;
int i = (int)d; // 가능하지만 의도가 덜 명확함
```

상속 관계에서 downcast:

```cpp
class Base {
public:
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    void hello() {}
};

Base* b = new Derived();

if (auto* d = dynamic_cast<Derived*>(b)) {
    d->hello();
}

delete b;
```

---

## 32. 연산자

대부분 Java와 비슷하다.

```cpp
int a = 10 + 3;
int b = 10 - 3;
int c = 10 * 3;
int d = 10 / 3; // 3
int e = 10 % 3; // 1
```

정수 나눗셈은 소수점이 버려진다.

```cpp
double x = 10 / 3;   // 3.0
double y = 10.0 / 3; // 3.333...
```

잘못된 코드:

```cpp
int x = 10;
int y = 0;
std::cout << x / y << "\n"; // undefined behavior
```

논리 연산:

```cpp
if (a > 0 && b > 0) {}
if (a > 0 || b > 0) {}
if (!(a > 0)) {}
```

비트 연산:

```cpp
int flags = 0;
flags |= 1 << 0;
flags |= 1 << 1;

if (flags & (1 << 0)) {
    std::cout << "bit 0 on\n";
}
```

---

## 33. 연산자 오버로딩

C++은 클래스에 연산자 의미를 정의할 수 있다. Java에는 일반 연산자 오버로딩이 없다. `String +` 정도가 특별 케이스다.

```cpp
struct Vec2 {
    int x;
    int y;
};

Vec2 operator+(const Vec2& a, const Vec2& b) {
    return Vec2{a.x + b.x, a.y + b.y};
}
```

사용:

```cpp
Vec2 a{1, 2};
Vec2 b{3, 4};
Vec2 c = a + b;
```

잘못된 설계:

```cpp
Vec2 operator+(const Vec2& a, const Vec2& b) {
    return Vec2{a.x - b.x, a.y - b.y}; // +인데 빼기를 함. 문법은 되지만 나쁜 설계
}
```

연산자 오버로딩은 직관적인 의미일 때만 쓰는 게 좋다.

---

## 34. 상속

Java:

```java
class Dog extends Animal {
}
```

C++:

```cpp
class Dog : public Animal {
};
```

`public`을 빼먹으면 `class` 상속은 기본 private이 된다.

잘못된 코드:

```cpp
class Dog : Animal { // private 상속
};
```

좋은 코드:

```cpp
class Dog : public Animal {
};
```

`struct`는 기본 public 상속이다.

---

## 35. virtual과 다형성

Java의 instance method는 기본적으로 동적 dispatch가 된다. C++은 기본적으로 정적 dispatch다. 다형성을 원하면 `virtual`이 필요하다.

잘못된 코드:

```cpp
#include <iostream>

class Animal {
public:
    void speak() const {
        std::cout << "animal\n";
    }
};

class Dog : public Animal {
public:
    void speak() const {
        std::cout << "dog\n";
    }
};

void print(const Animal& animal) {
    animal.speak();
}

int main() {
    Dog dog;
    print(dog); // animal 출력
}
```

좋은 코드:

```cpp
#include <iostream>

class Animal {
public:
    virtual ~Animal() = default;

    virtual void speak() const {
        std::cout << "animal\n";
    }
};

class Dog : public Animal {
public:
    void speak() const override {
        std::cout << "dog\n";
    }
};

void print(const Animal& animal) {
    animal.speak();
}
```

`override`는 꼭 붙이는 게 좋다. 오타를 컴파일러가 잡아준다.

잘못된 코드:

```cpp
class Animal {
public:
    virtual void speak() const {}
};

class Dog : public Animal {
public:
    void speak() {} // const가 빠져서 override가 아님
};
```

좋은 코드:

```cpp
class Dog : public Animal {
public:
    void speak() const override {}
};
```

---

## 36. 순수 가상 함수와 인터페이스

Java interface:

```java
interface Drawable {
    void draw();
}
```

C++:

```cpp
class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw() = 0;
};
```

`= 0`이면 순수 가상 함수다. 이 함수가 있으면 추상 클래스가 된다.

```cpp
class Circle : public Drawable {
public:
    void draw() override {
        std::cout << "circle\n";
    }
};
```

잘못된 코드:

```cpp
class Circle : public Drawable {
}; // draw를 구현하지 않아서 Circle도 추상 클래스

Circle c; // 컴파일 에러
```

---

## 37. 가상 소멸자

상속에서 base pointer로 derived 객체를 삭제할 수 있으면 base 소멸자는 virtual이어야 한다.

잘못된 코드:

```cpp
class Base {
public:
    ~Base() {}
};

class Derived : public Base {
public:
    ~Derived() {
        std::cout << "derived cleanup\n";
    }
};

Base* p = new Derived();
delete p; // Derived 소멸자가 호출되지 않을 수 있음
```

좋은 코드:

```cpp
class Base {
public:
    virtual ~Base() = default;
};
```

---

## 38. 객체 slicing

C++에서 derived 객체를 base 값으로 복사하면 derived 부분이 잘린다.

잘못된 코드:

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const {
        std::cout << "animal\n";
    }
};

class Dog : public Animal {
public:
    void speak() const override {
        std::cout << "dog\n";
    }
};

void print(Animal animal) { // 값으로 받음
    animal.speak();
}

Dog dog;
print(dog); // slicing 발생, animal 출력
```

좋은 코드:

```cpp
void print(const Animal& animal) {
    animal.speak();
}
```

---

## 39. 템플릿

Java generic:

```java
class Box<T> {
    T value;
}
```

C++ template:

```cpp
template <typename T>
class Box {
public:
    explicit Box(T value)
        : value_(std::move(value)) {}

    const T& value() const {
        return value_;
    }

private:
    T value_;
};
```

사용:

```cpp
Box<int> intBox{10};
Box<std::string> stringBox{"hello"};
```

C++ 템플릿은 Java generic의 type erasure와 다르게 컴파일 타임에 타입별 코드가 생성되는 느낌에 가깝다.

### 함수 템플릿

```cpp
template <typename T>
T maxValue(T a, T b) {
    return a > b ? a : b;
}

std::cout << maxValue(1, 2) << "\n";
std::cout << maxValue(1.5, 2.5) << "\n";
```

잘못된 코드:

```cpp
std::cout << maxValue(1, 2.5) << "\n"; // T를 int로 볼지 double로 볼지 애매
```

해결:

```cpp
std::cout << maxValue<double>(1, 2.5) << "\n";
```

또는 서로 다른 타입을 받게 작성한다.

```cpp
template <typename A, typename B>
auto maxValue(A a, B b) {
    return a > b ? a : b;
}
```

---

## 40. C++20 concept 맛보기

템플릿 에러는 길고 어렵다. C++20 concept은 template 인자 조건을 표현한다.

```cpp
#include <concepts>

template <std::integral T>
T add(T a, T b) {
    return a + b;
}
```

`std::integral`은 정수 타입만 허용한다.

잘못된 코드:

```cpp
add(1.2, 3.4); // double은 integral이 아님
```

현재 프로젝트가 C++17 기준이면 concept은 바로 쓰지 못할 수 있다.

---

## 41. 람다

Java lambda:

```java
list.forEach(x -> System.out.println(x));
```

C++ lambda:

```cpp
auto print = [](int x) {
    std::cout << x << "\n";
};

print(10);
```

캡처:

```cpp
int base = 10;

auto addBase = [base](int x) {
    return base + x;
};
```

참조 캡처:

```cpp
int sum = 0;
std::vector<int> numbers{1, 2, 3};

for (int n : numbers) {
    auto add = [&sum, n] {
        sum += n;
    };
    add();
}
```

잘못된 코드: dangling reference capture

```cpp
#include <functional>

std::function<int()> bad() {
    int x = 10;
    return [&x] {
        return x; // x는 함수 끝나면 사라짐
    };
}
```

좋은 코드:

```cpp
std::function<int()> good() {
    int x = 10;
    return [x] {
        return x;
    };
}
```

---

## 42. STL 컨테이너

STL은 C++ 표준 라이브러리의 자료구조와 알고리즘 묶음이다.

### `std::vector`

Java `ArrayList` 느낌.

```cpp
std::vector<int> v{1, 2, 3};
v.push_back(4);
```

### `std::list`

연결 리스트. 임의 접근이 느리다.

```cpp
#include <list>

std::list<int> values{1, 2, 3};
```

대부분은 `std::vector`부터 고려하는 게 좋다.

### `std::map`

Java `TreeMap` 느낌. key 정렬 유지.

```cpp
#include <map>

std::map<std::string, int> scores;
scores["kim"] = 90;
```

### `std::unordered_map`

Java `HashMap` 느낌.

```cpp
#include <unordered_map>

std::unordered_map<std::string, int> scores;
scores["kim"] = 90;
```

### `std::set`, `std::unordered_set`

```cpp
#include <set>
#include <unordered_set>

std::set<int> sorted{3, 1, 2};
std::unordered_set<int> hashed{3, 1, 2};
```

### `std::queue`, `std::stack`

```cpp
#include <queue>
#include <stack>

std::queue<int> q;
q.push(1);
q.push(2);
q.pop();

std::stack<int> st;
st.push(1);
st.pop();
```

잘못된 코드:

```cpp
std::queue<int> q;
std::cout << q.front() << "\n"; // 비어있는데 front 호출
```

좋은 코드:

```cpp
if (!q.empty()) {
    std::cout << q.front() << "\n";
}
```

---

## 43. iterator

Iterator는 컨테이너 안의 위치를 가리키는 객체다. Java의 `Iterator`와 비슷하지만 포인터처럼 `*`, `++`를 쓴다.

```cpp
std::vector<int> v{1, 2, 3};

for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << "\n";
}
```

보통은 range-for가 더 읽기 쉽다.

```cpp
for (int n : v) {
    std::cout << n << "\n";
}
```

### iterator invalidation

컨테이너가 재할당되면 기존 iterator/reference/pointer가 무효화될 수 있다.

잘못된 코드:

```cpp
std::vector<int> v{1, 2, 3};
auto it = v.begin();

v.push_back(4); // 재할당이 일어나면 it 무효화 가능

std::cout << *it << "\n"; // 위험
```

좋은 코드:

```cpp
std::vector<int> v{1, 2, 3};
v.reserve(10);

auto it = v.begin();
v.push_back(4); // reserve 범위 안이면 재할당 없음
std::cout << *it << "\n";
```

더 좋은 경우는 iterator를 오래 들고 있지 않는 것이다.

---

## 44. 알고리즘 라이브러리

C++은 컨테이너 멤버 함수만 쓰는 게 아니라 `<algorithm>`의 함수를 많이 쓴다.

```cpp
#include <algorithm>
#include <vector>

std::vector<int> v{3, 1, 2};
std::sort(v.begin(), v.end());
```

찾기:

```cpp
auto it = std::find(v.begin(), v.end(), 2);
if (it != v.end()) {
    std::cout << "found\n";
}
```

조건 개수:

```cpp
int count = std::count_if(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
});
```

변환:

```cpp
std::vector<int> out;
out.reserve(v.size());

std::transform(v.begin(), v.end(), std::back_inserter(out), [](int x) {
    return x * 2;
});
```

잘못된 코드:

```cpp
std::vector<int> out;
std::transform(v.begin(), v.end(), out.begin(), [](int x) {
    return x * 2;
}); // out에 공간이 없음
```

좋은 코드:

```cpp
std::vector<int> out(v.size());
std::transform(v.begin(), v.end(), out.begin(), [](int x) {
    return x * 2;
});
```

또는:

```cpp
std::vector<int> out;
std::transform(v.begin(), v.end(), std::back_inserter(out), [](int x) {
    return x * 2;
});
```

---

## 45. 예외 처리

Java에는 checked exception이 있다. C++에는 checked exception이 없다.

```cpp
#include <stdexcept>

int divide(int a, int b) {
    if (b == 0) {
        throw std::invalid_argument("division by zero");
    }
    return a / b;
}
```

잡기:

```cpp
try {
    std::cout << divide(10, 0) << "\n";
} catch (const std::invalid_argument& e) {
    std::cout << e.what() << "\n";
} catch (const std::exception& e) {
    std::cout << "error: " << e.what() << "\n";
}
```

예외는 보통 `const T&`로 잡는다.

잘못된 코드:

```cpp
try {
    throw std::runtime_error("fail");
} catch (std::exception e) { // 값으로 잡으면 slicing 가능
    std::cout << e.what() << "\n";
}
```

좋은 코드:

```cpp
catch (const std::exception& e) {
    std::cout << e.what() << "\n";
}
```

### `noexcept`

예외를 던지지 않는 함수임을 표현한다.

```cpp
int add(int a, int b) noexcept {
    return a + b;
}
```

`noexcept` 함수에서 예외가 밖으로 나가면 프로그램이 종료된다.

---

## 46. `std::optional`

Java의 `Optional<T>`와 비슷하다. 값이 있을 수도, 없을 수도 있음을 표현한다.

```cpp
#include <optional>

std::optional<int> findScore(const std::string& name) {
    if (name == "kim") {
        return 90;
    }
    return std::nullopt;
}
```

사용:

```cpp
auto score = findScore("kim");
if (score.has_value()) {
    std::cout << *score << "\n";
}
```

더 간단히:

```cpp
if (score) {
    std::cout << score.value() << "\n";
}
```

잘못된 코드:

```cpp
std::optional<int> score = std::nullopt;
std::cout << score.value() << "\n"; // 예외 발생
```

좋은 코드:

```cpp
std::cout << score.value_or(0) << "\n";
```

---

## 47. `std::variant`, `std::tuple`, structured binding

### `std::variant`

여러 타입 중 하나를 담는다.

```cpp
#include <variant>

std::variant<int, std::string> value;
value = 10;
value = std::string{"hello"};
```

사용:

```cpp
if (auto* s = std::get_if<std::string>(&value)) {
    std::cout << *s << "\n";
}
```

잘못된 코드:

```cpp
std::variant<int, std::string> value = 10;
std::cout << std::get<std::string>(value) << "\n"; // bad_variant_access 예외
```

### `std::tuple`

여러 값을 묶는다.

```cpp
#include <tuple>

std::tuple<int, std::string> user{1, "kim"};
```

structured binding:

```cpp
auto [id, name] = user;
```

pair에도 자주 쓴다.

```cpp
std::map<std::string, int> scores{{"kim", 90}};

for (const auto& [name, score] : scores) {
    std::cout << name << ": " << score << "\n";
}
```

---

## 48. 입력과 출력

### 콘솔 출력

```cpp
#include <iostream>

std::cout << "hello " << 10 << "\n";
```

`std::endl`은 줄바꿈 + flush다. 단순 줄바꿈이면 `"\n"`이 보통 더 낫다.

```cpp
std::cout << "hello" << std::endl; // flush까지 함
std::cout << "hello\n";            // 줄바꿈만
```

### 콘솔 입력

```cpp
int age;
std::cin >> age;
```

문자열 한 줄 입력:

```cpp
std::string line;
std::getline(std::cin, line);
```

`>>` 다음에 `getline`을 쓰면 남아있는 newline 때문에 문제가 생길 수 있다.

잘못된 코드:

```cpp
int age;
std::cin >> age;

std::string name;
std::getline(std::cin, name); // 빈 문자열이 들어갈 수 있음
```

좋은 코드:

```cpp
#include <limits>

int age;
std::cin >> age;
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

std::string name;
std::getline(std::cin, name);
```

---

## 49. 파일 입출력

```cpp
#include <fstream>
#include <string>

void writeFile() {
    std::ofstream out("hello.txt");
    out << "hello\n";
}
```

읽기:

```cpp
void readFile() {
    std::ifstream in("hello.txt");
    std::string line;

    while (std::getline(in, line)) {
        std::cout << line << "\n";
    }
}
```

잘못된 코드:

```cpp
std::ifstream in("hello.txt");
while (!in.eof()) {
    std::string line;
    std::getline(in, line);
    std::cout << line << "\n";
}
```

`eof()`는 읽기 시도 후에 true가 된다. 읽기 성공 여부를 조건으로 써야 한다.

좋은 코드:

```cpp
std::string line;
while (std::getline(in, line)) {
    std::cout << line << "\n";
}
```

---

## 50. friend

`friend`는 특정 함수나 클래스를 private 멤버에 접근 가능하게 만든다.

```cpp
class Vec2 {
public:
    Vec2(int x, int y)
        : x_(x), y_(y) {}

    friend std::ostream& operator<<(std::ostream& os, const Vec2& v);

private:
    int x_;
    int y_;
};

std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    return os << "(" << v.x_ << ", " << v.y_ << ")";
}
```

남용하면 캡슐화가 약해진다. 연산자 출력 같은 필요한 경우에 제한적으로 쓰는 게 좋다.

---

## 51. 접근 제어

```cpp
class Person {
public:
    void sayHello();

protected:
    int age_;

private:
    std::string name_;
};
```

Java와 비교:

```text
public    : 누구나 접근
protected : 상속받은 클래스와 같은 클래스 계층에서 접근
private   : 클래스 내부만 접근
```

C++에는 Java의 package-private 기본 접근자가 없다.

---

## 52. `explicit`

생성자 하나짜리 클래스는 암시적 변환에 쓰일 수 있다.

잘못된 코드:

```cpp
class UserId {
public:
    UserId(int value)
        : value_(value) {}

private:
    int value_;
};

void findUser(UserId id) {}

findUser(10); // int가 UserId로 암시적 변환됨
```

좋은 코드:

```cpp
class UserId {
public:
    explicit UserId(int value)
        : value_(value) {}

private:
    int value_;
};

findUser(UserId{10});
```

단일 인자 생성자는 대부분 `explicit`을 붙이는 습관이 좋다.

---

## 53. `nullptr`

C에서는 `NULL`을 많이 썼지만, C++에서는 `nullptr`을 쓴다.

```cpp
int* p = nullptr;
```

잘못된 코드:

```cpp
void f(int);
void f(int*);

f(NULL); // 어떤 overload인지 애매하거나 int로 갈 수 있음
```

좋은 코드:

```cpp
f(nullptr);
```

---

## 54. `typedef`와 `using`

타입 별칭을 만든다.

옛 방식:

```cpp
typedef std::vector<int> IntList;
```

현대 방식:

```cpp
using IntList = std::vector<int>;
```

템플릿 alias는 `using`이 훨씬 읽기 좋다.

```cpp
template <typename T>
using Vec = std::vector<T>;

Vec<int> numbers{1, 2, 3};
```

---

## 55. `using` 선언과 `using namespace`

```cpp
using std::cout;

cout << "hello\n";
```

이건 특정 이름 하나만 가져온다.

```cpp
using namespace std;
```

이건 namespace 안의 이름을 많이 가져온다. 예제 코드에서는 편하지만 큰 코드나 header에서는 피한다.

---

## 56. `decltype`

표현식의 타입을 얻는다.

```cpp
int x = 10;
decltype(x) y = 20; // int
```

반환 타입 추론이 필요할 때 가끔 쓴다.

```cpp
template <typename A, typename B>
auto add(A a, B b) -> decltype(a + b) {
    return a + b;
}
```

C++14 이후에는 단순한 경우 `auto` 반환 타입을 더 많이 쓴다.

```cpp
template <typename A, typename B>
auto add(A a, B b) {
    return a + b;
}
```

---

## 57. lvalue, rvalue 감각

정확히 들어가면 깊지만, 일단 이렇게 잡으면 된다.

```text
lvalue: 이름이 있고 주소를 잡을 수 있는 값
rvalue: 임시 값, 곧 사라지는 값
```

```cpp
int x = 10; // x는 lvalue
int y = x + 1; // x + 1은 rvalue
```

참조:

```cpp
int& r = x;      // lvalue reference
const int& cr = 10; // const reference는 임시 값도 받을 수 있음
```

잘못된 코드:

```cpp
int& r = 10; // non-const lvalue reference는 rvalue에 바인딩 불가
```

rvalue reference:

```cpp
int&& rr = 10;
```

이동 semantics와 완벽 전달에서 중요해진다.

---

## 58. perfect forwarding 맛보기

라이브러리 코드에서 자주 보인다.

```cpp
#include <utility>

template <typename T>
void wrapper(T&& value) {
    target(std::forward<T>(value));
}
```

일반 앱 코드에서는 처음부터 깊게 외우기보다, `std::move`와 `std::forward`가 다른 목적이라는 정도를 기억하면 된다.

```text
std::move    : 이동 가능하게 캐스팅
std::forward : 받은 값의 lvalue/rvalue 성격을 유지해서 전달
```

잘못된 코드:

```cpp
template <typename T>
void wrapper(T&& value) {
    target(std::move(value)); // lvalue로 들어온 것도 무조건 이동해버림
}
```

---

## 59. 함수 포인터와 `std::function`

함수도 값처럼 전달할 수 있다.

```cpp
int add(int a, int b) {
    return a + b;
}

int (*fp)(int, int) = add;
std::cout << fp(1, 2) << "\n";
```

더 편하게는 `std::function`을 쓴다.

```cpp
#include <functional>

std::function<int(int, int)> op = [](int a, int b) {
    return a + b;
};
```

성능이 중요한 곳에서는 template callable을 받을 수도 있다.

```cpp
template <typename Func>
void repeat(int count, Func func) {
    for (int i = 0; i < count; ++i) {
        func(i);
    }
}
```

---

## 60. 멀티스레드 기본

Java의 `Thread`, `synchronized`, `ExecutorService`처럼 C++에도 thread와 mutex가 있다.

```cpp
#include <thread>
#include <iostream>

void work() {
    std::cout << "work\n";
}

int main() {
    std::thread t(work);
    t.join();
}
```

`join()`을 호출하지 않고 `std::thread` 객체가 소멸되면 프로그램이 종료된다.

잘못된 코드:

```cpp
int main() {
    std::thread t(work);
} // join/detach 없이 소멸, std::terminate
```

공유 데이터는 mutex로 보호한다.

```cpp
#include <mutex>

int counter = 0;
std::mutex m;

void increment() {
    std::lock_guard<std::mutex> lock(m);
    ++counter;
}
```

잘못된 코드:

```cpp
int counter = 0;

void increment() {
    ++counter; // 여러 스레드가 동시에 실행하면 data race
}
```

단순 counter는 atomic을 쓸 수 있다.

```cpp
#include <atomic>

std::atomic<int> counter{0};

void increment() {
    ++counter;
}
```

---

## 61. undefined behavior

C++에서 제일 무서운 말이다. Java라면 예외가 나거나 VM이 보호해주는 상황도 C++에서는 "무슨 일이든 일어날 수 있음"이 된다.

대표 예시:

```cpp
int arr[3] = {1, 2, 3};
std::cout << arr[10] << "\n"; // 범위 밖 접근
```

```cpp
int* p = nullptr;
*p = 10; // nullptr 역참조
```

```cpp
int x;
std::cout << x << "\n"; // 초기화되지 않은 값 읽기
```

```cpp
int* p = new int(10);
delete p;
std::cout << *p << "\n"; // use after free
```

Java의 NullPointerException처럼 항상 잡히는 게 아니다. 이상하게 잘 되는 것처럼 보이다가 나중에 터질 수도 있다.

---

## 62. C++에서 자주 쓰는 안전한 기본값

```cpp
// 1. 직접 new/delete 대신 값 또는 스마트 포인터
std::vector<int> values;
auto user = std::make_unique<User>();

// 2. 큰 객체 읽기 전용 인자는 const reference
void print(const std::string& s);

// 3. 상속 다형성에는 virtual destructor
class Base {
public:
    virtual ~Base() = default;
};

// 4. override 붙이기
void speak() const override;

// 5. 단일 인자 생성자는 explicit
explicit UserId(int value);

// 6. enum class 사용
enum class State { Ready, Running, Done };

// 7. 초기화는 바로 하기
int count{};
std::string name;

// 8. 범위 기반 for에서 큰 객체는 const auto&
for (const auto& item : items) {}
```

---

## 63. Java에서 C++로 옮길 때 자주 하는 실수

### 1. 모든 객체를 `new`로 만들기

Java 습관:

```cpp
Person* p = new Person();
delete p;
```

C++ 기본값:

```cpp
Person p;
```

동적 할당이 필요할 때:

```cpp
auto p = std::make_unique<Person>();
```

### 2. 복사를 참조 공유로 착각

```cpp
Person a{"kim"};
Person b = a; // 복사
```

Java처럼 같은 객체를 가리키는 게 아니다.

### 3. base class method가 자동 virtual이라고 생각

```cpp
class Base {
public:
    void f();
};
```

이건 virtual이 아니다.

```cpp
class Base {
public:
    virtual void f();
};
```

### 4. 소멸 시점을 GC처럼 생각

```cpp
void f() {
    Person p;
} // 여기서 바로 소멸
```

### 5. header에 아무거나 정의

헤더는 여러 cpp에 들어갈 수 있다. 일반 함수/전역 변수 정의를 조심해야 한다.

---

## 64. C++ 문법 빠른 비교표

| 개념 | Java | C++ |
|---|---|---|
| 시작점 | `public static void main` | `int main()` |
| import | `import` | `#include` |
| 패키지/이름공간 | `package` | `namespace` |
| 문자열 | `String` 불변 | `std::string` 가변 |
| 동적 할당 | `new`, GC | `new/delete`, 보통 스마트 포인터 |
| 객체 변수 | 참조 | 값 또는 포인터/참조 |
| 상속 | `extends` | `: public Base` |
| 인터페이스 | `interface` | 순수 가상 함수 클래스 |
| 동적 dispatch | 기본 | `virtual` 필요 |
| override 검사 | `@Override` | `override` |
| generic | type erasure | template, 컴파일 타임 |
| null | `null` | `nullptr` |
| final 변수 | `final` | `const` |
| compile-time 상수 | `static final` | `constexpr` |
| 예외 | checked/unchecked | checked exception 없음 |
| 리소스 관리 | try-with-resources | RAII |
| 배열 범위 검사 | 대부분 예외 | raw array/vector `[]`는 검사 없음 |
| optional | `Optional<T>` | `std::optional<T>` |

---

## 65. 한 파일 예제

여러 문법을 한 번에 섞은 예제다.

```cpp
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class User {
public:
    User(int id, std::string name)
        : id_(id), name_(std::move(name)) {}

    int id() const {
        return id_;
    }

    const std::string& name() const {
        return name_;
    }

    void rename(std::string name) {
        name_ = std::move(name);
    }

private:
    int id_;
    std::string name_;
};

class UserRepository {
public:
    void add(User user) {
        users_.push_back(std::move(user));
    }

    const User* findById(int id) const {
        auto it = std::find_if(users_.begin(), users_.end(), [id](const User& user) {
            return user.id() == id;
        });

        if (it == users_.end()) {
            return nullptr;
        }

        return &*it;
    }

private:
    std::vector<User> users_;
};

int main() {
    UserRepository repo;
    repo.add(User{1, "kim"});
    repo.add(User{2, "lee"});

    const User* user = repo.findById(1);
    if (user != nullptr) {
        std::cout << user->name() << "\n";
    }
}
```

포인트:

- `User`는 값을 안전하게 소유한다.
- `std::string`은 직접 메모리 관리를 하지 않아도 된다.
- `add(User user)`로 받은 뒤 vector에 move한다.
- `findById`는 없을 수도 있으므로 pointer를 반환한다.
- 반환한 pointer는 `repo`가 살아 있고 `users_`가 재할당되지 않는 동안만 유효하다.

`findById`를 더 안전하게 하려면 id만 반환하거나, 복사본을 반환하거나, 컨테이너 안정성을 보장하는 구조를 선택해야 한다.

---

## 66. 같은 예제의 잘못된 버전

```cpp
#include <iostream>
#include <string>
#include <vector>

class User {
public:
    User(int id, const std::string& name) {
        id_ = id;
        name_ = new std::string(name);
    }

    ~User() {
        delete name_;
    }

    std::string* name() {
        return name_;
    }

private:
    int id_;
    std::string* name_;
};

int main() {
    std::vector<User> users;
    User user(1, "kim");
    users.push_back(user); // 얕은 복사 발생 가능

    std::cout << *users[0].name() << "\n";
} // double delete 가능
```

문제:

- `std::string*`를 직접 소유한다.
- 복사 생성자/복사 대입을 직접 정의하지 않았다.
- `push_back(user)`에서 복사가 일어나면 같은 포인터를 두 객체가 소유한다.
- 소멸자에서 같은 포인터를 두 번 delete할 수 있다.

좋은 해결:

```cpp
class User {
public:
    User(int id, std::string name)
        : id_(id), name_(std::move(name)) {}

private:
    int id_;
    std::string name_;
};
```

---

## 67. 보충: 선언, 정의, ODR

C++은 선언 declaration과 정의 definition을 구분한다.

```cpp
int add(int a, int b); // 선언

int add(int a, int b) { // 정의
    return a + b;
}
```

변수도 마찬가지다.

```cpp
extern int globalCount; // 선언

int globalCount = 0;    // 정의
```

Java는 클래스 단위로 컴파일하고 JVM이 링크를 처리하는 느낌이 강하지만, C++은 여러 translation unit을 컴파일한 뒤 linker가 합친다.

```text
main.cpp       -> main.o
calculator.cpp -> calculator.o
              -> linker가 app 생성
```

그래서 같은 전역 함수나 전역 변수가 여러 cpp에 정의되면 문제가 된다. 이 규칙을 ODR이라고 한다.

```text
ODR = One Definition Rule
하나의 프로그램 안에서 어떤 entity는 정의가 정확히 하나여야 한다.
```

잘못된 코드:

```cpp
// config.h
int count = 0;
```

```cpp
// a.cpp
#include "config.h"
```

```cpp
// b.cpp
#include "config.h"
```

`config.h`가 두 cpp에 들어가면 `count` 정의가 두 개 생긴다.

좋은 코드:

```cpp
// config.h
#pragma once

extern int count;
```

```cpp
// config.cpp
#include "config.h"

int count = 0;
```

C++17 이상이면 헤더에 `inline` 변수를 둘 수 있다.

```cpp
// config.h
#pragma once

inline int count = 0;
```

함수도 헤더에 정의하려면 보통 `inline`이 필요하다.

```cpp
inline int add(int a, int b) {
    return a + b;
}
```

template 함수/클래스는 컴파일러가 타입별 코드를 만들어야 하므로 보통 헤더에 정의까지 둔다.

---

## 68. 보충: storage duration과 linkage

C++ 객체는 어디에 만들어지고 얼마나 오래 사는지 중요하다.

```text
automatic storage duration: 지역 변수, 스코프 종료 시 소멸
static storage duration   : 전역/static 변수, 프로그램 종료 시 소멸
dynamic storage duration  : new/malloc 등으로 직접 확보
thread storage duration   : thread_local 변수, 스레드 종료 시 소멸
```

automatic:

```cpp
void f() {
    int x = 10;
} // x 소멸
```

static:

```cpp
int global = 10;

void f() {
    static int count = 0;
    ++count;
}
```

thread local:

```cpp
thread_local int perThreadCount = 0;
```

linkage는 이름이 다른 파일에서 보이는지와 관련 있다.

```cpp
int externalValue = 10;       // external linkage
static int internalValue = 20; // internal linkage, 이 cpp 안에서만 보임
```

현대 C++에서는 파일 내부 전용 함수/변수에 anonymous namespace를 자주 쓴다.

```cpp
namespace {
    int helperValue = 10;

    void helper() {}
}
```

잘못된 코드:

```cpp
// util.cpp
int helperValue = 10; // 다른 파일의 이름과 충돌할 수 있음
```

좋은 코드:

```cpp
// util.cpp
namespace {
    int helperValue = 10;
}
```

---

## 69. 보충: 초기화 함정

C++ 초기화는 문법이 많아서 헷갈린다.

```cpp
int a = 10;
int b(10);
int c{10};
int d{};
```

가능하면 `{}`를 쓰면 narrowing을 막아줘서 안전하다.

```cpp
int a = 3.14; // 3
int b{3.14};  // 컴파일 에러
```

### most vexing parse

C++에는 "객체 생성처럼 보이지만 함수 선언으로 해석되는" 함정이 있다.

잘못된 코드:

```cpp
class Timer {};

int main() {
    Timer timer(); // Timer 객체가 아니라 Timer를 반환하는 함수 선언처럼 해석됨
}
```

좋은 코드:

```cpp
Timer timer;
Timer timer{};
```

### aggregate initialization

생성자를 직접 만들지 않은 단순 struct는 중괄호로 멤버를 채울 수 있다.

```cpp
struct Point {
    int x;
    int y;
};

Point p{1, 2};
```

C++20부터는 designated initializer도 가능하다.

```cpp
Point p{.x = 1, .y = 2};
```

프로젝트가 C++17이면 위 문법은 못 쓴다.

---

## 70. 보충: signed, unsigned, `size_t`

컨테이너 크기는 보통 `std::size_t` 타입이다.

```cpp
std::vector<int> v{1, 2, 3};
std::size_t n = v.size();
```

`std::size_t`는 unsigned 정수다. 그래서 signed int와 섞으면 경고나 버그가 생기기 쉽다.

잘못된 코드:

```cpp
std::vector<int> v{1, 2, 3};

for (int i = 0; i < v.size(); ++i) { // int와 size_t 비교
    std::cout << v[i] << "\n";
}
```

좋은 코드:

```cpp
for (std::size_t i = 0; i < v.size(); ++i) {
    std::cout << v[i] << "\n";
}
```

더 좋은 코드:

```cpp
for (int value : v) {
    std::cout << value << "\n";
}
```

역방향 반복에서 unsigned는 특히 위험하다.

잘못된 코드:

```cpp
for (std::size_t i = v.size() - 1; i >= 0; --i) {
    std::cout << v[i] << "\n";
} // i >= 0은 unsigned라 항상 true
```

좋은 코드:

```cpp
for (std::size_t i = v.size(); i-- > 0;) {
    std::cout << v[i] << "\n";
}
```

또는 reverse iterator:

```cpp
for (auto it = v.rbegin(); it != v.rend(); ++it) {
    std::cout << *it << "\n";
}
```

---

## 71. 보충: 배열 decay와 포인터 산술

C-style 배열은 함수 인자로 넘어갈 때 포인터로 decay된다.

```cpp
void print(int arr[]) {
    // 실제 타입은 int*
}
```

아래 둘은 함수 인자에서는 거의 같다.

```cpp
void f(int arr[]);
void f(int* arr);
```

그래서 배열 크기 정보가 사라진다.

잘못된 코드:

```cpp
void printSize(int arr[]) {
    std::cout << sizeof(arr) << "\n"; // 배열 전체 크기가 아니라 포인터 크기
}
```

좋은 코드:

```cpp
void print(const std::vector<int>& values) {
    for (int value : values) {
        std::cout << value << "\n";
    }
}
```

고정 배열이면 template으로 크기를 받을 수 있다.

```cpp
template <std::size_t N>
void print(const int (&arr)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
        std::cout << arr[i] << "\n";
    }
}
```

포인터 산술:

```cpp
int arr[3] = {10, 20, 30};
int* p = arr;

std::cout << *p << "\n";       // 10
std::cout << *(p + 1) << "\n"; // 20
```

잘못된 코드:

```cpp
int arr[3] = {10, 20, 30};
int* p = arr + 10;
std::cout << *p << "\n"; // 범위 밖
```

현대 C++에서는 가능하면 `std::vector`, `std::array`, `std::span`을 먼저 고려한다.

---

## 72. 보충: `std::span`

`std::span`은 배열/vector의 데이터를 소유하지 않고 범위만 바라보는 view다. C++20 기능이다.

```cpp
#include <span>
#include <vector>

void print(std::span<const int> values) {
    for (int value : values) {
        std::cout << value << "\n";
    }
}

int main() {
    int arr[] = {1, 2, 3};
    std::vector<int> v{4, 5, 6};

    print(arr);
    print(v);
}
```

Java에서 `List<Integer>`를 읽기 전용으로 받는 느낌과 비슷하지만, `span`은 소유하지 않는다.

잘못된 코드:

```cpp
std::span<int> bad() {
    std::vector<int> v{1, 2, 3};
    return v; // v가 함수 끝나면 사라짐. dangling span
}
```

C++17 프로젝트라면 `std::span`은 표준에 없으니 vector reference나 iterator pair를 사용한다.

---

## 73. 보충: `= default`, `= delete`

컴파일러가 만들어주는 기본 함수를 명시적으로 쓰고 싶으면 `= default`를 쓴다.

```cpp
class Base {
public:
    virtual ~Base() = default;
};
```

특정 동작을 금지하고 싶으면 `= delete`를 쓴다.

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

사용:

```cpp
NonCopyable a;
// NonCopyable b = a; // 컴파일 에러
```

`std::unique_ptr`도 복사를 delete하고 이동만 허용하는 대표 예시다.

함수 overload를 막을 때도 쓸 수 있다.

```cpp
void print(int value) {
    std::cout << value << "\n";
}

void print(double) = delete;
```

```cpp
print(10);
// print(3.14); // 컴파일 에러
```

---

## 74. 보충: `final`, `override`, 상속 제한

`override`는 base의 virtual 함수를 정확히 재정의했는지 확인한다.

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;
};

class Dog : public Animal {
public:
    void speak() const override {}
};
```

`final`은 더 이상 override나 상속을 못 하게 한다.

```cpp
class Dog final : public Animal {
public:
    void speak() const override {}
};
```

함수에도 붙일 수 있다.

```cpp
class Base {
public:
    virtual void f() final {}
};
```

잘못된 코드:

```cpp
class Derived : public Base {
public:
    void f() override {} // Base::f가 final이면 컴파일 에러
};
```

상속 구조가 커질수록 `override`는 거의 필수 습관이라고 보면 된다.

---

## 75. 보충: overload resolution과 암시적 변환

C++은 overload 후보 중 가장 잘 맞는 함수를 고른다.

```cpp
void f(int) {
    std::cout << "int\n";
}

void f(double) {
    std::cout << "double\n";
}

f(10);   // int
f(3.14); // double
```

문제는 암시적 변환이 꽤 많이 일어난다는 점이다.

```cpp
void print(bool value) {
    std::cout << value << "\n";
}

print(10); // true로 변환됨
```

생성자도 `explicit`이 없으면 암시적 변환에 참여한다.

```cpp
class UserId {
public:
    UserId(int value) : value_(value) {}

private:
    int value_;
};

void load(UserId id) {}

load(10); // 가능하지만 의도가 흐려짐
```

좋은 코드:

```cpp
class UserId {
public:
    explicit UserId(int value) : value_(value) {}

private:
    int value_;
};

load(UserId{10});
```

---

## 76. 보충: `mutable`, logical const

`const` 멤버 함수는 객체의 관찰 가능한 상태를 바꾸지 않아야 한다.

하지만 캐시, mutex 같은 내부 구현 상태는 바꿔야 할 때가 있다. 이때 `mutable`을 쓴다.

```cpp
class Text {
public:
    explicit Text(std::string value)
        : value_(std::move(value)) {}

    std::size_t length() const {
        if (!cachedLength_) {
            cachedLength_ = value_.size();
        }
        return *cachedLength_;
    }

private:
    std::string value_;
    mutable std::optional<std::size_t> cachedLength_;
};
```

남용하면 const 의미가 무너진다. "외부에서 보기에 객체의 의미가 바뀌지 않는 내부 캐시" 정도에 제한해서 쓰는 게 좋다.

---

## 77. 보충: `volatile`은 멀티스레드 동기화가 아니다

Java의 `volatile`은 스레드 간 가시성 의미가 있다.

C++의 `volatile`은 그런 용도가 아니다. 메모리 mapped IO 같은 특수 상황에서 컴파일러 최적화를 제한하는 의미에 가깝다.

잘못된 코드:

```cpp
volatile bool done = false;

void worker() {
    while (!done) {
    }
}
```

이걸 스레드 동기화로 쓰면 안 된다.

좋은 코드:

```cpp
#include <atomic>

std::atomic<bool> done{false};

void worker() {
    while (!done.load()) {
    }
}
```

공유 데이터의 동기화는 `std::mutex`, `std::atomic`, condition variable 같은 도구로 한다.

---

## 78. 보충: condition variable

스레드가 어떤 조건이 될 때까지 기다려야 하면 condition variable을 쓴다.

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex m;
std::condition_variable cv;
std::queue<int> tasks;
bool done = false;

void worker() {
    while (true) {
        int task;

        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [] {
                return !tasks.empty() || done;
            });

            if (tasks.empty() && done) {
                break;
            }

            task = tasks.front();
            tasks.pop();
        }

        std::cout << "task: " << task << "\n";
    }
}
```

핵심:

- `std::unique_lock`은 lock/unlock을 유연하게 할 수 있다.
- `cv.wait(lock, predicate)` 형태를 쓰면 spurious wakeup을 안전하게 처리할 수 있다.
- predicate 없이 wait만 쓰는 코드는 버그가 나기 쉽다.

잘못된 코드:

```cpp
cv.wait(lock);
// 깨어났다고 항상 조건이 만족된 게 아님
```

좋은 코드:

```cpp
cv.wait(lock, [] {
    return !tasks.empty() || done;
});
```

---

## 79. 보충: comparator와 strict weak ordering

`std::sort`, `std::set`, `std::map`의 비교 함수는 strict weak ordering을 지켜야 한다.

간단히 말하면 비교 함수는 "작다"를 의미해야 한다.

```cpp
std::vector<int> v{3, 1, 2};

std::sort(v.begin(), v.end(), [](int a, int b) {
    return a < b;
});
```

잘못된 코드:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a <= b; // 같을 때도 true라 정렬 규칙을 깨뜨림
});
```

구조체 정렬:

```cpp
struct User {
    int id;
    std::string name;
};

std::vector<User> users{{2, "lee"}, {1, "kim"}};

std::sort(users.begin(), users.end(), [](const User& a, const User& b) {
    return a.id < b.id;
});
```

`std::set`에서 comparator가 같다고 판단하면 둘 중 하나만 들어갈 수 있다.

```cpp
struct CompareByName {
    bool operator()(const User& a, const User& b) const {
        return a.name < b.name;
    }
};

std::set<User, CompareByName> users;
```

name이 같으면 id가 달라도 같은 key처럼 취급된다.

---

## 80. 보충: `std::deque`, `priority_queue`

`std::vector`가 기본 선택이지만, 상황에 따라 다른 컨테이너가 낫다.

### `std::deque`

양쪽 끝 삽입/삭제가 필요하면 `deque`를 고려한다.

```cpp
#include <deque>

std::deque<int> dq;
dq.push_back(1);
dq.push_front(0);
dq.pop_back();
dq.pop_front();
```

### `std::priority_queue`

우선순위 큐다. 기본은 큰 값이 먼저 나온다.

```cpp
#include <functional>
#include <queue>

std::priority_queue<int> pq;
pq.push(3);
pq.push(1);
pq.push(5);

std::cout << pq.top() << "\n"; // 5
```

작은 값 먼저:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;
```

구조체 우선순위:

```cpp
struct Task {
    int priority;
    std::string name;
};

struct CompareTask {
    bool operator()(const Task& a, const Task& b) const {
        return a.priority < b.priority; // priority 큰 것이 먼저
    }
};

std::priority_queue<Task, std::vector<Task>, CompareTask> tasks;
```

---

## 81. 보충: `std::any`, `std::variant`, 상속 중 선택

여러 타입을 담고 싶을 때 선택지가 있다.

```text
타입 후보가 정해져 있음       -> std::variant
정말 아무 타입이나 담아야 함   -> std::any
공통 interface로 동작해야 함   -> virtual base class
```

`std::any`:

```cpp
#include <any>

std::any value = 10;
value = std::string{"hello"};
```

꺼내기:

```cpp
if (auto* s = std::any_cast<std::string>(&value)) {
    std::cout << *s << "\n";
}
```

잘못된 코드:

```cpp
std::any value = 10;
std::cout << std::any_cast<std::string>(value) << "\n"; // bad_any_cast
```

가능하면 `std::any`보다 `std::variant`가 타입이 더 명확하다.

---

## 82. 보충: 디버깅 빌드, sanitizer, warning

C++은 undefined behavior가 많아서 컴파일 옵션이 매우 중요하다.

학습용으로는 warning을 강하게 켜는 게 좋다.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o app
```

디버깅 정보:

```bash
g++ -std=c++17 -g main.cpp -o app
```

AddressSanitizer:

```bash
g++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer main.cpp -o app
./app
```

UndefinedBehaviorSanitizer:

```bash
g++ -std=c++17 -g -fsanitize=undefined main.cpp -o app
./app
```

둘을 같이 켤 수도 있다.

```bash
g++ -std=c++17 -g -fsanitize=address,undefined -fno-omit-frame-pointer main.cpp -o app
```

예시:

```cpp
int main() {
    int* p = new int[3]{1, 2, 3};
    int x = p[10]; // heap buffer overflow
    delete[] p;
    return x;
}
```

그냥 실행하면 이상하게 조용할 수도 있지만, sanitizer를 켜면 문제 위치를 훨씬 잘 알려준다.

---

## 83. 추천 학습 순서

1. `main`, include, namespace, primitive type
2. 값/포인터/참조
3. 함수 인자 전달: 값, `T&`, `const T&`, `T*`
4. class, 생성자, 소멸자, RAII
5. `std::string`, `std::vector`, `std::map`
6. iterator와 algorithm
7. 복사/이동, Rule of Zero
8. 스마트 포인터
9. 상속, virtual, override
10. template
11. 예외, optional
12. thread, mutex, atomic

---

## 84. 마지막 감각 정리

C++ 문법은 넓지만, 좋은 C++ 코드는 의외로 몇 가지 기준을 계속 반복한다.

```text
소유하는가?
수명은 어디까지인가?
복사되는가, 이동되는가, 참조되는가?
null이 가능한가?
범위 밖 접근은 없는가?
상속이면 virtual 소멸자가 필요한가?
헤더에 정의해도 되는 코드인가?
```

Java에서 C++로 넘어올 때는 "객체는 전부 참조"라는 감각을 잠깐 내려놓고, "이 변수 자체가 객체일 수 있다"는 감각을 잡는 게 제일 중요하다.

C++은 GC가 없는 대신, 스코프와 타입으로 수명을 아주 정확하게 표현할 수 있다. 그게 익숙해지면 RAII, `std::vector`, `std::string`, `std::unique_ptr` 같은 도구들이 Java의 GC와는 다른 방식으로 코드를 안전하게 만들어준다.
