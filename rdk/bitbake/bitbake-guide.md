# BitBake 학습 노트

BitBake는 임베디드 Linux 배포판 자체가 아니라, recipe와 configuration metadata를 읽어 task graph를 만들고 실행하는 빌드 엔진입니다. Yocto Project와 OpenEmbedded에서 핵심 빌드 실행기로 사용됩니다.

## 1. 실전: BitBake 로그 읽기

BitBake를 처음 접하면 로그가 가장 먼저 눈에 들어옵니다. 아래 로그 한 줄을 분해해보겠습니다.

```
recipe lib32-libnsl2-2.0.0-r0: task do_install: Started
```

### 로그 포맷

```
recipe <PN>-<PV>-r<PR>: task <task_name>: <status>
```

| 부분 | 값 | 의미 |
| --- | --- | --- |
| `PN` | `lib32-libnsl2` | Package Name. recipe 파일명에서 유도됨 |
| `PV` | `2.0.0` | Package Version |
| `PR` | `r0` | Package Revision. recipe가 수정될 때 올라감 |
| task | `do_install` | 실행 중인 task 이름 |
| status | `Started` | task 상태 (`Started` / `Succeeded` / `Failed`) |

`lib32-` 접두사는 **multilib** 빌드임을 나타냅니다. 64-bit target에서 32-bit library를 별도로 빌드할 때 붙습니다. 같은 recipe를 architecture variant별로 여러 번 실행한다고 보면 됩니다.

### Task가 의미하는 것

`do_install`이 실행 중이라는 건, 그 앞의 task들이 모두 성공했다는 뜻입니다. 표준 task 진행 순서:

```
do_fetch      → 소스코드 다운로드 (git clone, wget, http fetch, file copy)
do_unpack     → 압축 해제 (tar, zip, git checkout)
do_patch      → patch 파일 적용
do_configure  → configure / cmake / meson 실행
do_compile    → make / cmake --build / ninja 실행
do_install    → make install 또는 수동 install 명령으로 D에 파일 복사
do_package    → D의 파일을 package별로 분할
do_package_write_* → package를 ipk/deb/rpm으로 출력
```

실패하면 해당 task에서 멈추고 빨간색 `Failed` 로그가 남습니다. 예: `do_compile`에서 실패하면 `do_install`은 아예 시작하지 않습니다.

### 빌드가 실제로 일어나는 디렉터리

BitBake는 `TMPDIR` 아래에 recipe별로 작업 디렉터리를 만듭니다.

```
TMPDIR (보통 build/tmp/)
  work/
    <arch>-<os>/
      <pn>-<pv>-<pr>/       ← WORKDIR
        source/              ← S (소스 코드)
        build/               ← B (빌드 산출물)
        image/               ← D (do_install 설치 대상)
        packages-split/      ← 패키징된 결과
        temp/                ← log.do_*, run.do_* (실제 실행된 명령)
  deploy/
    packages/                ← 생성된 ipk/deb/rpm
    images/<machine>/        ← 최종 bootable image
```

`temp/log.do_<task>` 파일에 실제 실행된 shell 명령과 출력이 모두 기록됩니다. 빌드 실패 시 이 로그를 가장 먼저 봅니다.

### 여러 recipe가 동시에 실행될 때

BitBake는 dependency가 없는 recipe들을 병렬로 실행합니다. 따라서 로그가 여러 recipe에서 동시에 나오며 `Started`와 `Succeeded`가 섞여서 출력됩니다. 특정 recipe의 진행 상황만 보고 싶다면 `bitbake <recipe> -c compile` 식으로 단일 recipe의 특정 task만 실행할 수 있습니다.


## 2. 관계 정리

| 이름 | 의미 |
| --- | --- |
| BitBake | `.bb`, `.bbappend`, `.bbclass`, `.conf` metadata를 해석하고 task를 실행하는 엔진 |
| OpenEmbedded | BitBake가 읽는 recipe/class/layer metadata 생태계 |
| Poky | Yocto Project의 reference distribution |
| Yocto Project | 임베디드 Linux image를 재현 가능하게 만들기 위한 프로젝트, 문서, 도구, reference metadata 묶음 |

