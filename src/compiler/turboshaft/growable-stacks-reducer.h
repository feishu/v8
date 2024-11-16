// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_TURBOSHAFT_GROWABLE_STACKS_REDUCER_H_
#define V8_COMPILER_TURBOSHAFT_GROWABLE_STACKS_REDUCER_H_

#include "src/compiler/globals.h"
#include "src/compiler/turboshaft/assembler.h"
#include "src/compiler/turboshaft/graph.h"
#include "src/compiler/turboshaft/index.h"
#include "src/compiler/turboshaft/operations.h"
#include "src/compiler/turboshaft/phase.h"
#include "src/compiler/turboshaft/representations.h"
#include "src/compiler/turboshaft/uniform-reducer-adapter.h"

namespace v8::internal::compiler::turboshaft {

#include "src/compiler/turboshaft/define-assembler-macros.inc"

template <class Next>
class GrowableStacksReducer : public Next {
 public:
  TURBOSHAFT_REDUCER_BOILERPLATE(GrowableStacks)

  GrowableStacksReducer() {
    call_descriptor_ = compiler::GetWasmCallDescriptor(
        __ graph_zone(), __ data()->wasm_module_sig());
#if V8_TARGET_ARCH_32_BIT
    call_descriptor_ =
        compiler::GetI32WasmCallDescriptor(__ graph_zone(), call_descriptor_);
#endif
  }

  OpIndex REDUCE(Return)(OpIndex pop_count,
                         base::Vector<const OpIndex> return_values,
                         bool caller_frame_slots_copied) {
    if (call_descriptor_->ReturnSlotCount() > 0) {
      V<Word32> frame_marker = __ Load(
          __ FramePointer(), LoadOp::Kind::RawAligned(),
          MemoryRepresentation::Uint32(), WasmFrameConstants::kFrameTypeOffset);
      V<Word32> wasm_segment_start_const = __ Word32Constant(
          StackFrame::TypeToMarker(StackFrame::WASM_SEGMENT_START));

      Label<WordPtr> done(&Asm());
      IF (UNLIKELY(__ Equal(frame_marker, wasm_segment_start_const,
                            RegisterRepresentation::Word32()))) {
        auto sig =
            FixedSizeSignature<MachineType>::Returns(MachineType::Pointer())
                .Params(MachineType::Pointer());
        const CallDescriptor* ccall_descriptor =
            compiler::Linkage::GetSimplifiedCDescriptor(__ graph_zone(), &sig);
        const TSCallDescriptor* ts_ccall_descriptor = TSCallDescriptor::Create(
            ccall_descriptor, compiler::CanThrow::kNo,
            compiler::LazyDeoptOnThrow::kNo, __ graph_zone());
        GOTO(done,
             __ template Call<WordPtr>(
                 __ ExternalConstant(ExternalReference::wasm_load_old_fp()),
                 {__ ExternalConstant(ExternalReference::isolate_address())},
                 ts_ccall_descriptor));
      } ELSE {
        GOTO(done, __ FramePointer());
      }
      BIND(done, old_fp);

      base::SmallVector<OpIndex, 8> no_stack_values;
      for (size_t i = 0; i < call_descriptor_->ReturnCount(); i++) {
        LinkageLocation loc = call_descriptor_->GetReturnLocation(i);
        if (!loc.IsCallerFrameSlot()) {
          no_stack_values.push_back(return_values[i]);
          continue;
        }
        MachineType return_type = call_descriptor_->GetReturnType(i);
        __ Store(old_fp, return_values[i], StoreOp::Kind::RawAligned(),
                 MemoryRepresentation::FromMachineType(return_type),
                 compiler::kNoWriteBarrier,
                 FrameSlotToFPOffset(loc.GetLocation()));
      }
      return Next::ReduceReturn(pop_count, base::VectorOf(no_stack_values),
                                true);
    }
    return Next::ReduceReturn(pop_count, return_values,
                              caller_frame_slots_copied);
  }

 private:
  CallDescriptor* call_descriptor_ = nullptr;
};

#include "src/compiler/turboshaft/undef-assembler-macros.inc"

}  // namespace v8::internal::compiler::turboshaft

#endif  // V8_COMPILER_TURBOSHAFT_GROWABLE_STACKS_REDUCER_H_
