function printprop() {
    post("prop", (typeof prop) !== "undefined" ? prop : "undefined");
}
