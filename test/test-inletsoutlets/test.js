inlets = 1;
outlets = 1;

function change(i) {
    post("change inlets from", inlets, "to", inlets + i);
    inlets = inlets + i;
    post("change outlets from", outlets, "to", outlets + i);
    outlets = outlets + i;
}
