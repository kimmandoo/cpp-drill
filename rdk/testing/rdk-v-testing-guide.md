# RDK-V 테스트 가이드

RDK-V 테스트는 일반 C++ 테스트보다 계층 경계가 중요합니다. 앱, Firebolt, Thunder/RDK Services, IPC, HAL, SoC가 이어지는 구조라서 "어느 계층의 계약을 검증하는가"를 먼저 정해야 테스트가 빠르고 안정적입니다.

> 기준: RDK/Firebolt/Thunder 구성은 제품 image, SoC BSP, operator layer, 사내 SDK에 따라 달라질 수 있습니다. 특정 Thunder 버전, GoogleTest 버전, C++ 표준은 문서에서 단정하지 말고 실제 `CMakeLists.txt`, BitBake recipe, sysroot, manifest로 확인합니다.

## 1. 전체 구조

이 문서는 RDK-V 테스트에 초점을 둡니다. RDK-B, RDK-C도 HAL과 서비스 경계라는 큰 원칙은 비슷하지만, 사용하는 framework와 component 이름이 다를 수 있으므로 그대로 복사하기보다 프로젝트 구조에 맞게 적용해야 합니다.

```text
Application / App Runtime
  -> Firebolt API / SDK
  -> RDK Services / Thunder JSON-RPC
  -> Middleware / IPC / Device services
  -> HAL
  -> SoC / Driver / Hardware
```

공식 RDK 문서는 RDK-V를 애플리케이션, 애플리케이션 플랫폼, 미들웨어, HAL, SoC 계층으로 설명합니다. 테스트에서도 이 경계를 유지하면 Mock 위치와 실제 통합 지점을 정하기 쉽습니다.

### 용어 정리

| 용어 | 의미 | 테스트에서 보는 지점 |
| --- | --- | --- |
| RDK-V | Video/STB/TV 계열 RDK stack | 앱, media, RDK Services, HAL |
| Firebolt | 앱이 platform 기능을 호출하는 API 계층 | SDK, JSON-RPC, Capabilities |
| Thunder/WPEFramework | plugin hosting과 JSON-RPC routing 계층 | plugin lifecycle, 보안 토큰, transport |
| RDK Services | Thunder plugin으로 구현되는 set-top box 기능 서비스 | method contract, event, response schema |
| HAL | 하드웨어별 구현을 감추는 API 경계 | error code, init/deinit, optional 기능 |

## 2. 테스트 레벨

| 레벨 | 목적 | 실행 위치 | 예 |
| --- | --- | --- | --- |
| L1 | 단위/계약 테스트 | 개발 PC, CI | HAL adapter, parser, JSON payload builder |
| L2 | 모듈 통합 테스트 | 개발 PC 또는 타겟 | Thunder plugin + fake HAL |
| L3 | 실제 시나리오 테스트 | RDK 타겟 | 앱 실행, playback, Firebolt API 호출 |

L1은 많고 빨라야 합니다. L2는 계층 경계 계약 중심으로 둡니다. L3는 실제 사용자 가치가 큰 시나리오만 남기고 별도 레이블로 분리합니다.

## 3. 테스트 설계 원칙

### 회귀 방지

RDK-V 코드는 한 계층의 변경이 다른 계층 동작을 깨뜨릴 수 있습니다. 테스트는 변경 전 기대 동작을 고정하는 안전장치입니다.

```cpp
TEST(DisplayModeSelectorTest, SelectBestMode_When4k60Exists_Returns4k60) {
    DisplayModeSelector selector;
    std::vector<Mode> modes = {
        {"1920x1080", 60},
        {"3840x2160", 30},
        {"3840x2160", 60}
    };

    Mode selected = selector.selectBest(modes);

    EXPECT_EQ(selected.resolution, "3840x2160");
    EXPECT_EQ(selected.refreshRate, 60);
}
```

### 설계 개선

테스트하기 어려운 코드는 대개 결합도가 높습니다.

