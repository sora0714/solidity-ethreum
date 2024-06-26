contract C {
	function abiEncodeSlice(string memory sig, uint[] calldata data) external pure {
		bytes memory b1 = abi.encodeWithSignature(sig, data);
		bytes memory b2 = abi.encodeWithSignature(sig, data[0:]);
		// should hold but the engine cannot infer that data is fully equals data[0:] because each index is assigned separately
		assert(b1.length == b2.length); // fails for now

		bytes memory b3 = abi.encodeWithSignature(sig, data[:data.length]);
		// should hold but the engine cannot infer that data is fully equals data[:data.length] because each index is assigned separately
		assert(b1.length == b3.length); // fails for now

		bytes memory b4 = abi.encodeWithSignature(sig, data[5:10]);
		assert(b1.length == b4.length); // should fail

		uint x = 5;
		uint y = 10;
		bytes memory b5 = abi.encodeWithSignature(sig, data[x:y]);
		// should hold but the engine cannot infer that data[5:10] is fully equals data[x:y] because each index is assigned separately
		assert(b1.length == b5.length); // fails for now

		bytes memory b6 = abi.encodeWithSelector("f()", data[5:10]);
		assert(b4.length == b6.length); // should fail
	}
}
// ====
// SMTEngine: all
// ----
// Warning 6328: (335-365): CHC: Assertion violation happens here.\nCounterexample:\n\ndata = [36]\nb3 = []\nb4 = []\nx = 0\ny = 0\nb5 = []\nb6 = []\n\nTransaction trace:\nC.constructor()\nC.abiEncodeSlice(sig, [36]) -- counterexample incomplete; parameter name used instead of value
// Warning 6328: (589-619): CHC: Assertion violation happens here.
// Info 1391: CHC: 3 verification condition(s) proved safe! Enable the model checker option "show proved safe" to see all of them.