BitBake는 컴파일 명령만 실행하는 도구가 아니라 source fetch, patch, configure, compile, install, package, image 생성까지 metadata 기반 task로 관리합니다.

### 각 컴포넌트의 관계

BitBake는 혼자서는 아무것도 빌드하지 못합니다. recipe가 있어야 동작합니다.

```
Yocto Project
  └── Poky (reference distro + BSP + recipes)
        ├── BitBake (빌드 엔진)
        └── OpenEmbedded-Core (기본 recipe/class 모음)
              └── meta-openembedded (추가 recipe layer)
```

실제 제품 빌드에서는 Poky + vendor BSP layer + 제품 layer를 조합합니다. BitBake가 이 모든 layer의 recipe를 읽어 하나의 task graph로 만듭니다.

### layer가 겹칠 때

같은 recipe(예: `linux-yocto_6.6.bb`)가 여러 layer에 존재하면 `BBFILE_PRIORITY`가 높은 layer의 recipe가 선택됩니다. 특정 recipe를 강제로 지정하려면 `local.conf`에:

```bitbake
PREFERRED_PROVIDER_virtual/kernel = "linux-yocto"
PREFERRED_VERSION_linux-yocto = "6.6%"
```

## 3. 빌드 흐름

BitBake는 한 번에 모든 recipe를 빌드하지 않습니다. 다음 순서로 진행됩니다:

### 3.1 Parsing 단계

1. `bblayers.conf`에 나열된 모든 layer의 `layer.conf`를 읽음
2. 각 layer의 `BBFILES` glob에 매칭되는 `.bb`, `.bbappend` 파일을 모두 찾음
3. 모든 recipe를 파싱해서 변수와 task 정의를 수집
4. `PREFERRED_PROVIDER`, `PREFERRED_VERSION` 등으로 어떤 recipe가 최종 provider인지 결정

### 3.2 Task graph 생성

1. 빌드 target(예: `core-image-minimal`)에서 시작해서 `DEPENDS`, `RDEPENDS`를 따라 의존성 그래프를 만듦
2. 각 recipe의 task 간 순서(`addtask`로 정의된 `after`/`before` 관계)를 그래프에 반영
3. inter-recipe dependency도 task 단위로 연결됨: `do_configure[depends] += "other-recipe:do_populate_sysroot"`

### 3.3 실행 단계

1. 그래프에서 실행 가능한(모든 의존성이 충족된) task를 병렬로 실행
2. 각 task 실행 전에 stamp를 확인 — 이미 완료된 task는 건너뜀
3. sstate cache에 matching artifact가 있으면 `do_fetch`~`do_install`을 통째로 건너뛰고 `do_populate_sysroot`만 setscene으로 복원

### 3.4 산출물

```
tmp/deploy/
  ipk/ (또는 deb/rpm)   ← 개별 package
  images/<machine>/      ← 최종 rootfs image, kernel, dtb
  licenses/              ← 라이선스 manifest
```

### 3.5 sstate cache 상세

sstate(Shared State) cache는 task의 입력(signature)을 hash로 만들어, 동일한 입력에 대해 이전 빌드 결과를 재사용합니다. cache는 기본적으로 `tmp/sstate-cache/`에 저장되며, 팀 단위로 공유하려면 `SSTATE_MIRRORS`를 HTTP 서버로 설정합니다.

```bitbake
# local.conf
SSTATE_MIRRORS = "file://.* http://build-cache.internal/sstate/PATH"
```

cache hit 시 로그에 `Sstate summary: Wanted X Found Y Missed Z`가 출력됩니다. `Found`가 높을수록 빌드가 빠릅니다.

## 4. 주요 파일

| 파일 | 역할 |
| --- | --- |
| `.bb` | 하나의 소프트웨어 컴포넌트를 가져오고 빌드/설치/패키징하는 recipe |
| `.bbappend` | 기존 recipe에 변경을 덧붙이는 파일. BSP와 제품 커스터마이징에서 중요 |
| `.bbclass` | 여러 recipe가 공유하는 공통 로직. 예: `cmake`, `autotools`, `meson`, `systemd` |
| `.conf` | 빌드 설정. 예: `local.conf`, `bblayers.conf`, `machine/*.conf`, `distro/*.conf` |
| `layer.conf` | layer가 제공하는 recipe/class와 우선순위 등을 BitBake에 알림 |
| `local.conf` | 사용자의 로컬 빌드 설정. 예: `MACHINE`, 병렬 빌드, 추가 package |
| `bblayers.conf` | 빌드에 포함할 layer 목록 |

