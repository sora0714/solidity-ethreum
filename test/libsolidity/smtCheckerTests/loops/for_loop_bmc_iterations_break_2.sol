contract C
{
	function f() public pure {
		uint x;
		for (uint i = 0; i < 3; ++i) {
			++x;
			break;
		}
		assert(x == 1);
	}
}
// ====
// SMTEngine: bmc
// SMTSolvers: z3
// BMCLoopIterations: 4
// ----
// Warning 5740: (77-80): Unreachable code.
// Info 6002: BMC: 3 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
