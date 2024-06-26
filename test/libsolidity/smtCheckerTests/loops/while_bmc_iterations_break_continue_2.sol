contract C {
	function f() public pure {
		uint x;
		while (x < 3) {
			if (x > 1) {
				x = 3;
				break;
			}
			if (x >= 0) {
				x = 2;
				continue;
			}
		}
		assert(x == 3);
	}
}
// ====
// SMTEngine: bmc
// SMTSolvers: z3
// BMCLoopIterations: 3
// ----
// Info 6002: BMC: 1 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