### 파일명 규칙

Recipe 파일명은 `<name>_<version>.bb` 형식입니다. BitBake는 `_`를 기준으로 `PN`과 `PV`를 자동 추출합니다.

| 파일명 | PN | PV |
| --- | --- | --- |
| `helloworld_1.0.bb` | `helloworld` | `1.0` |
| `libnsl2_2.0.0.bb` | `libnsl2` | `2.0.0` |
| `linux-yocto_6.6.bb` | `linux-yocto` | `6.6` |
| `git_2.39.3.bb` | `git` | `2.39.3` |

`git`처럼 PN이 예약어와 겹치는 경우 `BPN`(bare PN)과 `PN`이 다를 수 있습니다. `lib32-libnsl2`처럼 multilib prefix가 붙으면 `PN = lib32-libnsl2`이지만 `BPN = libnsl2`입니다.

### .bbappend 매칭 규칙

`.bbappend` 파일명은 대상 recipe와 정확히 일치하거나 `%` wildcard를 쓸 수 있습니다:

| bbappend | 매칭되는 recipe |
| --- | --- |
| `helloworld_1.0.bbappend` | `helloworld_1.0.bb`만 |
| `helloworld_%.bbappend` | 모든 버전의 `helloworld_*.bb` |
| `helloworld_1.%.bbappend` | `helloworld_1.0.bb`, `helloworld_1.1.bb` 등 |
## 5. 간단한 recipe

아래는 C 소스 하나를 컴파일하는 가장 간단한 recipe입니다. 각 줄이 무엇을 의미하는지 해설합니다.

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

### 줄별 해설

| 줄 | 설명 |
| --- | --- |
| `SUMMARY` | 패키지 설명. `bitbake -s`나 package feed에 표시됨 |
| `SECTION` | 패키지 분류 카테고리 (예: `base`, `libs`, `devel`) |
| `LICENSE` | 라이선스 이름. `MIT`, `GPL-2.0-only`, `CLOSED` 등 SPDX identifier 사용 |
| `LIC_FILES_CHKSUM` | 라이선스 파일의 checksum. **필수 필드** — 없으면 빌드가 `do_populate_lic`에서 실패함. `md5`는 실제 라이선스 텍스트 파일의 MD5 hash |
| `SRC_URI` | 소스 위치. `file://`은 recipe 옆 `files/` 디렉터리에서 가져옴. `git://`, `https://`, `ftp://`도 가능 |
| `S` | 소스가 unpack된 디렉터리. `${UNPACKDIR}`은 `do_unpack`이 파일을 푼 위치 |
| `do_compile` | 컴파일 명령. `${CC}`는 cross-compiler, `${CFLAGS}`는 target에 맞는 flag로 자동 설정됨 |
| `do_install` | 설치 명령. `${D}`는 임시 root (`image/`), `${bindir}`는 `/usr/bin` |

### SRC_URI scheme별 사용법

| scheme | 예시 | 설명 |
| --- | --- | --- |
| `file://` | `file://helloworld.c` | recipe 옆 `files/` 디렉터리의 파일 |
| `git://` | `git://github.com/foo/bar.git;protocol=https;branch=main` | Git 저장소. `SRCREV`로 commit 고정 필수 |
| `https://` | `https://example.com/foo-1.0.tar.gz` | tarball 다운로드 |
| `ftp://` | `ftp://ftp.gnu.org/gnu/foo/foo-1.0.tar.gz` | FTP 다운로드 |

Git 사용 시 `SRCREV`는 반드시 고정해야 재현 가능한 빌드가 됩니다:

```bitbake
SRC_URI = "git://github.com/foo/bar.git;protocol=https;branch=main"
SRCREV = "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0"
```

`SRCREV = "${AUTOREV}"`로 설정하면 항상 최신 commit을 가져오지만, 재현 가능성이 깨지므로 release 빌드에서는 사용하지 않습니다.

## 6. 자주 쓰는 변수

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

