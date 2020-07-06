contract A {
	function foo() internal pure virtual returns (uint256) {}
}
contract B is A {
	function foo() internal pure override virtual returns (uint256) {}
}
contract C is A {
	function foo() internal view override virtual returns (uint256) {}
}
contract D is B, C {
	function foo() internal override(B, C) virtual returns (uint256) {}
}
contract E is C, B {
	function foo() internal pure override(B, C) virtual returns (uint256) {}
}
// ----
// TypeError 6959: (181-247): Overriding function changes state mutability from "pure" to "view".
// TypeError 6959: (272-339): Overriding function changes state mutability from "pure" to "nonpayable".
// TypeError 6959: (272-339): Overriding function changes state mutability from "view" to "nonpayable".
