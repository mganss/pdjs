#  pdjs

[![GitHub Actions](https://github.com/mganss/pdjs/workflows/CI/badge.svg)](https://github.com/mganss/pdjs/workflows/CI/badge.svg)
[![codecov](https://codecov.io/gh/mganss/pdjs/branch/master/graph/badge.svg?token=U4K4490WIM)](https://codecov.io/gh/mganss/pdjs/branch/master)

A JavaScript external for Pure Data based on [V8](https://v8.dev/).

pdjs tries to emulate Max's [js](https://docs.cycling74.com/max8/refpages/js) object.
Many JavaScript source files written for Max js should work unchanged with pdjs. 

While the Max js object uses a version of Mozilla's [SpiderMonkey](https://en.wikipedia.org/wiki/SpiderMonkey) JavaScript engine that was released in 2011 with Firefox 4.0 and thus
lacks many newer language features (such as `let`), pdjs uses Google's V8 JavaScript engine which supports the latest ECMAScript standards and provides much better performance.

### Supported platforms

- Windows x64
- Linux x64
- Linux arm64
- Linux arm
- macOS x64

## Usage

You can install through deken or grab a zip from [releases](https://github.com/mganss/pdjs/releases). Then create a `js` object giving it the name of a JavaScript file, e.g. `js src.js`, relative to your patch or absolute. You might also have to add a `declare -path pdjs` object so that PD can find the external. For more usage information you can consult the [Max JavaScript documentation](https://docs.cycling74.com/max8/vignettes/javascriptinmax) which applies to pdjs as well.

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
- [x] `open` (Windows only)
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
- [x] `outlet`
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

### Sharing JavaScript objects across `js` object instances

You can pass references to JavaScript objects across `js` object instances using the [`jsobject`](https://docs.cycling74.com/max8/vignettes/jsglobal#outlet) mechanism.

There is also a special global variable called `__global__` that references the same object from every `js` object instance. It's similar to the [`Global`](https://docs.cycling74.com/max8/vignettes/jsglobalobject) object in Max. Unlike in Max, you can also call functions contained in the `__global__` object.

## Building

pdjs uses CMake to build. Prebuilt V8 binaries can be downloaded from [my V8 fork](https://github.com/mganss/v8/releases/latest) and [pd.build](https://github.com/pierreguillot/pd.build) is used to build the external library.

### Prerequisites

- Windows: Visual Studio 2019 (any edition, older versions may work, though not tested). You need to have the [Desktop development with C++](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019) workload installed. If you want to build the Linux external from VS you'll also need the workload [Linux development with C++](https://docs.microsoft.com/en-us/cpp/linux/download-install-and-setup-the-linux-development-workload?view=vs-2019). 

- Linux: 
  - g++
  - cmake 3.13 or higher (if your distro has an older version, you can grab a static build from https://github.com/Microsoft/CMake/releases)
  - ninja-build
 
### V8 libraries

The build process expects the V8 library `v8_monolith` library in `v8/lib/[platform]`, e.g. `v8/lib/x64-linux`. You can either download prebuilt binaries from https://github.com/mganss/v8/releases/latest or build your own. This repo contains the GN configuration files that were used to build V8 in the [`v8`](https://github.com/mganss/pdjs/tree/master/v8) directory.

### Building

Also check out the GitHub Actions [workflow definition](https://github.com/mganss/pdjs/blob/master/.github/workflows/main.yml) for more details on the build process.

#### Windows

Open the top-level directory of the repo in VS and hit F6. The CMakeSettings.json contains 4 configurations: `x64-Debug` and `x64-Release` for Windows builds and `WSL-GCC-Debug` and `WSL-GCC-Release` for x64 Linux builds through WSL.

#### Linux

```sh
mkdir -p out/build/x64-linux-Debug
cmake -G Ninja \
  -DVERSION=1.0 \
  -DCMAKE_BUILD_TYPE=Debug \
  -B out/build/x64-linux-Debug -S .
cmake --build out/build/x64-linux-Debug -- -v
```
