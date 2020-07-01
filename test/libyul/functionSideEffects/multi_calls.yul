{
    function a() {
        b()
    }
    function b() {
        sstore(0, 1)
        b()
    }
    function c() {
        mstore(0, 1)
        a()
        d()
    }
    function d() {
    }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: can loop, writes storage
// b: can loop, writes storage
// c: can loop, writes storage, writes memory
// d: movable, sideEffectFree, sideEffectFreeIfNoMSize
