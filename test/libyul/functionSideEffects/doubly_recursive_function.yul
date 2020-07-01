{
    function a() { b() }
    function b() { a() }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
// b: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