### DEPENDS vs RDEPENDS

```bitbake
# 빌드할 때 libfoo의 header/library가 필요함
DEPENDS += "libfoo"

# 실행할 때 libfoo.so가 필요함 (자동으로 추론되지만 명시적 선언이 안전)
RDEPENDS:${PN} += "libfoo"
```

`DEPENDS`에 추가된 recipe는 `do_configure` 전에 `do_populate_sysroot`가 완료됩니다. 즉, header와 library가 sysroot에 준비된 상태에서 내 recipe의 configure/compile이 실행됩니다.

### package split 기본값

BitBake는 `do_install`로 `${D}`에 설치된 파일을 보고 자동으로 package를 나눕니다. 기본 split 규칙:

| package | 포함되는 파일 |
| --- | --- |
| `${PN}` (main) | `${bindir}/*`, `${libdir}/lib*.so.*` 등 실행/런타임 파일 |
| `${PN}-dev` | `${includedir}/*`, `${libdir}/lib*.so` (symlink), `.pc` 파일 |
| `${PN}-dbg` | 디버그 심볼 (`/usr/src/debug/`) |
| `${PN}-doc` | `${mandir}/*`, `${infodir}/*`, `${docdir}/*` |
| `${PN}-staticdev` | `${libdir}/lib*.a` |

특정 파일을 main package에 강제로 넣으려면:

```bitbake
FILES:${PN} += "${bindir}/my-tool ${libdir}/libcustom.so"
```
## 7. 변수 문법

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

## 8. Task

BitBake 실행 단위는 task입니다. 관례상 task 이름은 `do_`로 시작합니다.

### 표준 task 목록

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

### 기본 task 실행 순서

각 recipe의 표준 task는 `addtask`로 순서가 정의되어 있습니다. `base.bbclass`에서 정의된 기본 체인:

```
do_fetch → do_unpack → do_patch → do_configure → do_compile → do_install → do_package → do_package_write_*
                                                                                    ↘ do_populate_sysroot
```

`do_populate_sysroot`는 `do_install` 이후 실행되며, `${D}`의 파일 중 다른 recipe 빌드에 필요한 것(header, library, pkg-config)을 sysroot로 복사합니다.

### Task flag

task에는 flag를 붙여 동작을 제어할 수 있습니다.

| flag | 의미 | 예시 |
| --- | --- | --- |
| `[depends]` | 특정 recipe의 특정 task가 먼저 실행되어야 함 | `do_configure[depends] += "libfoo:do_populate_sysroot"` |
| `[deptask]` | DEPENDS로 연결된 모든 recipe의 특정 task가 먼저 실행 | `do_configure[deptask] = "do_populate_sysroot"` |
| `[rdeptask]` | RDEPENDS로 연결된 모든 recipe의 특정 task가 먼저 실행 | `do_build[rdeptask] = "do_package_write_ipk"` |
| `[recrdeptask]` | RDEPENDS 체인 전체(재귀)의 특정 task | `do_rootfs[recrdeptask] += "do_package_write_ipk"` |
| `[noexec]` | task를 실행하지 않고 "executed"로 표시만 함 | `do_fetch[noexec] = "1"` |
| `[nostamp]` | stamp를 생성하지 않아 항상 재실행 | `do_compile[nostamp] = "1"` |
| `[dirs]` | task 실행 전에 생성할 디렉터리 | `do_compile[dirs] = "${B}"` |
| `[cleandirs]` | task 실행 전에 비우고 생성할 디렉터리 | `do_configure[cleandirs] = "${B}"` |

### inter-recipe dependency 예시

```bitbake
# my-app.bb
DEPENDS += "libfoo"

# libfoo의 do_populate_sysroot가 완료된 후에 my-app의 do_configure 실행
# (DEPENDS만 선언하면 BitBake가 자동으로 이 관계를 만듦)
```

만약 `do_compile` 전에 libfoo의 특정 custom task가 필요하다면:

```bitbake
do_compile[depends] += "libfoo:do_generate_headers"
```

### task 확인 명령

```bash
# 특정 recipe의 task 목록과 의존성 확인
bitbake helloworld -c listtasks

# task 간 의존성 그래프를 dot 파일로 출력
bitbake -g core-image-minimal
# → task-depends.dot, pn-depends.dot 생성됨
```

