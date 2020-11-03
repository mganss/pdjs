function bang() {
    post("bang on inlet " + inlet);
}

function msg_float(f) {
    post("float", f);
}

function foo(f, s) {
    post("foo", f, s);
}

function list() {
    let args = Array.from(arguments);
    post("list", args);
}

function anything() {
    post("anything", messagename, Array.from(arguments));
}

function loadbang() {
    post("loadbang");
}

function private() {
    post("private");
}

private.private = 1;

function exception() {
    throw "exception";
}
