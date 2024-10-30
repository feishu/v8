// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const {Protocol} = InspectorTest.start(
    "Tests if `using` and `await using` works in REPL mode.");

Protocol.Runtime.evaluate({ expression: 'using x = { value: 1, [Symbol.dispose]() {return 42;}};', replMode: true });
Protocol.Runtime.evaluate({ expression: 'await using x = { value: 2, [Symbol.asyncDispose]() {return 43;}};', replMode: true });