### task 함수 작성

task는 shell 함수 또는 BitBake style Python 함수로 작성할 수 있습니다.

```bitbake
python do_printdate() {
    import datetime
    bb.plain("Date: %s" % datetime.date.today())
}

addtask printdate after do_fetch before do_build
```

`addtask printdate`는 `do_printdate` task를 graph에 추가합니다. task 함수 이름에는 `do_`가 붙지만, `addtask`에는 `do_`를 뺀 이름을 씁니다.

## 9. 자주 쓰는 명령

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

## 10. Layer 구조

### 기본 디렉터리 구조

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

### layer.conf 필수 내용

```bitbake
# meta-myproduct/conf/layer.conf

# layer 경로를 LAYERDIR 변수에 추가
BBPATH .= ":${LAYERDIR}"

# 이 layer에서 제공하는 recipe/bbappend 검색 경로
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

# layer collection 이름 (고유해야 함)
BBFILE_COLLECTIONS += "myproduct"

# 이 collection의 우선순위 (높을수록 우선)
BBFILE_PRIORITY_myproduct = "6"

# 다른 layer에 의존하는 경우
LAYERDEPENDS_myproduct = "core openembedded-layer"

# layer version (선택)
LAYERVERSION_myproduct = "1"
```

### layer priority 동작 방식

같은 recipe가 여러 layer에 있을 때 `BBFILE_PRIORITY`가 높은 layer의 recipe가 선택됩니다. Poky 기본 layer들의 우선순위:

| layer | priority |
| --- | --- |
| `meta` (oe-core) | 5 |
| `meta-poky` | 6 |
| `meta-yocto-bsp` | 5 |
| vendor BSP | 보통 6~8 |
| 제품 layer | 보통 7~10 |

### recipe 검색 경로 (BBFILES)

`BBFILES`는 glob 패턴으로 recipe 파일을 찾습니다. 관례상 `recipes-<category>/<recipe-name>/<recipe>_<version>.bb` 구조를 사용합니다. `bitbake-layers show-recipes`로 현재 빌드에 포함된 모든 recipe를 볼 수 있습니다.

### layer 관리 명령

```bash
# 현재 활성화된 layer 목록
bitbake-layers show-layers

# 특정 recipe가 어느 layer에서 왔는지
bitbake-layers show-recipes | grep helloworld

# layer에 포함된 모든 recipe의 의존성 검사
bitbake-layers check-layer-deps meta-myproduct
```

제품 변경은 가능하면 upstream recipe를 복사하지 말고 별도 layer의 `.bbappend`로 최소 변경만 추가합니다.

## 11. bbappend 예시

기존 recipe에 patch와 compile definition을 추가하는 예시입니다.

```bitbake
# recipes-rdk/rdkservice/rdkservice_%.bbappend

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://fix-jsonrpc-timeout.patch"

EXTRA_OECMAKE:append = " -DENABLE_RDK_TEST_HOOKS=ON"
```

`FILESEXTRAPATHS`는 append 파일 옆의 `files/` 디렉터리를 검색 경로에 넣기 위해 자주 사용합니다.

## 12. RDK 테스트 recipe 예시

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

## 13. ptest로 테스트 패키징

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

## 14. BitBake 디버깅 절차

빌드가 실패했을 때는 감으로 recipe를 고치기보다 task와 변수 값을 먼저 확인합니다.

### 14.1 실패 로그 찾기

BitBake가 실패하면 마지막에 오류 요약을 출력합니다. 하지만 실제 원인은 task 로그 파일에 있습니다.

```bash
# 실패한 recipe의 WORKDIR로 이동
cd tmp/work/<arch>-<os>/<pn>-<pv>-<pr>/temp

# 실패한 task의 로그 확인
cat log.do_compile

# 또는 오류만 grep
grep -i error log.do_compile
```

로그 파일 위치는 BitBake 출력에도 나옵니다:
```
ERROR: helloworld-1.0-r0 do_compile: Execution of '...' failed
Log data follows:
| ...error output...
```

### 14.2 기본 디버깅 명령

