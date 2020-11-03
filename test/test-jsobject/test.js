var jso = { x: "test" };

function jso_mess() {
    messnamed("jso", jso);
}

function jso_out() {
    post("jso_out");
    outlet(0, jso);
}

function jso_in(o) {
    post("jso_in", o.x);
}

var jso2 = { x: "other " };

function jso_mess2() {
    messnamed("jso", jso2);
}

function jso_mess3() {
    var lo = { x: "local" };
    messnamed("jso", lo);
    lo = {};
}
