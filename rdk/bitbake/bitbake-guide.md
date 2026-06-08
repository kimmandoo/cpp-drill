# BitBake 학습 노트

BitBake는 임베디드 Linux 배포판 자체가 아니라, recipe와 configuration metadata를 읽어 task graph를 만들고 실행하는 빌드 엔진입니다. Yocto Project와 OpenEmbedded에서 핵심 빌드 실행기로 사용됩니다.

## 1. 관계 정리

| 이름 | 의미 |
| --- | --- |
| BitBake | `.bb`, `.bbappend`, `.bbclass`, `.conf` metadata를 해석하고 task를 실행하는 엔진 |
| OpenEmbedded | BitBake가 읽는 recipe/class/layer metadata 생태계 |
| Poky | Yocto Project의 reference distribution |
| Yocto Project | 임베디드 Linux image를 재현 가능하게 만들기 위한 프로젝트, 문서, 도구, reference metadata 묶음 |

BitBake는 컴파일 명령만 실행하는 도구가 아니라 source fetch, patch, configure, compile, install, package, image 생성까지 metadata 기반 task로 관리합니다.

## 2. 빌드 흐름

```text
configuration 읽기
  -> layer 탐색
  -> recipe(.bb)와 append(.bbappend) 파싱
  -> provider/version/dependency 결정
  -> task graph 생성
  -> do_fetch, do_unpack, do_patch, do_configure, do_compile, do_install ...
  -> package/image 생성
  -> tmp/deploy/images/<machine>/ 등에 산출물 배치
```

완료된 task는 stamp와 signature를 이용해 다시 실행할 필요가 있는지 판단합니다. sstate cache와 setscene task는 이전 빌드 산출물을 재사용해 빌드 시간을 줄입니다.

## 3. 주요 파일

| 파일 | 역할 |
| --- | --- |
| `.bb` | 하나의 소프트웨어 컴포넌트를 가져오고 빌드/설치/패키징하는 recipe |
| `.bbappend` | 기존 recipe에 변경을 덧붙이는 파일. BSP와 제품 커스터마이징에서 중요 |
| `.bbclass` | 여러 recipe가 공유하는 공통 로직. 예: `cmake`, `autotools`, `meson`, `systemd` |
| `.conf` | 빌드 설정. 예: `local.conf`, `bblayers.conf`, `machine/*.conf`, `distro/*.conf` |
| `layer.conf` | layer가 제공하는 recipe/class와 우선순위 등을 BitBake에 알림 |
| `local.conf` | 사용자의 로컬 빌드 설정. 예: `MACHINE`, 병렬 빌드, 추가 package |
| `bblayers.conf` | 빌드에 포함할 layer 목록 |

## 4. 간단한 recipe

```bitbake
SUMMARY = "Simple helloworld application"
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://helloworld.c"

S = "${UNPACKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} helloworld.c -o helloworld
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 helloworld ${D}${bindir}/helloworld
}
```

`SRC_URI`는 source, patch, local file 위치를 지정합니다. Git 같은 SCM을 쓰면 재현 가능한 빌드를 위해 `SRCREV`를 고정하는 것이 일반적입니다.

## 5. 자주 쓰는 변수

| 변수 | 의미 |
| --- | --- |
| `PN` | recipe/package 이름. 보통 파일명에서 유도됨 |
| `PV` | version. 예: `helloworld_1.0.bb`라면 보통 `1.0` |
| `PR` | recipe revision. 최신 Yocto에서는 signature/sstate가 중요해 직접 자주 올리지는 않음 |
| `SRC_URI` | source, patch, local file 위치 |
| `SRCREV` | Git 등 SCM에서 사용할 정확한 revision |
| `S` | source directory |
| `B` | build directory |
| `D` | `do_install`이 설치할 임시 destination root |
| `DEPENDS` | build-time recipe dependency |
| `RDEPENDS:${PN}` | runtime package dependency |
| `FILES:${PN}` | 어떤 파일을 어떤 package에 넣을지 지정 |
| `PACKAGES` | recipe가 생성하는 package 목록 |
| `IMAGE_INSTALL` | image에 포함할 package 목록 |
| `MACHINE` | target board/machine |
| `DISTRO` | distribution policy |
| `BBLAYERS` | 포함할 layer 목록 |

`DEPENDS`는 빌드 시 필요한 recipe dependency이고, `RDEPENDS:${PN}`은 생성된 package가 실행 시 필요로 하는 runtime dependency입니다.

