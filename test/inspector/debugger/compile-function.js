// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --max-semi-space-size=1 --trace-gc

let {session, contextGroup, Protocol} = InspectorTest.start(
  'compile function');

let failFunction = `
function func() {
  const a = 1;
}
await new Promise(resolve => {
  func();
  resolve()
});
`;

let successFunction = `
return arg1 + arg2;
`;


InspectorTest.runAsyncTestSuite([
  async function testFailCompileFunction() {
    const url = "file1.js";
    Protocol.Debugger.enable();
    await contextGroup.compileFunction(failFunction, url);

    // no breakpoint
    let result1 = await Protocol.Debugger.setBreakpointByUrl({
        lineNumber: 2,  url
      })

    let hasBreakpoint1 = result1.result.locations.length > 0;
    InspectorTest.log(`hasBreakpoint: ${hasBreakpoint1}`);
  },
  async function testSuccessCompileFunction() {
    const url = "file2.js";
    Protocol.Debugger.enable();
    await contextGroup.compileFunction(successFunction, url, 2, 0, ["arg1", "arg2"]);
    let result1 = await Protocol.Debugger.setBreakpointByUrl({
      lineNumber: 2, url
    });

    let hasBreakpoint1 = result1.result.locations.length > 0;
    InspectorTest.log(`hasBreakpoint: ${hasBreakpoint1}`);
  }
]);
