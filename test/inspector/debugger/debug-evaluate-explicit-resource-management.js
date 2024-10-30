// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const {Protocol} =
    InspectorTest.start('Check if `using` and `await using` works is debug evaluate.');

(async function testUsing() {
  await Protocol.Debugger.enable();
  await Protocol.Runtime.enable();
  const promise = Protocol.Debugger.oncePaused();

  const { params: { callFrames: [{ callFrameId, functionLocation: { scriptId } }] } } = await promise;

  await Protocol.Debugger.evaluateOnCallFrame({
    callFrameId,
    expression: `
    using x = {
        value: 1,
        [Symbol.dispose]() {
            return 42;
      }
      };
    `
  });

  InspectorTest.completeTest();
})();

(async function testAwaitUsing() {
    await Protocol.Debugger.enable();
    await Protocol.Runtime.enable();
    const promise = Protocol.Debugger.oncePaused();

    const { params: { callFrames: [{ callFrameId, functionLocation: { scriptId } }] } } = await promise;

    await Protocol.Debugger.evaluateOnCallFrame({
      callFrameId,
      expression: `
      await using x = {
        value: 2,
        [Symbol.asyncDispose]() {
            return 43;
      }
      };
      `
    });

    InspectorTest.completeTest();
  })();
