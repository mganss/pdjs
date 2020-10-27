#  pdjs

[![GitHub Actions](https://github.com/mganss/pdjs/workflows/CI/badge.svg)](https://github.com/mganss/pdjs/workflows/CI/badge.svg)
[![codecov](https://codecov.io/gh/mganss/pdjs/branch/master/graph/badge.svg?token=U4K4490WIM)](https://codecov.io/gh/mganss/pdjs/branch/master)

A JavaScript external for Pure Data based on [V8](https://v8.dev/).

pdjs tries to emulate Max's [js](https://docs.cycling74.com/max8/refpages/js) object.
Many JavaScript source files written for Max js should work unchanged with pdjs. 

While the Max js object uses Mozilla's [SpiderMonkey](https://en.wikipedia.org/wiki/SpiderMonkey) JavaScript engine that was released in 2011 with Firefox 4.0 and thus
lacks many newer language features (such as `let`), pdjs uses Google's V8 JavaScript engine which supports the latest ECMAScript standards and provides much better performance.

### Supported platforms

- Windows x64
- Linux x64

Planned:

- Linux ARM64
- macOS x64

## Usage

Currently, you'll have to grab a build artifact from [GitHub Actions](https://github.com/mganss/pdjs/actions) for your platform (`js.ps_linux` for Linux, `js.dll` for Windows)
and place it where your PD patch can find it. Then create a `js` object giving it the name of a JavaScript file, e.g. `js src.js`, relative to your patch or absolute. For more usage information you can consult the [Max JavaScript documentation](https://docs.cycling74.com/max8/vignettes/javascriptinmax) which applies to pdjs as well.

## Feature support

### General

There is no built-in editor like in Max, source files have to be created and edited outside of Pure Data.

### [Arguments](https://docs.cycling74.com/max8/refpages/js#Arguments)

- [x] `filename`
- [ ] `inlets-outlets`
- [x] `jsarguments`

### [Messages](https://docs.cycling74.com/max8/refpages/js#Messages)

- [x] `bang`
- [ ] `int` (there are no ints in PD)
- [x] `float`
- [x] `list`
- [x] `anything`
- [ ] `autowatch`
- [x] `compile`
- [x] `delprop`
- [ ] `editfontsize`
- [x] `getprop`
- [x] `loadbang`
- [ ] `open`
- [x] `setprop`
- [ ] `statemessage`
- [ ] `wclose`

### [Special function names](https://docs.cycling74.com/max8/vignettes/jsbasic#Special_Function_Names)

- [ ] `msg_int`
- [x] `msg_float`
- [x] `list`
- [x] `anything`
- [x] `loadbang`
- [ ] `getvalueof`
- [ ] `setvalueof`
- [ ] `save`
- [ ] `notifydeleted`

Private functions are supported.

### [Global functions](https://docs.cycling74.com/max8/vignettes/jsglobal)

- [x] `cpost`
- [x] `error`
- [x] `include`
- [x] `messnamed`
- [x] `post`
- [x] `require`
- [ ] `arrayfromargs` (use `Array.from(arguments)` or `[...arguments]` instead)
- [ ] `assist`
- [ ] `declareattribute`
- [ ] `embedmessage`
- [ ] `notifyclients`
- [x] `outlet` (no support for `jsobject`)
- [ ] `setinletassist`
- [ ] `setoutletassist`

### Global properties

- [ ] `autowatch`
- [ ] `editfontsize`
- [x] `inlet`
- [x] `inlets`
- [ ] `inspector`
- [x] `jsarguments` (no support for `jsargs` message)
- [ ] `Max`
- [ ] `maxclass`
- [x] `messagename`
- [ ] `patcher`
- [x] `outlets`

### Other Objects

There is no support currently for other objects such as `Buffer`, `Dict`, `File`, etc.

## Building

pdjs uses CMake to build. The V8 library is pulled in through [vcpkg](https://github.com/microsoft/vcpkg) and [pd.build](https://github.com/pierreguillot/pd.build) is used to build the external library.

### Prerequisites

- Windows: Visual Studio 2019 (any edition, older versions may work, though not tested). You need to have the [Desktop development with C++](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019) workload installed. If you want to build the Linux external from VS you'll also need the workload [Linux development with C++](https://docs.microsoft.com/en-us/cpp/linux/download-install-and-setup-the-linux-development-workload?view=vs-2019). 

- Linux: 
  - g++
  - cmake 3.14 or higher (if your distro has an older version, you can grab a static build from https://github.com/Microsoft/CMake/releases)
  - ninja-build
 
### Installing the V8 package

vcpkg is used as a git submodule, so you can drop into a command line and [bootstrap vcpkg](https://github.com/microsoft/vcpkg#getting-started)
in the vcpkg subdirectory after cloning this repo. Then, either do `vcpkg install v8:x64-windows-static` or `vcpkg install v8:x64-linux` and grab a coffee.

### Building

Also check out the GitHub Actions [workflow definition](https://github.com/mganss/pdjs/blob/master/.github/workflows/main.yml) for more details on the build process. The GitHub Actions build doesn't use the V8 vcpkg from source but rather a pre-built export that only has the release binaries.

#### Windows

Open the top-level directory of the repo in VS and hit F6. The CMakeSettings.json contains 4 configurations: `x64-Debug` and `x64-Release` for Windows builds and `WSL-GCC-Debug` and `WSL-GCC-Release` for Linux builds through WSL.

#### Linux

```sh
mkdir -p out/build/x64-linux-Debug
cmake -G Ninja \
  -DVERSION=1.0 \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg-export/scripts/buildsystems/vcpkg.cmake \
  -B out/build/x64-linux-Debug -S .
cmake --build out/build/x64-linux-Debug -- -v
```
