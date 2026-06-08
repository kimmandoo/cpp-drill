# Firebolt 테스트 가이드

Firebolt는 RDK-V 애플리케이션이 디바이스 기능에 접근할 때 사용하는 표준 API 계층입니다. 공식 Firebolt 문서 기준으로 API는 OpenRPC schema로 정의되고, 이 schema에서 SDK와 문서 artifact가 생성됩니다. 따라서 테스트의 중심은 "SDK 호출이 올바른 JSON-RPC 계약과 권한 흐름을 따르는가"입니다.

## 1. 테스트 경계

```text
Application code
  -> Firebolt SDK
  -> Transport / JSON-RPC
  -> Firebolt platform implementation
  -> RDK Services / Thunder / HAL
```

L1 테스트에서는 transport를 Mock으로 대체합니다. 실제 platform implementation, Thunder, HAL까지 검증하는 테스트는 L2/L3로 분리합니다.

## 2. 핵심 개념

| 개념 | 설명 | 테스트 포인트 |
| --- | --- | --- |
| OpenRPC schema | Firebolt API 계약의 원천 | method, params, result shape |
| SDK | schema를 따르는 API wrapper | 함수 호출 -> JSON-RPC 변환 |
| Transport | JSON-RPC request/response 전달 계층 | request id, timeout, error propagation |
| Capabilities | API 사용 권한과 device 지원 범위 | supported, available, permitted, granted |
| Events | 플랫폼 상태 변경 알림 | subscribe, unsubscribe, callback |

## 3. JSON-RPC 구조

요청:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "Device.model",
  "params": {}
}
```

성공 응답:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": "RDK-Reference"
}
```

오류 응답:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32001,
    "message": "Permission denied"
  }
}
```

테스트에서는 문자열 전체를 비교하지 말고 JSON 구조로 검증합니다.

```cpp
MATCHER_P(JsonMethodIs, expected, "checks JSON-RPC method") {
    return arg.contains("method") && arg["method"] == expected;
}

TEST_F(FireboltDeviceTest, Model_SendsDeviceModelMethod) {
    EXPECT_CALL(transport_, send(JsonMethodIs("Device.model")))
        .WillOnce(::testing::Return(Json{
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"result", "RDK-Reference"}
        }));

    std::string model = device_.model();

    EXPECT_EQ(model, "RDK-Reference");
}
```

## 4. Mock Transport 패턴

```cpp
class IFireboltTransport {
public:
    virtual ~IFireboltTransport() = default;
    virtual Json send(const Json& request) = 0;
    virtual Subscription subscribe(const std::string& event, EventHandler handler) = 0;
    virtual void unsubscribe(Subscription subscription) = 0;
};

