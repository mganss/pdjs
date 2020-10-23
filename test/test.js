include("include.js");

inlets = 3;
outlets = 2;

post("Hello, world!");
post("jsarguments", jsarguments);
cpost("cpost", 1, 2, 3);
error("error");
post(`inlets: ${inlets} outlets: ${outlets}`);

function bang() {
    post("Bang on inlet " + inlet);
    outlet(0, "Hello, world!");
    outlet(1, "bang");
    outlet(0, 47.11);
    outlet(0, ["x", "y", "z", 1, 2, 3]);
    outlet(1, "hallo", "ballo", 12.34);
    messnamed("mess", "bang");
    messnamed("mess", 3.14);
    messnamed("mess", "test");
    messnamed("mess", "test", [1, 2, 3], 4);
}

function msg_float(f) {
    post("float: " + f);
}

function foo(f, s) {
    post("foo: " + f + " '" + s + "'");
}

function list() {
    let args = Array.from(arguments);
    post(`list ${args.length}: ${args.join(' ')}`);
}

function anything() {
    post("anything: " + messagename);
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