| 의존성 | 문제 | 개선 방향 |
| --- | --- | --- |
| 직접 HAL 호출 | 실제 하드웨어 없이는 테스트 불가 | HAL adapter 인터페이스 |
| 직접 Thunder 호출 | daemon/network/security 상태에 의존 | JSON-RPC client interface |
| 전역 singleton | 테스트 간 상태 오염 | fixture에서 수명 관리 |
| 고정 `sleep` | 느리고 flaky함 | timeout 기반 조건 대기 |
| 문자열 JSON 비교 | 공백/필드 순서에 취약 | JSON parser 기반 검증 |

## 4. 권장 디렉터리

```text
tests/
  CMakeLists.txt
  unit/
    display/
    device/
    firebolt/
  integration/
    thunder/
    hal/
  e2e/
    scenarios/
  mocks/
    MockDisplayHal.h
    MockJsonRpcClient.h
    MockFireboltTransport.h
  helpers/
    AsyncWait.h
    JsonRpcFixture.h
    TestDataBuilders.h
```

## 5. HAL 테스트

HAL은 C API인 경우가 많으므로 직접 전역 함수를 호출하는 코드보다 adapter를 두는 편이 테스트하기 쉽습니다.

```cpp
class IDisplayHal {
public:
    virtual ~IDisplayHal() = default;
    virtual HalResult initialize() = 0;
    virtual HalResult setResolution(int width, int height) = 0;
    virtual std::vector<Resolution> supportedResolutions() = 0;
};

class MockDisplayHal : public IDisplayHal {
public:
    MOCK_METHOD(HalResult, initialize, (), (override));
    MOCK_METHOD(HalResult, setResolution, (int width, int height), (override));
    MOCK_METHOD(std::vector<Resolution>, supportedResolutions, (), (override));
};
```

```cpp
TEST_F(DisplayControllerTest, SetResolution_WhenHalFails_ReturnsHardwareError) {
    EXPECT_CALL(hal_, setResolution(3840, 2160))
        .WillOnce(::testing::Return(HalResult::Failure));

    Result result = controller_.setResolution({3840, 2160});

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error(), ErrorCode::HardwareFailure);
}
```

### HAL 체크리스트

- `initialize` 전 호출, `deinitialize` 후 호출을 어떻게 처리하는가?
- HAL error code가 domain error로 올바르게 변환되는가?
- optional 기능 미지원 시 fallback이 있는가?
- callback과 동시 호출이 가능한 API에 race가 없는가?
- 실패 경로에서도 handle, memory, fd가 정리되는가?

### C HAL 함수 포인터 adapter 예시

기존 C HAL을 직접 Mock하기 어려우면 함수 포인터 테이블을 한 번 감싸서 테스트합니다.

```cpp
struct DisplayHalApi {
    int (*init)();
    int (*setResolution)(int width, int height);
    int (*term)();
};

class DisplayHalAdapter : public IDisplayHal {
public:
    explicit DisplayHalAdapter(DisplayHalApi api) : api_(api) {}

    HalResult initialize() override {
        return api_.init() == 0 ? HalResult::Success : HalResult::Failure;
    }

    HalResult setResolution(int width, int height) override {
        return api_.setResolution(width, height) == 0
            ? HalResult::Success
            : HalResult::Failure;
    }

private:
    DisplayHalApi api_;
};
```

이 예시는 실제 RDK HAL 함수명이 아닙니다. 프로젝트의 HAL header와 Doxygen 문서에서 함수명, 반환값, thread-safety 조건을 확인한 뒤 adapter를 작성합니다.

## 6. Thunder/RDK Services 테스트

RDK Services는 Thunder framework에서 관리되고 JSON-RPC로 접근되는 서비스 계층입니다. 공식 문서 기준으로 RDK Services는 set-top box 기능에 접근하기 위한 JSON-RPC 기반 서비스이며 Thunder가 HTTP와 websocket 요청을 지원합니다.

RDK Services plugin은 Thunder plugin contract를 따릅니다. 공식 문서는 plugin이 `PluginHost::IPlugin` 인터페이스를 따르고, `rdkservices` library의 `AbstractPlugin` helper를 확장할 수 있다고 설명합니다. 테스트에서는 plugin object를 직접 호출하는 L1과 실제 Thunder runtime을 통하는 L2/L3를 분리합니다.