## 6. 변수 문법

```bitbake
A = "value"          # 기본 할당. lazy expansion
A ?= "default"      # A가 아직 없으면 할당
A ??= "weak"        # 더 약한 기본값
A := "${B}"         # 즉시 확장
A += " item"        # 공백을 넣고 append
A .= "suffix"       # 공백 없이 append
A:append = " x"     # override style append. 공백은 직접 넣어야 함
A:prepend = "x "
A:remove = "x"
```

최신 BitBake override style은 `:append`, `:prepend`, `:remove` 형식을 사용합니다. 오래된 문서나 legacy layer에서는 `_append`, `_prepend`, `_remove`를 볼 수 있지만, Honister 이후 문서에서는 colon syntax가 기준입니다.

```bitbake
FOO = "a"
FOO:append = " b"
# 결과: "a b"

BAR = "a"
BAR:append = "b"
# 결과: "ab"
```

override style operation은 공백을 자동으로 넣지 않으므로 직접 제어해야 합니다. `:append`, `:prepend`, `:remove`는 expansion 시점에 적용되며, 공식 문서 기준 적용 순서는 append, prepend, remove입니다.

## 7. Task

BitBake 실행 단위는 task입니다. 관례상 task 이름은 `do_`로 시작합니다.

```bitbake
do_fetch
do_unpack
do_patch
do_configure
do_compile
do_install
do_package
do_package_write_*
do_populate_sysroot
do_rootfs
do_image
do_deploy
```

task는 shell 함수 또는 BitBake style Python 함수로 작성할 수 있습니다.

```bitbake
python do_printdate() {
    import datetime
    bb.plain("Date: %s" % datetime.date.today())
}

addtask printdate after do_fetch before do_build
```

`addtask printdate`는 `do_printdate` task를 graph에 추가합니다. task 함수 이름에는 `do_`가 붙지만, `addtask`에는 `do_`를 뺀 이름을 씁니다.

## 8. 자주 쓰는 명령

```bash
bitbake core-image-minimal
bitbake helloworld -c listtasks
bitbake helloworld -c compile
bitbake helloworld -c compile -f
bitbake helloworld -C compile
bitbake -e helloworld | less
bitbake -s
bitbake -g core-image-minimal
bitbake -n core-image-minimal
bitbake -p
bitbake -k core-image-minimal
bitbake -S printdiff helloworld
```

| 명령 | 의미 |
| --- | --- |
| `bitbake <target>` | target의 기본 task, 보통 `do_build` 실행 |
| `-c <task>` | 특정 task 실행 |
| `-f` | 지정 task 강제 실행 |
| `-C <task>` | 해당 task부터 다시 빌드되도록 invalidation |
| `-e` | 최종 변수 값과 history 확인 |
| `-s` | 사용 가능한 recipe와 version 확인 |
| `-g` | dependency graph 파일 생성 |
| `-n` | dry-run |
| `-p` | parse만 수행 |
| `-k` | 오류가 나도 가능한 task 계속 진행 |
| `-S printdiff` | signature 차이 추적 |

## 9. Layer 구조

```text
meta-myproduct/
  conf/
    layer.conf
  recipes-apps/
    helloworld/
      helloworld_1.0.bb
      files/
        helloworld.c
  recipes-kernel/
  recipes-bsp/
  classes/
```

제품 변경은 가능하면 upstream recipe를 복사하지 말고 별도 layer의 `.bbappend`로 최소 변경만 추가합니다.

## 10. bbappend 예시

기존 recipe에 patch와 compile definition을 추가하는 예시입니다.

```bitbake
# recipes-rdk/rdkservice/rdkservice_%.bbappend

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://fix-jsonrpc-timeout.patch"

EXTRA_OECMAKE:append = " -DENABLE_RDK_TEST_HOOKS=ON"
```

`FILESEXTRAPATHS`는 append 파일 옆의 `files/` 디렉터리를 검색 경로에 넣기 위해 자주 사용합니다.

## 11. RDK 테스트 recipe 예시

RDK 테스트 binary를 target image에 설치하는 단순 예시입니다.