class MockFireboltTransport : public IFireboltTransport {
public:
    MOCK_METHOD(Json, send, (const Json& request), (override));
    MOCK_METHOD(Subscription, subscribe, (const std::string& event, EventHandler handler), (override));
    MOCK_METHOD(void, unsubscribe, (Subscription subscription), (override));
};
```

이 패턴을 쓰면 앱 코드 테스트가 실제 RDK image, network, Thunder process에 의존하지 않습니다.

## 5. Capabilities 테스트

Firebolt Capabilities는 device가 어떤 기능을 지원하고, 앱이 어떤 권한으로 사용할 수 있는지 결정합니다. 공식 요구사항은 capability가 `use`, `manage`, `provide` 역할로 표현될 수 있고, API가 필요한 capability를 OpenRPC schema에 표시해야 한다고 설명합니다.

### supported, available, permitted, granted

| 상태 | 의미 | 테스트 포인트 |
| --- | --- | --- |
| supported | device가 기능을 지원함. 공식 요구사항 기준 runtime에 변하지 않는 set로 다룸 | device manifest parsing |
| available | 지금 이 순간 기능을 사용할 수 있음 | 네트워크/리소스/플랫폼 상태 변화 |
| permitted | 앱 또는 role이 capability를 호출할 수 있도록 허용됨 | app manifest, distributor policy |
| granted | 사용자 grant가 필요한 capability에 대해 grant가 있음 | request/deny/revoke 흐름 |

이 네 가지를 섞으면 테스트가 부정확해집니다. 예를 들어 Wi-Fi 기능은 supported일 수 있지만 현재 네트워크 리소스 상태 때문에 available이 아닐 수 있습니다.

| 역할 | 의미 | 예 |
| --- | --- | --- |
| `use` | 앱이 기능을 사용 | 앱이 closed captions 설정 조회 |
| `manage` | 관리 UI나 플랫폼이 기능을 관리 | settings app이 captions 활성화 |
| `provide` | 앱/플랫폼이 기능을 제공 | provider app이 특정 capability 제공 |

### 검증 시나리오

| 시나리오 | 기대 |
| --- | --- |
| capability 지원 + grant 있음 | API 호출 성공 |
| capability 미지원 | unavailable error 또는 기능 비활성 |
| grant 필요 + grant 없음 | grant flow 또는 permission error |
| optional capability 없음 | 핵심 기능은 성공, optional 기능만 제외 |
| `allOf` capability 일부 누락 | API 차단 |
| `anyOf` capability 일부 존재 | 가능한 구현 경로로 API 수행 |

```cpp
TEST_F(CapabilityTest, ProtectedApi_WhenGrantDenied_ReturnsPermissionError) {
    EXPECT_CALL(capabilities_, hasGrant("xrn:firebolt:capability:device:model"))
        .WillOnce(::testing::Return(false));

    Result<std::string> result = device_.model();

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error(), FireboltError::PermissionDenied);
}
```

## 6. Optional capability 예시

선택 capability가 없을 때 핵심 API까지 실패시키면 안 되는 경우가 있습니다. 공식 요구사항 기준으로 API 수행에 반드시 필요한 capability는 OpenRPC method schema의 capabilities tag에 들어가야 하지만, 기능을 향상시키는 optional capability는 해당 method의 필수 capability로 나열하지 않아야 합니다.

```cpp
TEST_F(PlayerTest, Play_WhenDolbyVisionUnavailable_PlaysWithoutDolbyVision) {
    EXPECT_CALL(capabilities_, isAvailable("xrn:firebolt:capability:hdr:dolbyvision"))
        .WillOnce(::testing::Return(false));

    EXPECT_CALL(transport_, send(JsonMethodIs("Player.play")))
        .WillOnce(::testing::Return(Json{{"result", Json{{"started", true}}}}));

    auto result = player_.play(ContentId{"movie-1"});

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().started);
}
```

### anyOf/allOf operator 테스트

OpenRPC capabilities extension은 여러 capability가 필요한 경우 `allOf`, `anyOf`, `oneOf` 같은 operator 의미를 가질 수 있습니다. 기본값은 `allOf`로 보는 것이 안전합니다.

```cpp
TEST_F(CapabilityResolverTest, AnyOf_WhenOneTransportAvailable_AllowsCall) {
    CapabilityRule rule = CapabilityRule::anyOf({
        "xrn:firebolt:capability:bluetooth:scan",
        "xrn:firebolt:capability:rf4ce:scan",
        "xrn:firebolt:capability:wifi:scan"
    });

    EXPECT_CALL(store_, isAvailable("xrn:firebolt:capability:wifi:scan"))
        .WillOnce(::testing::Return(true));

    EXPECT_TRUE(resolver_.canInvoke(rule));
}

