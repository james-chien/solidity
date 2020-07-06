{
  function f() -> a, b {}
  let x
  x, x := f()
}
// ----
// DeclarationError 9005: (38-49): Variable x occurs multiple times on the left-hand side of the assignment.
