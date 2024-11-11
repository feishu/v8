// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --allow-natives-syntax
function store(a, value) {
  a[0] = value;
}
%PrepareFunctionForOptimization(store);

// Warm the IC up with HOLEY_DOUBLE_ELEMENTS.
let double_array = new Array(5);
double_array[0] = 0.5;
store(double_array, 0);

function createSmiArray() {
  // The array creation here has an AllocationSite.
  return new Array(5);  // HOLEY_SMI_ELEMENTS
}
%PrepareFunctionForOptimization(createSmiArray);
let smi_array = new createSmiArray();
store(smi_array, 0);

// The StoreKeyedSloppy IC in store should stay monomorphic.
let feedback = %GetFeedback(store);
if (feedback != undefined) {
   assertEquals('StoreKeyedSloppy', feedback[0][0]);
   assertTrue(feedback[0][1].startsWith('MONOMORPHIC'));
}
