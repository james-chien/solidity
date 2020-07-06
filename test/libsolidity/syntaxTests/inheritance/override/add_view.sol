contract B { function f() virtual public {} }
contract C is B { function f() public view {} }
// ----
// TypeError 9456: (64-91): Overriding function is missing "override" specifier.