```bitbake
SUMMARY = "RDK component test binaries"
LICENSE = "CLOSED"

SRC_URI = "git://example.com/rdk-tests.git;protocol=https;branch=main"
SRCREV = "0123456789abcdef0123456789abcdef01234567"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS += "googletest nlohmann-json"
RDEPENDS:${PN} += "bash"

EXTRA_OECMAKE += "\
    -DRDK_ENABLE_L1_TESTS=ON \
    -DRDK_ENABLE_TARGET_TESTS=ON \
"

do_install:append() {
    install -d ${D}${bindir}/rdk-tests
    install -m 0755 ${B}/rdk_l1_tests ${D}${bindir}/rdk-tests/rdk_l1_tests
    install -m 0755 ${B}/rdk_l3_tests ${D}${bindir}/rdk-tests/rdk_l3_tests
}

FILES:${PN} += "${bindir}/rdk-tests"
```

실제 프로젝트에서는 `LICENSE`, dependency, install path, service log 위치를 제품 정책에 맞춰 조정해야 합니다.

### image에 테스트 패키지 넣기

테스트 binary recipe를 만들었다고 해서 자동으로 image에 포함되지는 않습니다. 제품 image나 local 개발 image에 package를 추가해야 합니다.

```bitbake
# conf/local.conf 또는 image recipe
IMAGE_INSTALL:append = " rdk-tests"
```

개발용 image와 release image를 분리하고, release image에는 L3 테스트 binary나 test hook이 들어가지 않도록 확인합니다.

## 12. ptest로 테스트 패키징

Yocto/OpenEmbedded에서는 package test를 위해 `ptest`를 사용할 수 있습니다. 공식 Yocto test manual 기준 ptest는 OpenEmbedded build system이 만든 package에 대한 테스트를 target machine에서 실행하는 방식입니다. RDK L1 테스트 binary를 target에서 실행해야 한다면 ptest 패키징을 고려할 수 있습니다.

```bitbake
inherit cmake ptest

do_install_ptest() {
    install -d ${D}${PTEST_PATH}
    install -m 0755 ${B}/rdk_l1_tests ${D}${PTEST_PATH}/rdk_l1_tests
    install -m 0755 ${S}/tests/run-ptest ${D}${PTEST_PATH}/run-ptest
}
```

```bash
#!/bin/sh
./rdk_l1_tests --gtest_output=xml:rdk-l1-results.xml
```

ptest package를 image에 넣으려면 보통 ptest 관련 image feature/package 구성이 필요합니다.

```bitbake
IMAGE_FEATURES:append = " ptest-pkgs"
IMAGE_INSTALL:append = " ptest-runner"
```

타겟에서는 다음처럼 실행합니다.

```bash
ptest-runner rdk-tests
```

L3처럼 실제 daemon, permission, hardware state가 필요한 테스트는 ptest보다 별도 target automation으로 분리하는 편이 안정적일 수 있습니다.

## 13. BitBake 디버깅 절차

빌드가 실패했을 때는 감으로 recipe를 고치기보다 task와 변수 값을 먼저 확인합니다.

```bash
bitbake rdk-tests -c listtasks
bitbake rdk-tests -e | less
bitbake rdk-tests -c configure -f
bitbake rdk-tests -c compile -f
bitbake rdk-tests -c devshell
```

| 상황 | 볼 것 |
| --- | --- |
| source가 안 받아짐 | `SRC_URI`, `SRCREV`, network/protocol 설정 |
| patch가 안 붙음 | `FILESEXTRAPATHS`, patch path, `do_patch` log |
| header를 못 찾음 | `DEPENDS`, sysroot, `CFLAGS`, CMake toolchain |
| runtime에서 library 없음 | `RDEPENDS:${PN}`, package split, `FILES:${PN}` |
| task가 계속 재실행됨 | `bitbake -S printdiff <recipe>` |

## 14. RDK와 연결해서 볼 점

| 테스트 | BitBake 통합 기준 |
| --- | --- |
| PC/빌드 호스트에서 가능한 L1 | 빌드 후 별도 CI 단계에서 실행 가능 |
| sysroot가 필요한 L2 | recipe dependency와 test package를 명확히 선언 |
| 타겟 서비스가 필요한 L3 | image에 test binary를 포함하고 타겟에서 실행 |

타겟 daemon, Thunder security token, 실제 HAL이 필요한 테스트를 빌드 호스트에서 실행하려고 하면 flaky하거나 실패 원인이 불분명해집니다.

## 15. 참고 자료

- [BitBake User Manual - Introduction](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-intro.html)
- [BitBake User Manual - Syntax and Operators](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-metadata.html)
- [Yocto Project Test Manual - ptest](https://docs.yoctoproject.org/5.1.2/test-manual/ptest.html)
- [Yocto Project Reference Manual - Variables](https://docs.yoctoproject.org/ref-manual/variables.html)
