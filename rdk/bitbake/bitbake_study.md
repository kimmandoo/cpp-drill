# Bitbake

https://docs.yoctoproject.org/bitbake/bitbake-user-manual/bitbake-user-manual-intro.html#introduction

BitBake는 리눅스 배포판 자체가 아니라 .bb 레시피와 .conf 설정 메타데이터로 작업 그래프를 만들고 병렬 실행하는 범용 빌드 엔진. cmake, ninja랑 같은 과인듯.

Makefile 대신 .bb 레시피, .conf 설정, .bbclass 클래스, .bbappend 확장 파일 같은 메타데이터를 사용함.

python으로 작성된 런타임 엔진정도.

BitBake는 작업 실행기
OpenEmbedded는 BitBake가 읽는 거대한 메타데이터 집합
Yocto Project는 OpenEmbedded 빌드 시스템, Poky 참조 배포판, 문서, 테스트, 툴링 등을 묶어 임베디드 리눅스 이미지를 재현 가능하게 만드는 프로젝트

단위가 점점 커짐.

## build

```text
설정 읽기
  ↓
레이어 찾기
  ↓
레시피(.bb)와 append(.bbappend) 파싱
  ↓
provider / version / dependency 결정
  ↓
task graph 생성
  ↓
do_fetch, do_unpack, do_patch, do_configure, do_compile, do_install...
  ↓
패키징 / 이미지 생성
  ↓
tmp/deploy/images 등에 결과물 배치
```

BitBake는 먼저 bblayers.conf, 각 레이어의 layer.conf, 그리고 bitbake.conf 같은 기본 설정 메타데이터를 파싱한다. 

BBPATH는 설정 파일과 클래스 파일을 찾는 데 쓰이고, BBFILES는 .bb와 .bbappend 파일을 찾는 데 쓰인다.

각 타깃은 fetch, unpack, patch, configure, compile 같은 여러 task로 쪼개지고 BitBake는 이 task들을 독립 단위로 보고 의존성이 만족되는 순서대로 병렬 실행함. 

완료된 task는 stamp와 signature를 통해 다시 실행할 필요가 있는지 판단하고, sstate/setscene 메커니즘을 통해 이전 빌드 산출물을 재사용할 수 있다.

cmake랑 유사한 부분이 많은 듯.

| 파일              | 의미                                                                             |
| --------------- | ------------------------------------------------------------------------------ |
| `.bb`           | 레시피. 하나의 소프트웨어 컴포넌트를 어떻게 가져오고, 빌드하고, 설치하고, 패키징할지 정의                      |
| `.bbappend`     | 기존 `.bb` 레시피를 복사하지 않고 덧붙이거나 수정 BSP, 커스터마이징에서 매우 중요합니다.                     |
| `.bbclass`      | 여러 레시피가 공유하는 공통 빌드 로직 예: `cmake`, `autotools`, `meson`, `systemd`.         |
| `.conf`         | 빌드 설정 예: `local.conf`, `bblayers.conf`, `machine/*.conf`, `distro/*.conf`. |
| `layer.conf`    | 레이어가 어떤 레시피와 클래스를 제공하는지 BitBake에 알려줌                                       |
| `local.conf`    | 사용자의 로컬 빌드 설정 `MACHINE`, 병렬 빌드, 이미지 추가 패키지 등을 지정                       |
| `bblayers.conf` | 빌드에 포함할 레이어 목록                                                             |


.bb는 레시피, .conf는 빌드 설정, .bbclass는 공유 기능, .bbappend는 기존 레시피 확장/수정 용도. .bbappend는 대응되는 .bb 레시피가 있어야 하며, 이름 매칭이 맞지 않으면 빌드 시작 시 오류발생함.

레시피가 cmake 에서 CMakeLists.txt 같은 역할임.

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

간단히 예제를 들어보면

SRC_URI는 소스를 어디서 가져올 지 지정한다. git소스라면 SRCREV(소스 리비전)를 명시해야되고, 별도의 빌드 설정파일이 없으면 do_compile, do_install을 직접 구현해야됨

| 변수               | 의미                                                                |
| ---------------- | ----------------------------------------------------------------- |
| `PN`             | recipe/package 이름. 보통 파일명에서 자동 유도됩니다.                             |
| `PV`             | 버전. 예: `helloworld_1.0.bb`라면 보통 `1.0`.                            |
| `PR`             | recipe revision. 현대 Yocto에서는 signature가 중요해서 예전만큼 자주 직접 만지지 않습니다. |
| `SRC_URI`        | 소스 위치. tarball, Git, 로컬 파일, patch 등을 지정합니다.                       |
| `SRCREV`         | Git 등 SCM에서 사용할 정확한 revision.                                     |
| `S`              | 소스가 풀린 디렉터리.                                                      |
| `B`              | 빌드 디렉터리.                                                          |
| `D`              | `do_install`이 파일을 설치하는 임시 destination root.                       |
| `DEPENDS`        | 빌드 타임 의존성. 다른 recipe 이름을 씁니다.                                     |
| `RDEPENDS:${PN}` | 런타임 의존성. 생성된 패키지 기준입니다.                                           |
| `FILES:${PN}`    | 어떤 파일이 어떤 패키지에 들어갈지 지정합니다.                                        |
| `PACKAGES`       | 이 recipe가 생성할 패키지 목록입니다.                                          |
| `IMAGE_INSTALL`  | 이미지에 포함할 패키지 목록입니다.                                               |
| `MACHINE`        | 타깃 보드/머신. 예: `qemux86-64`, `raspberrypi5`.                        |
| `DISTRO`         | 배포판 정책. 예: `poky`.                                                |
| `BBLAYERS`       | 빌드에 포함할 layer 목록입니다.                                              |

