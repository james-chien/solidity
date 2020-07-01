{
    function a() { a() }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
