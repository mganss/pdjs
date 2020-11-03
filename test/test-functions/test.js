inlets = 1;
outlets = 2;

function bang() {
    post("post");
    cpost("cpost", 1, 2, 3);
    error("error");
    outlet(0, "outlet 1");
    outlet(1, "outlet 2");
    outlet(0, 47.11);
    outlet(0, ["x", "y", "z", 1, 2, 3]);
    outlet(1, "hallo", "ballo", 12.34);
    messnamed("mess", "bang");
    messnamed("mess", 3.14);
    messnamed("mess", "test");
    messnamed("mess", "test", [1, 2, 3], 4);
}
