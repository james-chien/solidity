{
    function a() { b() }
    function b() { c() }
    function c() { b() }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
// b: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
// c: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