```bash
# task 목록 확인
bitbake rdk-tests -c listtasks

# 최종 변수 값 확인 (매우 김, less로 열 것)
bitbake rdk-tests -e | less

# 특정 변수만 확인
bitbake rdk-tests -e | grep "^SRC_URI="

# configure만 강제 재실행
bitbake rdk-tests -c configure -f

# compile만 강제 재실행
bitbake rdk-tests -c compile -f

# devshell: recipe의 WORKDIR에서 interactive shell
bitbake rdk-tests -c devshell
```

### 14.3 devshell 활용

`devshell`은 recipe의 빌드 환경으로 들어가는 interactive shell입니다. `$CC`, `$CFLAGS`, `$LDFLAGS` 등이 모두 설정된 상태라서, 컴파일 명령을 직접 실행해보며 디버깅할 수 있습니다.

```bash
bitbake rdk-tests -c devshell
# 이제 WORKDIR 안에서:
echo $CC          # cross-compiler 경로
echo $CFLAGS      # target에 맞는 flag
make VERBOSE=1    # 상세 빌드 출력으로 확인
```

### 14.4 clean과 rebuild

```bash
# 특정 task만 clean (stamp 제거)
bitbake rdk-tests -c cleansstate

# recipe 전체 clean (WORKDIR + sstate 제거)
bitbake rdk-tests -c cleanall

# 의존성까지 모두 clean하고 다시 빌드
bitbake rdk-tests -c cleanall && bitbake rdk-tests
```

`-c clean`은 WORKDIR만 지우고 sstate는 남깁니다. `-c cleansstate`는 sstate cache까지 지워서 다음 빌드 시 모든 task가 재실행됩니다.

### 14.5 상황별 디버깅

| 상황 | 볼 것 |
| --- | --- |
| source가 안 받아짐 | `SRC_URI`, `SRCREV`, network/protocol 설정, `log.do_fetch` |
| patch가 안 붙음 | `FILESEXTRAPATHS`, patch path, `log.do_patch` |
| header를 못 찾음 | `DEPENDS`, sysroot 경로, `CFLAGS`, CMake toolchain file |
| link 에러 | `LDFLAGS`, `DEPENDS` 누락, library search path |
| runtime에서 library 없음 | `RDEPENDS:${PN}`, package split, `FILES:${PN}` |
| task가 계속 재실행됨 | `bitbake -S printdiff <recipe>` |
| recipe parse 에러 | `bitbake -p` (parse only) |

### 14.6 -e 출력 읽는 법

`bitbake -e`는 모든 변수의 최종 값과 override history를 출력합니다. 출력 포맷:

```
# $SRC_URI [2 operations]
#   set conf/documentation.conf:389
#     [_doc] "git://example.com/foo.git;protocol=https;branch=main"
#   set meta-myproduct/recipes-apps/foo/foo_1.0.bb:12
#     "git://example.com/foo.git;protocol=https;branch=main file://fix.patch"
# pre-expansion value:
#   "git://example.com/foo.git;protocol=https;branch=main file://fix.patch"
SRC_URI="git://example.com/foo.git;protocol=https;branch=main file://fix.patch"
```

- `# $SRC_URI [2 operations]` — 이 변수에 2번의 할당이 있었음
- `set <file>:<line>` — 각 할당의 출처
- `pre-expansion value` — `${}` 확장 전 값
- 마지막 줄 — 최종 값

## 15. RDK와 연결해서 볼 점

| 테스트 | BitBake 통합 기준 |
| --- | --- |
| PC/빌드 호스트에서 가능한 L1 | 빌드 후 별도 CI 단계에서 실행 가능 |
| sysroot가 필요한 L2 | recipe dependency와 test package를 명확히 선언 |
| 타겟 서비스가 필요한 L3 | image에 test binary를 포함하고 타겟에서 실행 |

타겟 daemon, Thunder security token, 실제 HAL이 필요한 테스트를 빌드 호스트에서 실행하려고 하면 flaky하거나 실패 원인이 불분명해집니다.

## 16. 참고 자료

