inlets = 1;
outlets = 1;

name = "hallo";

function bang() {
    outlet(0, "bang called");
}

function msg_float(f) {
    outlet(0, "msg_float called with", f);
}

function list() {
    outlet(0, "list called with", Array.from(arguments));
}

function test() {
    post("post from function test.");
    outlet(0, "test called with", Array.from(arguments));
}
