# GTest/GMock C++ 테스트 문법 가이드

RDK C++ 테스트에서는 Google Test와 Google Mock을 주로 사용합니다. 이 문서는 테스트 코드를 바로 작성할 때 필요한 문법과 예시를 모은 참조 문서입니다.

## 1. 기본 include

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
```

예제에서 callback, thread, tuple, smart pointer를 쓰면 다음 표준 헤더도 필요합니다.

```cpp
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
```

자주 쓰는 matcher/action은 필요한 범위에서만 가져옵니다.

```cpp
using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::Contains;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;
```

## 2. TEST와 TEST_F

fixture가 필요 없으면 `TEST`, 공통 준비물이 있으면 `TEST_F`를 사용합니다.

```cpp
TEST(JsonRpcIdTest, Next_ReturnsIncreasingValue) {
    JsonRpcId id;

    EXPECT_EQ(id.next(), 1);
    EXPECT_EQ(id.next(), 2);
}
```

```cpp
class DisplayServiceTest : public ::testing::Test {
protected:
    MockDisplayHal hal_;
    DisplayService service_{hal_};
};

TEST_F(DisplayServiceTest, GetResolution_WhenHalSucceeds_ReturnsResolution) {
    EXPECT_CALL(hal_, getResolution())
        .WillOnce(Return(Resolution{3840, 2160}));

    Resolution result = service_.getResolution();

    EXPECT_EQ(result.width, 3840);
    EXPECT_EQ(result.height, 2160);
}
```

## 3. ASSERT와 EXPECT

| 매크로 | 실패 시 동작 | 사용 기준 |
| --- | --- | --- |
| `ASSERT_*` | 현재 함수 즉시 종료 | 이후 검증이 무의미하거나 위험할 때 |
| `EXPECT_*` | 실패 기록 후 계속 실행 | 여러 값을 함께 진단하고 싶을 때 |

```cpp
auto response = client.call("DeviceInfo.1.model");

ASSERT_TRUE(response.has_value());
EXPECT_EQ(response->status, 200);
EXPECT_EQ(response->body["success"], true);
```

## 4. Mock 클래스

```cpp
class IDeviceHal {
public:
    virtual ~IDeviceHal() = default;
    virtual std::string getModel() = 0;
    virtual bool setPowerState(PowerState state) = 0;
};

class MockDeviceHal : public IDeviceHal {
public:
    MOCK_METHOD(std::string, getModel, (), (override));
    MOCK_METHOD(bool, setPowerState, (PowerState state), (override));
};
```

`const`, reference, `noexcept`, overload signature가 원본 virtual method와 정확히 맞아야 합니다.

## 5. EXPECT_CALL과 ON_CALL

`EXPECT_CALL`은 호출 여부까지 검증합니다.

```cpp
EXPECT_CALL(hal_, setPowerState(PowerState::Standby))
    .Times(1)
    .WillOnce(Return(true));

EXPECT_TRUE(service_.enterStandby());
```

`ON_CALL`은 기본 동작만 제공합니다.

```cpp
ON_CALL(hal_, getModel())
    .WillByDefault(Return("RDK-Reference"));

EXPECT_EQ(service_.model(), "RDK-Reference");
```

로그나 metrics처럼 테스트 목적과 무관한 호출은 과도하게 `EXPECT_CALL`로 묶지 않습니다.

## 6. Matcher

```cpp
EXPECT_CALL(transport_, send(::testing::HasSubstr("DeviceInfo.1.model")));
EXPECT_CALL(audio_, setVolume(AllOf(Ge(0), Le(100))));
EXPECT_THAT(modes, Contains(Resolution{3840, 2160}));
```

JSON-RPC payload는 문자열 전체 비교보다 구조 비교가 안정적입니다.

```cpp
MATCHER_P(JsonMethodIs, expected, "checks JSON-RPC method") {
    return arg.contains("method") && arg["method"] == expected;
}