TEST_F(CapabilityResolverTest, AllOf_WhenOneCapabilityMissing_BlocksCall) {
    CapabilityRule rule = CapabilityRule::allOf({
        "xrn:firebolt:capability:device:model",
        "xrn:firebolt:capability:localization:locale"
    });

    EXPECT_CALL(store_, isAvailable("xrn:firebolt:capability:device:model"))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(store_, isAvailable("xrn:firebolt:capability:localization:locale"))
        .WillOnce(::testing::Return(false));

    EXPECT_FALSE(resolver_.canInvoke(rule));
}
```

## 7. Event 테스트

이벤트는 subscribe 이후 callback이 정확히 호출되는지 검증합니다.

```cpp
TEST_F(LifecycleTest, OnForeground_DispatchesCallback) {
    EventHandler captured;

    EXPECT_CALL(transport_, subscribe("Lifecycle.onForeground", _))
        .WillOnce([&] (const std::string&, EventHandler handler) {
            captured = handler;
            return Subscription{1};
        });

    bool called = false;
    lifecycle_.onForeground([&] {
        called = true;
    });

    captured(Json{{"state", "foreground"}});

    EXPECT_TRUE(called);
}
```

unsubscribe 테스트도 같이 둡니다. 이벤트 기반 코드는 누락된 unsubscribe가 메모리 누수나 중복 callback으로 이어질 수 있습니다.

```cpp
TEST_F(LifecycleTest, Subscription_WhenDestroyed_Unsubscribes) {
    EXPECT_CALL(transport_, unsubscribe(Subscription{1}))
        .Times(1);

    {
        LifecycleSubscription subscription = lifecycle_.onForeground([] {});
        subscription.setId(Subscription{1});
    }
}
```

## 8. Error 테스트

| 오류 | 테스트 예 |
| --- | --- |
| invalid params | 잘못된 enum, 필수 필드 누락 |
| permission denied | capability grant 없음 |
| unavailable | device가 기능 미지원 |
| transport failure | timeout, connection closed |
| malformed response | result field 없음, 타입 불일치 |

```cpp
TEST_F(DeviceTest, Model_WhenTransportTimesOut_ReturnsTransportError) {
    EXPECT_CALL(transport_, send(_))
        .WillOnce(::testing::Throw(TransportTimeout{}));

    auto result = device_.model();

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error(), FireboltError::TransportTimeout);
}
```

```cpp
TEST_F(DeviceTest, Model_WhenResponseTypeIsWrong_ReturnsProtocolError) {
    EXPECT_CALL(transport_, send(JsonMethodIs("Device.model")))
        .WillOnce(::testing::Return(Json{{"result", 1234}}));

    auto result = device_.model();

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error(), FireboltError::ProtocolError);
}
```

## 9. L1/L2/L3 분리

| 레벨 | Firebolt 테스트 내용 |
| --- | --- |
| L1 | SDK wrapper, payload 생성, response parsing, capability decision |
| L2 | Firebolt runtime 또는 mock platform과의 contract |
| L3 | 실제 타겟에서 앱 lifecycle, permission prompt, device capability |

L1 테스트는 개발 PC에서 빠르게 실행되어야 합니다. L3 테스트는 타겟 상태, 권한 설정, 앱 설치 상태에 민감하므로 별도 레이블과 로그 수집이 필요합니다.

## 10. 테스트 이름 예시

```text
DeviceModel_WhenTransportReturnsString_ReturnsModel
DeviceModel_WhenCapabilityMissing_ReturnsUnavailable
Lifecycle_OnForegroundEvent_InvokesRegisteredHandler
Capabilities_RequestGrant_WhenUserDenies_ReturnsPermissionDenied
Keyboard_Show_WhenInvalidType_ReturnsInvalidParams
```

## 11. 실행 명령

```bash
./firebolt_tests --gtest_filter="*Capability*"
./firebolt_tests --gtest_filter="LifecycleTest.*"
./firebolt_tests --gtest_repeat=50 --gtest_break_on_failure
./firebolt_tests --gtest_output=xml:firebolt-test-results.xml
```

## 12. Schema contract 체크

Firebolt API는 OpenRPC schema가 기준이므로, hand-written wrapper를 쓰는 프로젝트라면 schema와 wrapper method 이름이 어긋나지 않는지 검사하는 테스트를 둘 수 있습니다.

```cpp
TEST_F(OpenRpcContractTest, DeviceModel_MethodExistsInSchema) {
    OpenRpcSchema schema = OpenRpcSchema::load("firebolt-openrpc.json");

    ASSERT_TRUE(schema.hasMethod("Device.model"));
    EXPECT_THAT(schema.method("Device.model").resultType(), ::testing::Eq("string"));
}
```

이 테스트는 실제 schema loader 구현이 있어야 의미가 있습니다. 핵심은 Firebolt API 이름과 capability 요구사항을 코드에 중복 하드코딩하지 않는 것입니다.

## 13. 참고 자료

- [Firebolt APIs](https://rdkcentral.github.io/firebolt/apis/latest/)
- [Firebolt Capabilities](https://rdkcentral.github.io/firebolt/requirements/latest/specifications/general/capabilities/)
- [Firebolt App Pass-through APIs](https://rdkcentral.github.io/firebolt/requirements/latest/specifications/openrpc-extensions/app-passthrough-apis/)
- [OpenRPC Specification](https://spec.open-rpc.org/)
