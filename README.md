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
- [ ] `delprop`
- [ ] `editfontsize`
- [ ] `getprop`
- [x] `loadbang`
- [ ] `open`
- [ ] `setprop`
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
- [x] `include` (no support for object argument)
- [x] `messnamed`
- [x] `post`
- [ ] `require`
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
