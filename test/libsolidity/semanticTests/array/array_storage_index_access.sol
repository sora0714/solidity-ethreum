contract C {
    uint[] storageArray;
    function test_indices(uint256 len) public
    {
        while (storageArray.length < len)
            storageArray.push();
        while (storageArray.length > len)
            storageArray.pop();
        for (uint i = 0; i < len; i++)
            storageArray[i] = i + 1;

        for (uint i = 0; i < len; i++)
            require(storageArray[i] == i + 1);
    }
}
// ----
// test_indices(uint256): 1 ->
// test_indices(uint256): 129 ->
// gas irOptimized: 3017687
// gas legacy: 3038654
// gas legacyOptimized: 2995964
// test_indices(uint256): 5 ->
// gas irOptimized: 579670
// gas legacy: 573810
// gas legacyOptimized: 571847
// test_indices(uint256): 10 ->
// gas irOptimized: 157953
// gas legacy: 160108
// gas legacyOptimized: 156996
// test_indices(uint256): 15 ->
// gas irOptimized: 172733
// gas legacy: 175973
// gas legacyOptimized: 171596
// test_indices(uint256): 0xFF ->
// gas irOptimized: 5673823
// gas legacy: 5715748
// gas legacyOptimized: 5632556
// test_indices(uint256): 1000 ->
// gas irOptimized: 18173005
// gas legacy: 18347810
// gas legacyOptimized: 18037248
// test_indices(uint256): 129 ->
// gas irOptimized: 4166279
// gas legacy: 4140113
// gas legacyOptimized: 4108272
// test_indices(uint256): 128 ->
// gas irOptimized: 405522
// gas legacy: 433498
// gas legacyOptimized: 400909
// test_indices(uint256): 1 ->
// gas irOptimized: 583437
// gas legacy: 576715
// gas legacyOptimized: 575542
