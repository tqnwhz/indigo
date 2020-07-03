#pragma once

#include <memory>
#include <vector>

#include "../mir/mir.hpp"

namespace front::fake {
using std::move;
using std::nullopt;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

using mir::types::ArrayTy;
using mir::types::FunctionTy;
using mir::types::IntTy;
using mir::types::PtrTy;
using mir::types::SharedTyPtr;

using mir::inst::AssignInst;
using mir::inst::Inst;
using mir::inst::JumpInstruction;
using mir::inst::JumpInstructionKind;
using mir::inst::JumpKind;
using mir::inst::LoadInst;
using mir::inst::MirFunction;
using mir::inst::MirPackage;
using mir::inst::Op;
using mir::inst::OpInst;
using mir::inst::PtrOffsetInst;
using mir::inst::RefInst;
using mir::inst::StoreInst;
using mir::inst::Value;
using mir::inst::Variable;
using mir::inst::VarId;

class FakeGenerator {
 public:
  shared_ptr<MirPackage> _package;

  void fakeMirGenerator1() {
    int i;
    Value value1;
    Value value2;

    _package = shared_ptr<MirPackage>(new MirPackage());

    _package->functions[0] = MirFunction(
        "main", shared_ptr<FunctionTy>(new FunctionTy(
                    shared_ptr<IntTy>(new IntTy()), vector<SharedTyPtr>())));

    _package->functions[0].variables[1] =
        Variable(SharedTyPtr(new ArrayTy(SharedTyPtr(new IntTy), 10)), true);
    _package->functions[0].variables[2] =
        Variable(SharedTyPtr(new IntTy()), true);
    _package->functions[0].variables[3] =
        Variable(SharedTyPtr(new IntTy()), true);
    _package->functions[0].variables[4] =
        Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), false, true);
    _package->functions[0].variables[5] =
        Variable(SharedTyPtr(new IntTy()), false, true);
    _package->functions[0].variables[6] =
        Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), false, true);

    _package->functions[0].basic_blks.insert({0, mir::inst::BasicBlk(0)});
    _package->functions[0].basic_blks.insert({1, mir::inst::BasicBlk(1)});
    _package->functions[0].basic_blks.insert({2, mir::inst::BasicBlk(2)});
    _package->functions[0].basic_blks.insert({3, mir::inst::BasicBlk(3)});

    _package->functions[0].basic_blks.at(0).jump = JumpInstruction(
        JumpInstructionKind::Br, 3, -3, nullopt, JumpKind::Undefined);
    _package->functions[0].basic_blks.at(0).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(4), VarId(1)))));

    for (i = 0; i < 10; i++) {
      value1.emplace<0>(i + 1);
      _package->functions[0].basic_blks.at(0).inst.push_back(
          move(unique_ptr<Inst>(new StoreInst(value1, VarId(4)))));
      if (i < 9) {
        _package->functions[0].basic_blks.at(0).inst.push_back(move(
            unique_ptr<Inst>(new PtrOffsetInst(VarId(4), VarId(4), value1))));
      }
    }
    value1.emplace<0>(0);
    _package->functions[0].basic_blks.at(0).inst.push_back(
        move(unique_ptr<Inst>(new AssignInst(VarId(2), value1))));

    _package->functions[0].basic_blks.at(3).id = 3;
    _package->functions[0].basic_blks.at(3).jump = JumpInstruction(
        JumpInstructionKind::BrCond, 1, 2, VarId(5), JumpKind::Loop);
    _package->functions[0].basic_blks.at(3).preceding.insert(0);
    _package->functions[0].basic_blks.at(3).preceding.insert(1);
    value1.emplace<0>(9);
    value2.emplace<1>(VarId(2));
    _package->functions[0].basic_blks.at(3).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(5), value1, value2, Op::Sub))));
    value1.emplace<1>(VarId(2));
    value2.emplace<1>(VarId(5));
    _package->functions[0].basic_blks.at(3).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(5), value1, value2, Op::Lt))));

    _package->functions[0].basic_blks.at(1).id = 1;
    _package->functions[0].basic_blks.at(1).jump = JumpInstruction(
        JumpInstructionKind::Br, 3, -1, nullopt, JumpKind::Loop);
    _package->functions[0].basic_blks.at(1).preceding.insert(3);
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(4), VarId(1)))));
    value1.emplace<1>(VarId(2));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new PtrOffsetInst(VarId(4), VarId(4), value1))));
    value1.emplace<1>(VarId(4));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new LoadInst(value1, VarId(3)))));
    value1.emplace<0>(9);
    value2.emplace<1>(VarId(2));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(5), value1, value2, Op::Sub))));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(6), VarId(1)))));
    value1.emplace<1>(VarId(2));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new PtrOffsetInst(VarId(6), VarId(6), value1))));
    value1.emplace<1>(VarId(6));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new LoadInst(value1, VarId(5)))));
    value1.emplace<1>(VarId(4));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new StoreInst(value1, VarId(5)))));
    value1.emplace<1>(VarId(6));
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new StoreInst(value1, VarId(3)))));
    value1.emplace<1>(VarId(2));
    value2.emplace<0>(1);
    _package->functions[0].basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(2), value1, value2, Op::Add))));

    _package->functions[0].basic_blks.at(2).id = 2;
    _package->functions[0].basic_blks.at(2).jump = JumpInstruction(
        JumpInstructionKind::Return, -1, -1, nullopt, JumpKind::Undefined);
    _package->functions[0].basic_blks.at(2).preceding.insert(3);
  }
};
}  // namespace front::fake
