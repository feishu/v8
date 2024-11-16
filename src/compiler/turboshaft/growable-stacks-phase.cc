// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/compiler/turboshaft/growable-stacks-phase.h"

#include "src/compiler/turboshaft/copying-phase.h"
#include "src/compiler/turboshaft/growable-stacks-reducer.h"

namespace v8::internal::compiler::turboshaft {

void GrowableStacksPhase::Run(PipelineData* data, Zone* temp_zone) {
  turboshaft::CopyingPhase<turboshaft::GrowableStacksReducer>::Run(data,
                                                                   temp_zone);
}

}  // namespace v8::internal::compiler::turboshaft
