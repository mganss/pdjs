var g = __global__;
var o = { x: "test2" };

g.x = "test2";
g.f = function () {
    post("g.f");
}

function bang() {
    messnamed("jso", o);
    outlet(0, "bang");
}