- [BitBake User Manual - Introduction](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-intro.html)
- [BitBake User Manual - Syntax and Operators](https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-metadata.html)
- [Yocto Project Test Manual - ptest](https://docs.yoctoproject.org/5.1.2/test-manual/ptest.html)
- [Yocto Project Reference Manual - Variables](https://docs.yoctoproject.org/ref-manual/variables.html)

## 17. 실습: Step-by-Step 튜토리얼

이 섹션은 BitBake/Yocto를 처음 접하는 사람이 실제로 빌드 환경을 구성하고 recipe를 만들어보는 실습입니다. Poky 빌드 환경이 이미 있다고 가정하고 시작합니다.

### 17.1 사전 준비: 빌드 환경 확인

```bash
# Poky 빌드 디렉터리로 이동
cd ~/poky

# 빌드 환경 초기화 (이미 되어 있다면 생략)
source oe-init-build-env build

# BitBake가 동작하는지 확인
bitbake --version
# → BitBake Build Tool Core version 2.x.x
```

### 17.2 실습 1: 새 layer 만들기

제품 커스터마이징은 항상 별도 layer에서 시작합니다. upstream을 직접 수정하지 않습니다.

```bash
# poky/build 디렉터리에서
cd ~/poky
source oe-init-build-env build

# 새 layer 생성
bitbake-layers create-layer ../meta-tutorial

# 생성된 구조 확인
tree ../meta-tutorial
```

생성된 구조:
```
meta-tutorial/
  conf/
    layer.conf
  recipes-example/
    example/
      example_0.1.bb
  COPYING.MIT
  README
```

### 17.3 실습 2: layer를 빌드에 등록

```bash
# layer를 bblayers.conf에 추가
bitbake-layers add-layer ../meta-tutorial

# 등록 확인
bitbake-layers show-layers
# → meta-tutorial 이 목록에 보여야 함
```

### 17.4 실습 3: 간단한 C 프로그램 recipe 작성

먼저 소스 파일을 만듭니다:

```bash
# recipe 디렉터리 생성
mkdir -p ../meta-tutorial/recipes-hello/hello/files

# C 소스 작성
cat > ../meta-tutorial/recipes-hello/hello/files/hello.c << 'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    printf("Hello from BitBake tutorial!\\n");
    return 0;
}
EOF
```

Recipe 파일 작성:

```bash
cat > ../meta-tutorial/recipes-hello/hello/hello_1.0.bb << 'EOF'
SUMMARY = "BitBake tutorial hello application"
DESCRIPTION = "A simple hello world program for learning BitBake"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://hello.c"

S = "${UNPACKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} hello.c -o hello
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 hello ${D}${bindir}/hello
}
EOF
```

### 17.5 실습 4: 빌드 실행

```bash
# recipe가 인식되는지 확인
bitbake-layers show-recipes | grep hello
# → hello: 1.0

# 빌드
bitbake hello
```

빌드 중 출력되는 로그를 관찰해보세요:

```
NOTE: Executing Tasks
NOTE: recipe hello-1.0-r0: task do_fetch: Started
NOTE: recipe hello-1.0-r0: task do_fetch: Succeeded
NOTE: recipe hello-1.0-r0: task do_unpack: Started
...
NOTE: recipe hello-1.0-r0: task do_compile: Started
NOTE: recipe hello-1.0-r0: task do_compile: Succeeded
...
NOTE: Tasks Summary: Attempted X tasks of which Y didn't need to be rerun and all succeeded.
```

### 17.6 실습 5: 빌드 결과 확인

```bash
# WORKDIR 확인
ls tmp/work/*/hello/1.0/
# → source/  build/  image/  temp/  packages-split/

# 설치된 파일 확인
ls tmp/work/*/hello/1.0/image/usr/bin/
# → hello

# 생성된 package 확인
ls tmp/deploy/ipk/*/hello*
# → hello_1.0-r0_<arch>.ipk

# recipe 변수 확인
bitbake hello -e | grep "^WORKDIR="
bitbake hello -e | grep "^D="
```

### 17.7 실습 6: recipe 수정하고 재빌드

`do_install`에 메시지를 추가해봅니다:

```bash
# recipe 파일 열어서 do_install 수정
# (에디터로 ../meta-tutorial/recipes-hello/hello/hello_1.0.bb 열기)
```

`do_install` 함수를 다음과 같이 수정:

```bitbake
do_install() {
    install -d ${D}${bindir}
    install -m 0755 hello ${D}${bindir}/hello
    bbnote "Hello binary installed to ${D}${bindir}"
}
```

```bash
# 다시 빌드 — do_install부터 재실행됨
bitbake hello

# 강제로 전체 재빌드
bitbake hello -c cleanall
bitbake hello
```

### 17.8 실습 7: bbappend로 기존 recipe 수정

이번에는 `hello` recipe를 직접 수정하지 않고 bbappend로 변경을 추가합니다.

```bash
# bbappend 파일 생성
mkdir -p ../meta-tutorial/recipes-hello/hello

cat > ../meta-tutorial/recipes-hello/hello/hello_%.bbappend << 'EOF'
# hello recipe에 post-install message 추가
do_install:append() {
    bbnote "bbappend: post-install hook executed"
}
EOF

# 빌드
bitbake hello -c cleanall
bitbake hello
```

로그에서 `bbappend: post-install hook executed` 메시지가 출력되는지 확인합니다.

### 17.9 실습 8: 의도적으로 실패 유발하고 디버깅

일부러 컴파일 에러를 만들어 디버깅 절차를 연습합니다.

```bash
# hello.c에 문법 오류 추가
cat > ../meta-tutorial/recipes-hello/hello/files/hello.c << 'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    printf("Hello from BitBake tutorial!\\n")
    return 0;
}
EOF

# 빌드 시도 → do_compile에서 실패
bitbake hello
```

실패 로그가 출력됩니다. 이제 디버깅:

```bash
# 실패한 task의 로그 확인
cat tmp/work/*/hello/1.0/temp/log.do_compile

# 오류 수정 후 재빌드 (compile만 다시 실행)
# (hello.c의 세미콜론 추가)
bitbake hello -c compile -f
bitbake hello
```

### 17.10 실습 9: devshell로 빌드 환경 진입

```bash
# recipe의 빌드 환경으로 interactive shell 진입
bitbake hello -c devshell

# devshell 안에서:
echo $CC              # cross-compiler 경로
echo $CFLAGS          # 컴파일 flag
echo $WORKDIR         # 작업 디렉터리
ls source/            # 소스 코드
ls image/             # 설치 대상

# 수동으로 컴파일 테스트
$CC $CFLAGS source/hello.c -o hello

# 종료
exit
```

### 17.11 실습 10: package를 image에 포함

```bash
# local.conf에 package 추가
echo 'IMAGE_INSTALL:append = " hello"' >> conf/local.conf

# minimal image 빌드 (시간이 오래 걸릴 수 있음)
# bitbake core-image-minimal

# 또는 package만 빌드해서 확인
bitbake hello
```

### 17.12 실습 후 정리

```bash
# layer 제거
bitbake-layers remove-layer ../meta-tutorial

# local.conf에서 IMAGE_INSTALL 라인 제거
# (에디터로 conf/local.conf 수정)
```

### 17.13 자주 만나는 문제와 해결

| 문제 | 원인 | 해결 |
| --- | --- | --- |
| `ERROR: Nothing PROVIDES 'hello'` | recipe를 찾지 못함 | `bitbake-layers show-recipes \| grep hello`로 확인, BBFILES glob 확인 |
| `do_fetch` 실패 | `SRC_URI` 경로 오류 | `file://` 경로가 recipe 기준 `files/` 디렉터리를 가리키는지 확인 |
| `do_configure` 실패 | `S` 디렉터리가 없음 | `S = "${UNPACKDIR}"` 또는 `S = "${WORKDIR}/git"` 확인 |
| `do_compile` 실패 | 컴파일 에러 | `log.do_compile` 확인, `devshell`에서 수동 컴파일 테스트 |
| `do_install` 실패 | `${D}` 경로 문제 | `install -d`로 디렉터리 먼저 생성했는지 확인 |
| `do_package` 실패 | 파일이 package에 안 들어감 | `FILES:${PN}` 설정 확인 |
| recipe parse 에러 | 문법 오류 | `bitbake -p`로 parse만 실행해서 오류 위치 확인 |
| 빌드가 너무 오래 걸림 | 모든 task 재실행 | sstate cache 확인 (`SSTATE_MIRRORS` 설정), `BB_NUMBER_THREADS`, `PARALLEL_MAKE` 조정 |