EXPECT_CALL(transport_, send(JsonMethodIs("DeviceInfo.1.model")));
```

## 7. Argument capture

콜백 등록이나 이벤트 구독 테스트에서는 인자를 캡처해서 직접 호출합니다.

```cpp
std::function<void(Event)> captured;

EXPECT_CALL(eventSource_, subscribe(_))
    .WillOnce(SaveArg<0>(&captured));

service_.start();
captured(Event{"hdmi.connected"});

EXPECT_TRUE(service_.isHdmiConnected());
```

람다를 쓰면 캡처와 추가 동작을 함께 처리할 수 있습니다.

```cpp
EXPECT_CALL(bus_, subscribe("network.changed", _))
    .WillOnce([&] (const std::string&, EventHandler handler) {
        capturedHandler = handler;
    });
```

## 8. Action 예시

`WillOnce(Return(...))`만으로 부족할 때는 `DoAll`, `SetArgPointee`, `Invoke`, `ByMove` 등을 사용합니다.

### output parameter 채우기

```cpp
EXPECT_CALL(hal_, getResolution(_))
    .WillOnce(::testing::DoAll(
        ::testing::SetArgPointee<0>(Resolution{3840, 2160}),
        Return(true)
    ));

Resolution resolution{};
ASSERT_TRUE(service_.readResolution(&resolution));
EXPECT_EQ(resolution.width, 3840);
```

### 인자를 검사하면서 동작 실행

```cpp
EXPECT_CALL(transport_, send(_))
    .WillOnce(::testing::Invoke([] (const Json& request) {
        EXPECT_EQ(request["method"], "DeviceInfo.1.model");
        return Json{{"result", "RDK-Reference"}};
    }));
```

### move-only 반환

```cpp
EXPECT_CALL(factory_, createClient())
    .WillOnce(Return(::testing::ByMove(std::make_unique<JsonRpcClient>())));
```

## 9. 순서 검증

초기화/해제 순서처럼 계약 자체가 순서인 경우에만 `InSequence`를 사용합니다.

```cpp
{
    ::testing::InSequence seq;
    EXPECT_CALL(hal_, initialize()).WillOnce(Return(true));
    EXPECT_CALL(hal_, open()).WillOnce(Return(true));
    EXPECT_CALL(hal_, close()).WillOnce(Return(true));
}
```

## 10. StrictMock과 NiceMock

| 타입 | 특징 | 권장 상황 |
| --- | --- | --- |
| `StrictMock<T>` | 예상하지 않은 호출을 실패 처리 | 외부 계약을 엄격히 고정할 때 |
| `NiceMock<T>` | 예상하지 않은 호출을 허용 | 로그/metrics처럼 부수 호출이 많을 때 |
| 기본 Mock | 예상하지 않은 호출에 경고 | 일반적인 개발 중 테스트 |

## 11. 파라미터화 테스트

```cpp
class VolumeClampTest : public ::testing::TestWithParam<std::tuple<int, int>> {};

