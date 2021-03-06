#include "mir.hpp"

#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>

namespace mir::inst {

void display_op(std::ostream& o, Op val) {
  switch (val) {
    case Op::Add:
      o << "+";
      break;
    case Op::Sub:
      o << "-";
      break;
    case Op::Mul:
      o << "*";
      break;
    case Op::MulSh:
      o << "MulSh";
      break;
    case Op::Div:
      o << "/";
      break;
    case Op::Rem:
      o << "%";
      break;
    case Op::Gt:
      o << ">";
      break;
    case Op::Lt:
      o << "<";
      break;
    case Op::Gte:
      o << ">=";
      break;
    case Op::Lte:
      o << "<=";
      break;
    case Op::Eq:
      o << "==";
      break;
    case Op::Neq:
      o << "!=";
      break;
    case Op::And:
      o << "&";
      break;
    case Op::Or:
      o << "|";
      break;
    case Op::Xor:
      o << "^";
      break;
    case Op::Not:
      o << "!";
      break;
    case Op::Shl:
      o << "<<";
      break;
    case Op::Shr:
      o << ">>>";
      break;
    case Op::ShrA:
      o << ">>";
      break;
  }
}

void display_op(std::ostream& o, OpAcc val) {
  switch (val) {
    case OpAcc::MulAdd:
      o << "MulAdd";
      break;
    case OpAcc::MulShAdd:
      o << "MulShAdd";
      break;
  }
}

void Variable::display(std::ostream& o) const {
  o << *ty;
  if (is_memory_var) {
    o << " memory (" << *(type()) << ")";
  }
  if (is_temp_var) {
    o << " temp";
  }
  o << ", priority: " << priority;
}

// void VarId::display(std::ostream& o) const { o << "$" << id; }

void Value::display(std::ostream& o) const {
  if (auto x = std::get_if<VarId>(this)) {
    if (has_shift()) {
      o << "(";
    }
    o << *x;
    if (has_shift()) {
      o << ", ";
      arm::display_shift(o, shift);
      o << " " << (unsigned int)shift_amount << ")";
    }
  } else if (auto x = std::get_if<int32_t>(this)) {
    o << *x;
  }
}

void AssignInst::display(std::ostream& o) const { o << dest << " = " << src; }

void OpInst::display(std::ostream& o) const {
  o << dest << " = " << lhs << " ";
  display_op(o, op);
  o << " " << rhs;
}

void OpAccInst::display(std::ostream& o) const {
  o << dest << " = ";
  display_op(o, op);
  o << " " << lhs << ", " << rhs << ", " << acc;
}

void CallInst::display(std::ostream& o) const {
  o << dest << " = " << func << "(";
  for (size_t i = 0; i < params.size(); i++) {
    if (i != 0) o << ", ";
    o << params[i];
  }
  o << ")";
}

void RefInst::display(std::ostream& o) const {
  o << dest << " = &";
  if (auto x = std::get_if<VarId>(&val)) {
    o << *x;
  } else if (auto x = std::get_if<std::string>(&val)) {
    o << "@" << *x;
  }
}

void LoadInst::display(std::ostream& o) const {
  o << dest << " = load " << src;
}

void LoadOffsetInst::display(std::ostream& o) const {
  o << dest << " = load "
    << "[ " << src << " , " << offset << " ]";
}

void StoreInst::display(std::ostream& o) const {
  o << "store " << val << " to " << dest;
}

void StoreOffsetInst::display(std::ostream& o) const {
  o << "store " << val << " to "
    << "[ " << dest << " , " << offset << " ]";
}

void PtrOffsetInst::display(std::ostream& o) const {
  o << dest << " = offset " << ptr << " by " << offset;
}

void PhiInst::display(std::ostream& o) const {
  o << dest << " = phi [";
  for (size_t i = 0; i < vars.size(); i++) {
    if (i != 0) o << ", ";
    o << vars[i];
  }
  o << "]";
}

void JumpInstruction::display(std::ostream& o) const {
  switch (kind) {
    case JumpInstructionKind::Undefined:
      o << "undefined_jump!";
      break;

    case JumpInstructionKind::Br:
      o << "br " << bb_true;
      break;

    case JumpInstructionKind::BrCond:
      o << "br " << cond_or_ret.value() << ", " << bb_true << ", " << bb_false;
      if (jump_kind == JumpKind::Branch)
        o << " if_branch";
      else if (jump_kind == JumpKind::Loop)
        o << " loop";

      break;

    case JumpInstructionKind::Return:
      if (cond_or_ret.has_value())
        o << "ret " << cond_or_ret.value();
      else
        o << "ret void";
      break;

    case JumpInstructionKind::Unreachable:
      o << "unreachable!";
      break;

    default:
      break;
  }
}

void BasicBlk::display(std::ostream& o) const {
  o << "bb" << id << ":";

  // print preceding
  o << "    // preceding: ";
  for (auto i = preceding.begin(); i != preceding.end(); i++) {
    if (i != preceding.begin()) o << ", ";
    o << *i;
  }
  o << std::endl;

  for (auto i = inst.begin(); i != inst.end(); i++) {
    o << '\t' << **i << std::endl;
  }

  o << "\t" << jump << std::endl;
}

void MirFunction::display(std::ostream& o) const {
  auto& return_ty = type->ret;
  auto& params = type->params;
  if (type->is_extern) {
    o << "extern fn " << name << "(";
    for (auto i = params.begin(); i != params.end(); i++) {
      if (i != params.begin()) o << ", ";
      o << **i;
    }
    o << ") -> " << *return_ty << ";" << std::endl;
  } else {
    o << "fn " << name << "(";
    for (auto i = params.begin(); i != params.end(); i++) {
      if (i != params.begin()) o << ", ";
      o << **i;
    }
    o << ") -> " << *return_ty << " {" << std::endl;
    for (auto& i : variables) {
      o << "\t" << VarId(i.first) << ": " << i.second << std::endl;
      ;
    }
    for (auto i = basic_blks.begin(); i != basic_blks.end(); i++) {
      o << i->second;
    }
    o << "}" << std::endl;
  }
}

size_t MirFunction::variable_table_size() const {
  size_t size = 0;
  for (auto& v : variables) {
    auto s = v.second.ty->size();
    if (s) size += s.value();
  }
  return size;
}

void MirPackage::display(std::ostream& o) const {
  for (auto& f : functions) {
    o << f.second << std::endl;
  }
}

}  // namespace mir::inst