### JSON-RPC client 경계

```cpp
class IJsonRpcClient {
public:
    virtual ~IJsonRpcClient() = default;
    virtual Json call(const std::string& method, const Json& params) = 0;
};
```

```cpp
TEST_F(DeviceInfoRpcTest, Model_WhenServiceSucceeds_ReturnsJsonRpcResult) {
    EXPECT_CALL(deviceInfo_, model())
        .WillOnce(::testing::Return("RDK-Reference"));

    Json response = plugin_.invoke("DeviceInfo.1.model", Json::object());

    ASSERT_EQ(response["success"], true);
    EXPECT_EQ(response["model"], "RDK-Reference");
}
```

### Plugin lifecycle 예시

```cpp
TEST_F(DeviceInfoPluginTest, Initialize_WhenHalReady_RegistersMethods) {
    EXPECT_CALL(hal_, initialize())
        .WillOnce(::testing::Return(HalResult::Success));

    std::string message = plugin_.Initialize(&service_);

    EXPECT_TRUE(message.empty());
    EXPECT_TRUE(plugin_.hasMethod("DeviceInfo.1.model"));
}

TEST_F(DeviceInfoPluginTest, Deinitialize_ReleasesHal) {
    EXPECT_CALL(hal_, shutdown()).Times(1);

    plugin_.Deinitialize(&service_);
}
```

실제 method 등록 API와 lifecycle signature는 사용하는 Thunder/RDK Services 버전에 따라 다를 수 있습니다. 이 예시는 "초기화 성공 시 method 등록", "해제 시 HAL 자원 정리"라는 테스트 관점을 보여주기 위한 형태입니다.

### Thunder 보안 토큰 테스트

타겟 환경에서는 보안 토큰이 필요할 수 있습니다. 이 경우 L1에서는 token provider를 Mock 처리하고, L2/L3에서 실제 token 획득 실패를 진단합니다.

```cpp
class ISecurityTokenProvider {
public:
    virtual ~ISecurityTokenProvider() = default;
    virtual Result<std::string> token() = 0;
};

TEST_F(JsonRpcClientTest, Call_WhenTokenUnavailable_ReturnsAuthError) {
    EXPECT_CALL(tokenProvider_, token())
        .WillOnce(::testing::Return(Result<std::string>::error(ErrorCode::AuthUnavailable)));

    auto result = client_.call("DeviceInfo.1.model", Json::object());

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error(), ErrorCode::AuthUnavailable);
}
```

## 7. IPC/RPC 테스트

프로젝트에 따라 IARM-Bus, RBus, Thunder JSON-RPC 등 여러 통신 방식이 등장할 수 있습니다. 테스트에서는 transport를 인터페이스로 감싸고 payload 변환과 event dispatch를 분리합니다.

```cpp
class IEventBus {
public:
    virtual ~IEventBus() = default;
    virtual void publish(const std::string& name, const Json& payload) = 0;
    virtual void subscribe(const std::string& name, EventHandler handler) = 0;
};
```

```cpp
TEST_F(HdmiEventTest, OnBusEvent_UpdatesConnectionState) {
    EventHandler handler;

    EXPECT_CALL(bus_, subscribe("hdmi.connected", _))
        .WillOnce([&] (const std::string&, EventHandler h) {
            handler = h;
        });

    monitor_.start();
    handler(Json{{"port", "HDMI1"}, {"connected", true}});

    EXPECT_TRUE(monitor_.isConnected("HDMI1"));
}
```

## 8. 비동기 테스트

RDK 이벤트 테스트는 고정 `sleep`을 피하고 조건 대기를 사용합니다.

```cpp
template <typename Predicate>
bool waitUntil(Predicate predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}
```

```cpp
ASSERT_TRUE(waitUntil([&] {
    return observer.received("onHdmiConnected");
}, std::chrono::seconds(2))) << "HDMI connected event was not received";
```

## 9. RDK7/RDK8 계열 차이 다루기

RDK7/RDK8이라는 이름이 릴리스 계열을 나타내더라도 모든 저장소가 같은 C++ 표준, Thunder 버전, GoogleTest 버전을 쓰는 것은 아닙니다.

