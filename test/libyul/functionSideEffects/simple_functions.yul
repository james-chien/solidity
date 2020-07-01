{
    function a() {}
    function f() { mstore(0, 1) }
    function g() { sstore(0, 1) }
    function h() { let x := msize() }
    function i() { let z := mload(0) }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: movable, sideEffectFree, sideEffectFreeIfNoMSize
// f: writes memory
// g: writes storage
// h: sideEffectFree, sideEffectFreeIfNoMSize, reads memory
// i: sideEffectFreeIfNoMSize, reads memory