DEPENDS는 빌드 때 무조건 필요한 의존성, RDEPENDS:${PN}은 런타임 패키지 의존성. 컴파일이랑 런타임 차이라고 보면 될 듯

## 문법

```bitbake
A = "value"         # 기본 대입. 참조 변수는 나중에 확장됨. 사용 시점에
A ?= "default"      # A가 아직 없으면 대입.
A ??= "weak"        # 약한 기본값.
A := "${B}"         # 즉시 확장. 파싱 시점에
A += " item"        # 공백을 넣고 append.
A .= "suffix"       # 공백 없이 append.
A:append = " x"     # override 스타일 append. 공백 직접 넣어야 함.
A:prepend = "x "
A:remove = "x"
```

```bitbake
FOO = "a"
FOO:append = " b"
# 결과: "a b"

BAR = "a"
BAR:append = "b"
# 결과: "ab"
```

append, prepend, remove는 공백 처리를 직접해야 됨

## task

bitbake의 실행단위는 task. 관례 상 각 task 이름은 do_로 시작함.

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

task는 shell 함수 또는 BitBake 스타일 Python 함수로 작성할 수 있고, addtask로 task 그래프에 넣습니다. 

```bitbake
python do_printdate() {
    import datetime
    bb.plain("Date: %s" % datetime.date.today())
}

addtask printdate after do_fetch before do_build
```

예를 들어 addtask printdate after do_fetch before do_build는 do_fetch 이후, do_build 이전에 do_printdate를 실행하게 만듭니다. do_printdate로 선언했지만 addtask printdate가 된다?!

한 번 성공한 task는 stamp 때문에 다시 실행되지 않을 수 있습니다. 강제로 다시 실행하고 싶으면 다음처럼 합니다.

```bitbake
bitbake helloworld -c compile -f
bitbake helloworld

bitbake helloworld -C compile // 특정 task 부터 다시 빌드할거면
```

`bitbake core-image-minimal`  지정한 target의 기본 task, 보통 do_build를 실행.

`bitbake helloworld -c listtasks` 특정 recipe가 가진 task 목록을 봅니다.

```bitbake
bitbake helloworld -c fetch
bitbake helloworld -c unpack
bitbake helloworld -c compile
bitbake helloworld -c install
```

특정 task만 실행함.

`bitbake -e helloworld | less` 변수가 최종적으로 어떤 값이 되었는지 확인합니다. 설정 디버깅에서 매우 중요합니다.

`bitbake -s` 사용 가능한 recipe와 버전을 봅니다.

`bitbake -g core-image-minimal` 의존성 그래프 파일 생성

`bitbake -n core-image-minimal` dry-run입니다. 실제 실행하지 않고 흐름을 봅니다.

`bitbake -p` 파싱만 합니다. 레이어/레시피 문법 오류를 확인할 때 유용합니다.

`bitbake -k core-image-minimal` 에러가 나도 가능한 만큼 계속 빌드합니다.

`bitbake -S printdiff helloworld` 왜 task가 다시 실행되는지 signature 차이를 추적할 때 씁니다.

## Layer

Layer는 recipe, class, conf, patch, machine 설정 등을 묶는 단위. 

```bash
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

Layer를 쓰는 이유는 수정사항을 원본 Yocto/OE-Core 레이어와 분리하기 위해서. worktree같은거라고 보면 될듯. 가능하면 .bbappend로 필요한 부분만 덧붙이는 방식이 낫습니다. 공식 문서도 레이어는 커스터마이징을 서로 분리하기 위한 구조이며, 전체 recipe overlay보다는 .bbappend 사용을 권장

# yocto 이미지 빌드해보기

```bash
sudo apt-get update

sudo apt-get install -y \
  build-essential chrpath cpio debianutils diffstat file gawk gcc git \
  iputils-ping libacl1 libcrypt-dev locales python3 python3-git \
  python3-jinja2 python3-pexpect python3-pip python3-subunit socat \
  texinfo unzip wget xz-utils zstd]
  
mkdir -p ~/yocto-lab
cd ~/yocto-lab

git clone https://git.openembedded.org/bitbake
./bitbake/bin/bitbake-setup init
```

선택은 대충 맞게

- configuration: poky-wrynose 또는 poky-master
- bitbake configuration: poky
- MACHINE: qemux86-64
- DISTRO: poky

소싱. source poky-master-poky-distro_poky-machine_qemux86-64/build/init-build-env

소싱하게 되면 `poky-master-poky-distro_poky-machine_qemux86-64/build/`로 이동하게 됨.

```bash
bitbake-config-build list-fragments # 현재설정확인
bitbake-config-build enable-fragment core/yocto/root-login-with-empty-password # root 로그인 허용하기
```

gui이미지 말고 일단 작은 이미지 빌드하기

```bash
bitbake core-image-minimal
bitbake core-image-sato # 이건 공식 튜토리얼
```

빌드 결과는 보통 여기 생김 - tmp/deploy/images/<machine>/

이건 build 디렉토리 아래에 있다.