namespace mir::types {

void PtrTy::reduce_array() {
  if (this->item->kind() == TyKind::Array) {
    auto arr = std::dynamic_pointer_cast<ArrayTy>(this->item);
    this->item = arr->item;
  }
  // else noop
}

void IntTy::display(std::ostream& o) const { o << "i32"; }

void VoidTy::display(std::ostream& o) const { o << "void"; }

void PtrTy::display(std::ostream& o) const {
  item->display(o);
  o << "*";
}

void ArrayTy::display(std::ostream& o) const {
  o << "[";
  item->display(o);
  o << " x " << len << "]";
}

void FunctionTy::display(std::ostream& o) const {
  o << "Fn(";
  for (auto i = params.begin(); i != params.end(); i++) {
    if (i != params.begin()) o << ", ";
    o << **i;
  }
  o << ") -> " << *ret;
}

void RestParamTy::display(std::ostream& o) const { o << "...Rest"; }

std::shared_ptr<IntTy> new_int_ty() { return std::make_shared<IntTy>(); }

std::shared_ptr<VoidTy> new_void_ty() { return std::make_shared<VoidTy>(); }

std::shared_ptr<ArrayTy> new_array_ty(SharedTyPtr item, int len) {
  return std::make_shared<ArrayTy>(item, len);
}

std::shared_ptr<PtrTy> new_ptr_ty(SharedTyPtr item) {
  return std::make_shared<PtrTy>(item);
}

std::shared_ptr<FunctionTy> new_function_ty(SharedTyPtr ret,
                                            std::vector<SharedTyPtr> params,
                                            bool is_extern = false) {
  return std::make_shared<FunctionTy>(ret, params, is_extern);
}

}  // namespace mir::types
