//=- llvm/CodeGen/CriticalAntiDepBreaker.h - Anti-Dep Support -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the CriticalAntiDepBreaker class, which
// implements register anti-dependence breaking along a blocks
// critical path during post-RA scheduler.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_CRITICALANTIDEPBREAKER_H
#define LLVM_CODEGEN_CRITICALANTIDEPBREAKER_H

#include "AntiDepBreaker.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include <map>

namespace llvm {
class RegisterClassInfo;
class TargetInstrInfo;
class TargetRegisterInfo;

  class CriticalAntiDepBreaker : public AntiDepBreaker {
    MachineFunction& MF;
    MachineRegisterInfo &MRI;
    const TargetInstrInfo *TII;
    const TargetRegisterInfo *TRI;
    const RegisterClassInfo &RegClassInfo;

    /// AllocatableSet - The set of allocatable registers.
    /// We'll be ignoring anti-dependencies on non-allocatable registers,
    /// because they may not be safe to break.
    const BitVector AllocatableSet;

    /// Classes - For live regs that are only used in one register class in a
    /// live range, the register class. If the register is not live, the
    /// corresponding value is null. If the register is live but used in
    /// multiple register classes, the corresponding value is -1 casted to a
    /// pointer.
    MISTD::vector<const TargetRegisterClass*> Classes;

    /// RegRefs - Map registers to all their references within a live range.
    MISTD::multimap<unsigned, MachineOperand *> RegRefs;
    typedef MISTD::multimap<unsigned, MachineOperand *>::const_iterator
      RegRefIter;

    /// KillIndices - The index of the most recent kill (proceding bottom-up),
    /// or ~0u if the register is not live.
    MISTD::vector<unsigned> KillIndices;

    /// DefIndices - The index of the most recent complete def (proceding bottom
    /// up), or ~0u if the register is live.
    MISTD::vector<unsigned> DefIndices;

    /// KeepRegs - A set of registers which are live and cannot be changed to
    /// break anti-dependencies.
    BitVector KeepRegs;

  public:
    CriticalAntiDepBreaker(MachineFunction& MFi, const RegisterClassInfo&);
    ~CriticalAntiDepBreaker();

    /// Start - Initialize anti-dep breaking for a new basic block.
    void StartBlock(MachineBasicBlock *BB);

    /// BreakAntiDependencies - Identifiy anti-dependencies along the critical
    /// path
    /// of the ScheduleDAG and break them by renaming registers.
    ///
    unsigned BreakAntiDependencies(const MISTD::vector<SUnit>& SUnits,
                                   MachineBasicBlock::iterator Begin,
                                   MachineBasicBlock::iterator End,
                                   unsigned InsertPosIndex,
                                   DbgValueVector &DbgValues);

    /// Observe - Update liveness information to account for the current
    /// instruction, which will not be scheduled.
    ///
    void Observe(MachineInstr *MI, unsigned Count, unsigned InsertPosIndex);

    /// Finish - Finish anti-dep breaking for a basic block.
    void FinishBlock();

  private:
    void PrescanInstruction(MachineInstr *MI);
    void ScanInstruction(MachineInstr *MI, unsigned Count);
    bool isNewRegClobberedByRefs(RegRefIter RegRefBegin,
                                 RegRefIter RegRefEnd,
                                 unsigned NewReg);
    unsigned findSuitableFreeRegister(RegRefIter RegRefBegin,
                                      RegRefIter RegRefEnd,
                                      unsigned AntiDepReg,
                                      unsigned LastNewReg,
                                      const TargetRegisterClass *RC,
                                      SmallVectorImpl<unsigned> &Forbid);
  };
}

#endif
