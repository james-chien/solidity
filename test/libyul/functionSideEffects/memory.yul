{
    function a() { mstore8(0, 32) }
    function f() { a() }
    function g() { sstore(0, 1) } // does not affect memory
    function h() { pop(mload(0)) }
    function i() { pop(msize()) }
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize
// a: writes memory
// f: writes memory
// g: writes storage
// h: sideEffectFreeIfNoMSize, reads memory
// i: sideEffectFree, sideEffectFreeIfNoMSize, reads memory
