{
  function f() -> x { x := g() }
  function g() -> x { for {} 1 {} {} }
  pop(f())
}
// ----
// : movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
// f: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
// g: movable, sideEffectFree, sideEffectFreeIfNoMSize, can loop
