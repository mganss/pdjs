var o = { "x": 1 };
x = 2;
y = "y";
include("include.js", o);
post("o.x", o.x);
post("x", x);

var req = require("require.js");
req.bar();
post("require foo", req.foo);
require(); // -> undefined

include("compile_error.js", o);
include("run_error.js", {});