TEST_P(VolumeClampTest, Clamp_ReturnsValueInRange) {
    const int input = std::get<0>(GetParam());
    const int expected = std::get<1>(GetParam());

    EXPECT_EQ(clampVolume(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    VolumeCases,
    VolumeClampTest,
    ::testing::Values(
        std::make_tuple(-1, 0),
        std::make_tuple(50, 50),
        std::make_tuple(101, 100)
    )
);
```

C++17 이상이 보장되는 프로젝트라면 구조화된 바인딩을 사용할 수 있습니다. 그렇지 않으면 `std::get`을 쓰면 호환성이 좋습니다.

## 12. 예외 테스트

```cpp
TEST(ConfigParserTest, Parse_WhenRequiredFieldMissing_ThrowsInvalidConfig) {
    ConfigParser parser;

    EXPECT_THROW(parser.parse(R"({"name":"device"})"), InvalidConfig);
}
```

RDK production 코드가 예외를 쓰지 않고 error code/result type을 쓰는 프로젝트라면 예외 테스트보다 result 검증을 우선합니다.

## 13. Death test

프로세스 종료, `ASSERT_DEATH`, fatal check를 검증할 때만 사용합니다.

```cpp
TEST(ProcessGuardTest, Abort_WhenInitializedTwice) {
    ProcessGuard guard;
    guard.initialize();

    ASSERT_DEATH(guard.initialize(), "already initialized");
}
```

Death test는 느리고 플랫폼 차이를 탈 수 있으므로 일반 오류 처리 테스트로 대체 가능한지 먼저 봅니다.

## 14. 비동기 callback 테스트

```cpp
TEST_F(EventDispatcherTest, Dispatch_InvokesRegisteredCallback) {
    bool called = false;

    dispatcher_.on("power.changed", [&] (const Event& event) {
        called = event.name == "power.changed";
    });

    dispatcher_.dispatch(Event{"power.changed"});

    EXPECT_TRUE(called);
}
```

thread가 관여하면 timeout이 있는 latch/helper를 사용합니다.

```cpp
TEST_F(AsyncEventTest, EmitsEventWithinTimeout) {
    std::mutex mutex;
    std::condition_variable cv;
    bool received = false;

    service_.onEvent([&] {
        std::lock_guard<std::mutex> lock(mutex);
        received = true;
        cv.notify_one();
    });

    service_.startAsyncWork();

    std::unique_lock<std::mutex> lock(mutex);
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&] {
        return received;
    }));
}
```

## 15. Fixture에서 자원 정리

RDK 테스트는 전역 상태와 외부 자원이 섞이기 쉬우므로 fixture의 `TearDown`에서 자원을 명확히 정리합니다.

```cpp
class ThunderClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_unique<ThunderClient>(endpoint_);
    }

    void TearDown() override {
        client_.reset();
        tokenCache_.clear();
    }

    std::string endpoint_ = "http://127.0.0.1:9998/jsonrpc";
    TokenCache tokenCache_;
    std::unique_ptr<ThunderClient> client_;
};
```

## 16. CMake 예시

```cmake
cmake_minimum_required(VERSION 3.16)
project(rdk_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
find_package(GTest REQUIRED)

add_executable(rdk_unit_tests
    DeviceServiceTest.cpp
)

target_link_libraries(rdk_unit_tests
    PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(rdk_unit_tests)
```

사내 RDK SDK나 Yocto sysroot를 쓰는 경우 `find_package` 대신 toolchain file, sysroot include path, 사내 제공 CMake package를 우선 확인합니다.

## 17. 실행 명령

```bash
ctest --test-dir build --output-on-failure
./rdk_unit_tests --gtest_filter="DisplayServiceTest.*"
./rdk_unit_tests --gtest_repeat=50 --gtest_break_on_failure
./rdk_unit_tests --gtest_output=xml:test-results.xml
```

## 18. 자주 나는 오류

| 증상 | 원인 | 해결 |
| --- | --- | --- |
| expected once, never called | 실제 코드 경로가 기대 호출을 타지 않음 | 입력 조건과 guard clause 확인 |
| called more times than expected | 테스트가 너무 엄격하거나 retry가 존재 | `Times`와 실제 계약 재검토 |
| no matching function for `MOCK_METHOD` | signature 불일치 | 원본 virtual method와 완전히 맞춤 |
| flaky async test | 고정 sleep 또는 race condition | 조건 대기, event latch 사용 |
| uninteresting mock function call | 테스트 목적과 무관한 호출을 엄격하게 다룸 | `NiceMock` 또는 `ON_CALL` 검토 |
| leaked mock object | mock 수명 관리 실패 | fixture 멤버 또는 smart pointer 사용 |

## 19. 참고 자료

- [GoogleTest Primer](https://google.github.io/googletest/primer.html)
- [Google Mock Cookbook](https://google.github.io/googletest/gmock_cook_book.html)
