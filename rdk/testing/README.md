# RDK 학습 노트

이 디렉터리는 RDK-V 테스트를 이해하고 작성하기 위한 한국어 학습 노트입니다. 중복 문서는 합치고, 파일명은 소문자 hyphen 스타일로 정리했습니다.

## 읽는 순서

| 순서 | 문서 | 내용 |
| --- | --- | --- |
| 1 | [rdk-v-testing-guide.md](rdk-v-testing-guide.md) | RDK-V 구조, L1/L2/L3 전략, HAL/Thunder/IPC 테스트, CI 운영 |
| 2 | [gtest-gmock-cpp-guide.md](gtest-gmock-cpp-guide.md) | GTest/GMock 문법, matcher, callback capture, death test, CMake |
| 3 | [firebolt-testing-guide.md](firebolt-testing-guide.md) | Firebolt JSON-RPC, Capabilities, event, error 테스트 |

BitBake/Yocto 관련 내용은 [../bitbake/bitbake-guide.md](../bitbake/bitbake-guide.md)에 분리했습니다.

## 예제 코드

| 파일 | 내용 |
| --- | --- |
| [examples/mocking/MockExampleTest.cpp](examples/mocking/MockExampleTest.cpp) | GMock expectation, action, argument capture, custom matcher 예시 |
| [examples/advanced/MatcherTest.cpp](examples/advanced/MatcherTest.cpp) | 문자열/숫자/container/map matcher 예시 |

예제 코드는 개념 설명용입니다. 실제 프로젝트에 옮길 때는 production interface 이름, C++ 표준, GoogleTest/GMock 버전, build system 구성을 현재 RDK SDK에 맞춰 조정합니다.

## 빠른 기준표

| 테스트 대상 | 주로 검증할 것 | 권장 방식 |
| --- | --- | --- |
| 순수 C++ 로직 | 입력/출력, 오류 처리, 상태 전이 | GTest 단위 테스트 |
| 외부 의존성이 있는 서비스 | HAL, IPC, Thunder, 파일시스템 의존성 격리 | 인터페이스 + GMock |
| HAL wrapper | C API 계약, 에러 코드 매핑, 초기화/해제 순서 | adapter Mock |
| Thunder/RDK Services | JSON-RPC 요청/응답, 플러그인 생명주기, 보안 토큰 처리 | Mock transport 또는 타겟 통합 테스트 |
| Firebolt | OpenRPC 기반 API, Capabilities, grant 흐름, 이벤트 | SDK transport Mock + JSON-RPC contract test |
| 시스템 시나리오 | 앱 실행, 재생, 네트워크/HDMI/리모컨 이벤트 | 실제 타겟 또는 automation |

## 보강된 주요 주제

- RDK-V와 RDK-B/C의 적용 범위 차이
- HAL adapter와 C HAL 함수 포인터 wrapper
- Thunder/RDK Services plugin lifecycle 테스트
- Thunder 보안 토큰 실패 진단
- Firebolt `supported`, `available`, `permitted`, `granted` 구분
- Firebolt optional capability와 OpenRPC schema contract
- BitBake `ptest`, image 포함, task 디버깅 절차

## 파일명 정리 내역

| 이전 파일 | 새 위치 |
| --- | --- |
| `GUIDE_KO.md` | `rdk-v-testing-guide.md`에 병합 |
| `RDK_V_TESTING_COMPLETE_GUIDE.md` | `rdk-v-testing-guide.md`에 병합 |
| `RDK7_RDK8_TESTING_GUIDE.md` | `rdk-v-testing-guide.md`에 병합 |
| `rdk_grammer.md` | `gtest-gmock-cpp-guide.md`로 변경 |
| `FIREBOLT_TESTING_GUIDE.md` | `firebolt-testing-guide.md`로 변경 |

## 참고 자료

- [RDK-V RDK6 Architecture](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/rdk6/architecture/)
- [RDK-V RDK7 Architecture](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/rdk7/rdk7-architecture/)
- [RDK Services](https://developer.rdkcentral.com/documentation/documentation/rdk_video_documentation/sub-systems/rdkservices/)
- [Firebolt APIs](https://rdkcentral.github.io/firebolt/apis/latest/)
- [Firebolt Capabilities](https://rdkcentral.github.io/firebolt/requirements/latest/specifications/general/capabilities/)
- [GoogleTest User Guide](https://google.github.io/googletest/)
