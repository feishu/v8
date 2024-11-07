// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --js-explicit-resource-management

let {session, contextGroup, Protocol} = InspectorTest.start(
    'Checks if catch prediction works on new `using` and `await using` syntax.');

Protocol.Debugger.enable();
Protocol.Debugger.onPaused(({params: {data}}) => {
  InspectorTest.log('paused on exception:');
  InspectorTest.logMessage(data);
  Protocol.Debugger.resume();
});

contextGroup.addInlineScript(
    `
function disposalUncaughtUsingSyntax() {
    using x = {
      value: 1,
      [Symbol.dispose]() {
    throw new Error("fail");
    }
    };
}

async function disposalUncaughtAwaitUsingSyntax() {
    await using y = {
      value: 2,
      [Symbol.asyncDispose]() {
    throw new Error("fail");
    }
    };
}

function disposalCaughtUsingSyntax() {
  try {
    using x = {
      value: 1,
      [Symbol.dispose]() {
    throw new Error("fail");
    }
    };
  } catch (e) {
  }
}

async function disposalCaughtAwaitUsingSyntax() {
  try {
    await using y = {
      value: 2,
      [Symbol.asyncDispose]() {
    throw new Error("fail");
    }
    };
  } catch (e) {
  }
}
`,
    'test.js');

InspectorTest.runAsyncTestSuite([
  async function testPauseOnInitialState() {
    await evaluate('disposalUncaughtUsingSyntax()');
    await evaluate('disposalUncaughtAwaitUsingSyntax()');
    await evaluate('disposalCaughtUsingSyntax()');
    await evaluate('disposalCaughtAwaitUsingSyntax()');
  },

  async function testPauseOnExceptionOff() {
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
    await evaluate('disposalUncaughtUsingSyntax()');
    await evaluate('disposalUncaughtAwaitUsingSyntax()');
    await evaluate('disposalCaughtUsingSyntax()');
    await evaluate('disposalCaughtAwaitUsingSyntax()');
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
  },

  async function testBreakOnCaughtException() {
    await Protocol.Debugger.setPauseOnExceptions({state: 'caught'});
    await evaluate('disposalUncaughtUsingSyntax()');
    await evaluate('disposalUncaughtAwaitUsingSyntax()');
    await evaluate('disposalCaughtUsingSyntax()');
    await evaluate('disposalCaughtAwaitUsingSyntax()');
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
  },

  async function testBreakOnUncaughtException() {
    await Protocol.Debugger.setPauseOnExceptions({state: 'uncaught'});
    await evaluate('disposalUncaughtUsingSyntax()');
    await evaluate('disposalUncaughtAwaitUsingSyntax()');
    await evaluate('disposalCaughtUsingSyntax()');
    await evaluate('disposalCaughtAwaitUsingSyntax()');
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
  },

  async function testBreakOnAll() {
    await Protocol.Debugger.setPauseOnExceptions({state: 'all'});
    await evaluate('disposalUncaughtUsingSyntax()');
    await evaluate('disposalUncaughtAwaitUsingSyntax()');
    await evaluate('disposalCaughtUsingSyntax()');
    await evaluate('disposalCaughtAwaitUsingSyntax()');
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
  },

  async function testBreakOnExceptionInSilentMode(next) {
    await Protocol.Debugger.setPauseOnExceptions({state: 'all'});
    InspectorTest.log(`evaluate 'disposalUncaughtUsingSyntax()'`);
    await Protocol.Runtime.evaluate(
        {expression: 'disposalUncaughtUsingSyntax()', silent: true});
    InspectorTest.log(`evaluate 'disposalUncaughtAwaitUsingSyntax()'`);
    await Protocol.Runtime.evaluate(
        {expression: 'disposalUncaughtAwaitUsingSyntax()', silent: true});
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
    InspectorTest.log(`evaluate 'disposalCaughtUsingSyntax()'`);
    await Protocol.Runtime.evaluate(
        {expression: 'disposalCaughtUsingSyntax()', silent: true});
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
    InspectorTest.log(`evaluate 'disposalCaughtAwaitUsingSyntax()'`);
    await Protocol.Runtime.evaluate(
        {expression: 'disposalCaughtAwaitUsingSyntax()', silent: true});
    await Protocol.Debugger.setPauseOnExceptions({state: 'none'});
  }
]);

async function evaluate(expression) {
  InspectorTest.log(`\nevaluate '${expression}'..`);
  contextGroup.addInlineScript(expression);
  await InspectorTest.waitForPendingTasks();
}