| 위험한 단정 | 안전한 표현 |
| --- | --- |
| "RDK7은 C++14, RDK8은 C++17이다" | 프로젝트 toolchain에서 요구하는 C++ 표준을 확인한다 |
| "RDK8은 특정 gmock 버전 이상이다" | SDK/sysroot가 제공하는 GoogleTest 버전을 확인한다 |
| "Thunder는 특정 major 버전으로 나뉜다" | 사용 중인 Thunder/WPEFramework API와 plugin contract를 확인한다 |
| "Firebolt는 특정 image에 항상 포함된다" | 제품 image와 Firebolt manifest에서 지원 여부를 확인한다 |

### 호환성 CMake 예시

```cmake
option(RDK_USE_FIREBOLT "Enable Firebolt tests" ON)
option(RDK_ENABLE_TARGET_TESTS "Enable target-only tests" OFF)

if (RDK_USE_FIREBOLT)
    target_compile_definitions(rdk_tests PRIVATE RDK_USE_FIREBOLT=1)
endif()
```

버전 분기는 테스트 본문 곳곳에 흩뿌리지 말고 adapter, fixture, compile definition에서 흡수합니다.

## 10. CI/CD 구성

```cmake
add_executable(rdk_l1_tests
    unit/display/DisplayControllerTest.cpp
    unit/device/DeviceInfoTest.cpp
)

target_link_libraries(rdk_l1_tests
    PRIVATE rdk_test_common GTest::gtest GTest::gmock GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(rdk_l1_tests PROPERTIES LABELS "L1")

if (RDK_ENABLE_TARGET_TESTS)
    add_executable(rdk_l3_tests e2e/AppLifecycleTest.cpp)
    add_test(NAME rdk_l3_tests COMMAND rdk_l3_tests)
    set_tests_properties(rdk_l3_tests PROPERTIES LABELS "L3;target")
endif()
```

```bash
ctest -L L1 --output-on-failure
ctest -L L2 --output-on-failure
ctest -L "L3|target" --output-on-failure
```

### 실패 로그 수집 예시

```bash
#!/bin/sh
set -eu

./rdk_l3_tests --gtest_output=xml:/tmp/rdk-l3-results.xml || status=$?

journalctl -u thunder --no-pager > /tmp/thunder.log 2>/dev/null || true
cp /opt/logs/rdkservices.log /tmp/rdkservices.log 2>/dev/null || true

exit "${status:-0}"
```

타겟마다 service manager와 log path가 다를 수 있으므로, 실제 제품 image의 로그 수집 규칙에 맞춰 조정합니다.

## 11. 타겟 테스트 운영 체크리스트

- 테스트 시작 전에 필요한 daemon 상태를 확인합니다.
- 타겟 image version, git revision, feature flag를 로그에 남깁니다.
- 실패 시 Thunder/RDK service log, app log, crash dump 위치를 출력합니다.
- 권한/token 실패와 API 실패를 구분합니다.
- L3는 PR마다 전부 돌리기보다 nightly 또는 release gate로 운용합니다.

## 12. Anti-pattern

| 패턴 | 문제 | 대안 |
| --- | --- | --- |
| 테스트 간 순서 의존 | 단독 실행 시 실패 | 각 테스트가 자기 데이터를 준비 |
| 내부 구현 호출 순서 과검증 | 리팩터링에 취약 | 외부 관찰 가능한 결과 검증 |
| 실제 서비스 항상 호출 | 느리고 불안정 | L1은 Mock, L2/L3만 실제 연결 |
| 고정 sleep | flaky 또는 느림 | 조건 대기 + timeout |
| 버전 번호 하드코딩 | 플랫폼 변경 시 오정보 | toolchain/manifest 기준 확인 |

## 13. 참고 자료

- [RDK6 Architecture](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/rdk6/architecture/)
- [RDK7 Architecture](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/rdk7/rdk7-architecture/)
- [RDK Services](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/sub-systems/rdkservices/)
- [RDK Video HAL](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/hal/)
- [Thunder Security](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/sub-systems/rdkservices/thunder_security/)
