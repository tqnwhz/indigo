#pragma once

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "../mir/ty.hpp"
#include "../prelude/prelude.hpp"

namespace arm {
bool is_valid_immediate(uint32_t val);

enum class RegisterShiftKind : uint8_t { Asr, Lsl, Lsr, Ror, Rrx };
void display_shift(std::ostream& o, RegisterShiftKind shift);
enum class MemoryAccessKind : uint8_t { None, PreIndex, PostIndex };
enum class RegisterKind {
  GeneralPurpose,
  DoubleVector,
  QuadVector,
  VirtualGeneralPurpose,
  VirtualDoubleVector,
  VirtualQuadVector
};

// Register type. Any value larger or equal to 64 is considered as a virtual
// register, and any value larger or equal to 2^31 is considered as a virtual
// vector register.
using Reg = uint32_t;

/// Frame pointer (base pointer)
const uint32_t REG_FP = 11;
/// Stack pointer
const uint32_t REG_SP = 13;
/// Link register
const uint32_t REG_LR = 14;
/// Program counter
const uint32_t REG_PC = 15;

const uint32_t REG_GP_START = 0;
const uint32_t REG_DOUBLE_START = 16;
const uint32_t REG_QUAD_START = 48;
const uint32_t REG_V_GP_START = 64;
const uint32_t REG_V_DOUBLE_START = 1 << 31;
const uint32_t REG_V_QUAD_START = 3 << 30;

const std::vector<Reg> TEMP_REGS = {0, 1, 2, 3, 12, REG_LR};
const std::vector<Reg> GLOB_REGS = {4, 5, 6, 7, 8, 9, 10};

inline bool is_virtual_register(Reg r) { return r >= 64; }
RegisterKind register_type(Reg r);
uint32_t register_num(Reg r);
void display_reg_name(std::ostream& o, Reg r);

Reg make_register(RegisterKind ty, uint32_t num);

enum class ConstType { Word, AsciZ };

class ConstValue
    : public std::variant<uint32_t, std::vector<uint32_t>, std::string>,
      public prelude::Displayable {
 public:
  ConstValue() {}
  ConstValue(uint32_t x)
      : std::variant<uint32_t, std::vector<uint32_t>, std::string>(x),
        ty(ConstType::Word) {}
  ConstValue(std::vector<uint32_t> x, std::optional<int> len = std::nullopt)
      : std::variant<uint32_t, std::vector<uint32_t>, std::string>(x),
        len(len),
        ty(ConstType::Word) {}
  ConstValue(std::string x)
      : std::variant<uint32_t, std::vector<uint32_t>, std::string>(x),
        ty(ConstType::AsciZ) {}
  ConstValue(std::string x, ConstType ty)
      : std::variant<uint32_t, std::vector<uint32_t>, std::string>(x), ty(ty) {}

  ConstType ty;
  std::optional<int> len;

  size_t size();
  virtual void display(std::ostream& o) const;
  virtual ~ConstValue() {}
};

struct RegisterOperand : public prelude::Displayable {
  RegisterOperand() : RegisterOperand(0) {}

  RegisterOperand(Reg reg) : RegisterOperand(reg, RegisterShiftKind::Lsl, 0) {}

  RegisterOperand(Reg reg, RegisterShiftKind shift, uint8_t shift_amount)
      : reg(reg), shift(shift), shift_amount(shift_amount) {}

  Reg reg;
  RegisterShiftKind shift;
  uint8_t shift_amount;

  void replace_reg_if_virtual(Reg reg) {
    if (is_virtual_register(this->reg)) this->reg = reg;
  }
  virtual void display(std::ostream& o) const;
  bool operator==(const RegisterOperand& other) const {
    return reg == other.reg && shift == other.shift &&
           shift_amount == other.shift_amount;
  };
  bool operator==(const Reg& other) const {
    return reg == other && shift == RegisterShiftKind::Lsl && shift_amount == 0;
  };
};

struct MemoryOperand : public prelude::Displayable {
  MemoryOperand(Reg r1, int16_t offset = 0,
                MemoryAccessKind kind = MemoryAccessKind::None)
      : kind(kind), r1(r1), offset(offset) {}

  MemoryOperand(Reg r1, RegisterOperand rm, bool neg_rm = false,
                MemoryAccessKind kind = MemoryAccessKind::None)
      : kind(kind), r1(r1), offset(rm), neg_rm(neg_rm) {}

  // Pre- or post-indexed memory access
  MemoryAccessKind kind;
  // Base register
  Reg r1;
  // Offset register
  std::variant<RegisterOperand, int16_t> offset;
  // is offset negative?
  bool neg_rm;

  std::pair<Reg, std::optional<Reg>> is_virtual() {
    auto x = std::get_if<RegisterOperand>(&offset);
    if (x)
      return {r1, x->reg};
    else {
      return {r1, {}};
    }
  }
  void replace_reg_if_virtual(Reg r1_, Reg offset_) {
    if (is_virtual_register(r1)) r1 = r1_;
    auto x = std::get_if<RegisterOperand>(&offset);
    if (x) {
      x->replace_reg_if_virtual(offset_);
    }
  }
  virtual void display(std::ostream& o) const;
  bool operator==(const MemoryOperand& other) const {
    auto r =
        (kind == other.kind) && (r1 == other.r1) && (offset == other.offset);
    if (!r) return r;
    if (offset.index() == other.offset.index() && offset.index() == 0)
      return r && (neg_rm == other.neg_rm);
    else
      return r;
  }
};

struct NoOperand {};

/// Flexible operand as specified in ARM datasheet
class Operand2 : public std::variant<RegisterOperand, int32_t>,
                 public prelude::Displayable {
 public:
  Operand2(std::variant<RegisterOperand, int32_t>& x)
      : std::variant<RegisterOperand, int32_t>(x) {}
  Operand2(std::variant<RegisterOperand, int32_t> x)
      : std::variant<RegisterOperand, int32_t>(x) {}
  Operand2(RegisterOperand r) : std::variant<RegisterOperand, int32_t>(r) {}
  Operand2(int32_t i) : std::variant<RegisterOperand, int32_t>(i) {}

  Reg get_reg() { return std::get_if<RegisterOperand>(this)->reg; }
  bool is_reg() { return std::get_if<RegisterOperand>(this); }
  bool is_virtual() {
    auto ip = std::get_if<RegisterOperand>(this);
    return ip && is_virtual_register(ip->reg);
  }
  void replace_reg_if_virtual(Reg reg) {
    if (auto ip = std::get_if<RegisterOperand>(this)) {
      ip->replace_reg_if_virtual(reg);
    }
  }
  virtual void display(std::ostream& o) const;
  bool operator==(const Operand2& other) const {
    auto this_ =
        dynamic_cast<const std::variant<RegisterOperand, int32_t>&>(*this);
    auto other_ =
        dynamic_cast<const std::variant<RegisterOperand, int32_t>&>(other);
    return this_ == other_;
  }
  bool operator==(const int32_t& other) const {
    if (auto ip = std::get_if<int32_t>(this)) {
      return *ip == other;
    } else {
      return false;
    }
  }
  friend bool operator==(const Operand2& lhs, const Reg& other) {
    if (auto ip = std::get_if<RegisterOperand>(&lhs)) {
      return *ip == other;
    } else {
      return false;
    }
  }
  friend bool operator==(const Reg& lhs, const Operand2& other) {
    if (auto ip = std::get_if<RegisterOperand>(&other)) {
      return *ip == lhs;
    } else {
      return false;
    }
  }
  bool operator==(const RegisterOperand& other) const {
    if (auto ip = std::get_if<RegisterOperand>(this)) {
      return *ip == other;
    } else {
      return false;
    }
  }
};

typedef std::string Label;

enum class OpCode {
  Nop,
  // Branches

  // Branch
  B,
  // Branch and link (call)
  Bl,
  // Branch exchange (return)
  Bx,
  // Compare and branch if zero
  Cbz,
  // Compare and branch if not zero
  Cbnz,
  // // If then
  // IT,

  // Arithmetic

  // Move
  Mov,
  // Move top 16 bits
  MovT,
  // Move not
  Mvn,
  // Add
  Add,
  // Subtract
  Sub,
  // Reverse subtract
  Rsb,
  // Multiply
  Mul,
  // Signed Most Significant Word Multiply
  SMMul,
  // Multiply and add
  Mla,
  // Signed Most Significant Word Multiply and add
  SMMla,
  // Signed Divide
  SDiv,
  // And
  And,
  // Or
  Orr,
  // Xor
  Eor,
  // Bit clear
  Bic,
  // Logical Shift Left
  Lsl,
  // Logical Shift Right
  Lsr,
  // Arithmetic Shift Right
  Asr,
  // Compare
  Cmp,
  // Compare Minus
  Cmn,

  // Memory

  // Load Register
  LdR,
  // Load Multiple Register
  LdM,
  // Store Register
  StR,
  // Store Multiple Register
  StM,
  // Push
  Push,
  // Pop
  Pop,

  // Label (pseudo-instruction)
  _Label,
  // Control Comment (pseudo-instruction)
  _Ctrl,
  // Mod (pseudo-instruction)
  _Mod
};

void display_op(OpCode op, std::ostream& o);

enum class ConditionCode : uint8_t {
  Equal,
  NotEqual,
  CarrySet,
  UnsignedGe,
  CarryClear,
  UnsignedLt,
  MinusOrNegative,
  PositiveOrZero,
  Overflow,
  NoOverflow,
  UnsignedGt,
  UnsignedLe,
  Ge,
  Lt,
  Gt,
  Le,
  Always,
};

void display_cond(ConditionCode cond, std::ostream& o);
// Invert Condition code (result = NOT cond)
ConditionCode invert_cond(ConditionCode cond);
// Reverse Consition code (a cond b -> b result a)
ConditionCode reverse_cond(ConditionCode cond);

struct Inst : public prelude::Displayable {
  Inst(OpCode op, ConditionCode cond = ConditionCode::Always)
      : op(op), cond(cond){};

  OpCode op;
  ConditionCode cond;

  virtual void display(std::ostream& o) const = 0;
  virtual ~Inst() {}
};

/// Pure Instruction that does not use any operand
///
/// Valid opcode: Nop, Bx
struct PureInst final : public Inst {
  PureInst(OpCode op, ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond) {}

  virtual void display(std::ostream& o) const;
  virtual ~PureInst() {}
};

/// 4-operand arithmetic instruction
///
/// Valid opcode: Mla, SMMla
struct Arith4Inst final : public Inst {
  Arith4Inst(OpCode op, Reg rd, Reg r1, Reg r2, Reg r3,
             ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), rd(rd), r1(r1), r2(r2), r3(r3) {}

  Reg rd;
  Reg r1;
  Reg r2;
  Reg r3;

  virtual void display(std::ostream& o) const;
  virtual ~Arith4Inst() {}
};

/// 3-operand arithmetic instruction
///
/// Valid opcode: Add, Sub, Rsb, Mul, SDiv, And, Orr, Eor, Lsl, Lsr, Asr
struct Arith3Inst final : public Inst {
  Arith3Inst(OpCode op, Reg rd, Reg r1, Operand2 r2,
             ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), rd(rd), r1(r1), r2(r2) {}

  Reg rd;
  Reg r1;
  Operand2 r2;

  virtual void display(std::ostream& o) const;
  virtual ~Arith3Inst() {}
};

/// 2-operand arithmetic instruction
///
/// Valid Opcode: Mov, Cmp, Cmn
struct Arith2Inst final : public Inst {
  Arith2Inst(OpCode op, Reg r1, Operand2 r2,
             ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), r1(r1), r2(r2) {}

  Reg r1;
  Operand2 r2;

  virtual void display(std::ostream& o) const;
  virtual ~Arith2Inst() {}
};

/// Branch instruction
///
/// Valid opcode: B, Bl
struct BrInst final : public Inst {
  BrInst(OpCode op, Label l, ConditionCode c = ConditionCode::Always)
      : l(l), Inst(op, c) {}
  BrInst(OpCode op, Label l, int param_size,
         ConditionCode c = ConditionCode::Always)
      : l(l), param_cnt(param_size), Inst(op, c) {}
  Label l;
  int param_cnt = 0;

  virtual void display(std::ostream& o) const;
  virtual ~BrInst() {}
};

/// Load and store instruction
///
/// Valid opcode: LdR, StR
struct LoadStoreInst final : public Inst {
  LoadStoreInst(OpCode op, Reg rd, MemoryOperand mem,
                ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), rd(rd), mem(mem) {}
  LoadStoreInst(OpCode op, Reg rd, std::string mem,
                ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), rd(rd), mem(mem) {}

  Reg rd;
  std::variant<std::string, MemoryOperand> mem;

  virtual void display(std::ostream& o) const;
  virtual ~LoadStoreInst() {}
};

/// Multi-target load and store instruction
///
/// Valid opcode: LdM, StM
struct MultLoadStoreInst final : public Inst {
  MultLoadStoreInst(OpCode op, Reg rn, std::set<Reg> rd,
                    ConditionCode cond = ConditionCode::Always)
      : rd(std::move(rd)), rn(rn), Inst(op, cond) {}
  std::set<Reg> rd;
  Reg rn;

  virtual void display(std::ostream& o) const;
  virtual ~MultLoadStoreInst() {}
};

/// Push and pop instruction
///
/// Valid opcode: Push, Pop
struct PushPopInst final : public Inst {
  PushPopInst(OpCode op, std::set<Reg> regs,
              ConditionCode cond = ConditionCode::Always)
      : Inst(op, cond), regs(regs) {}

  std::set<Reg> regs;

  virtual void display(std::ostream& o) const;
  virtual ~PushPopInst() {}
};

/// Label pseudo-instruction
///
/// Valid opcode: _Label
struct LabelInst final : public Inst {
  LabelInst(Label label)
      : Inst(OpCode::_Label, ConditionCode::Always), label(label) {}

  Label label;

  virtual void display(std::ostream& o) const;
  virtual ~LabelInst() {}
};

/// Control pseudo-instruction
///
/// Valid opcode: _Ctrl
struct CtrlInst final : public Inst {
  CtrlInst(std::string key, std::any val, bool is_asm_option = false)
      : Inst(OpCode::_Ctrl, ConditionCode::Always),
        key(std::move(key)),
        val(std::move(val)),
        is_asm_option(is_asm_option) {}

  bool is_asm_option;
  std::string key;
  std::any val;

  virtual void display(std::ostream& o) const;
  virtual ~CtrlInst() {}
};

const std::string STACK_OFFSET_CTRL = "offset_stack";
using StackOffsetTy = int32_t;

struct Function final : public prelude::Displayable {
  Function(std::string& name, std::shared_ptr<mir::types::FunctionTy> ty,
           std::vector<std::unique_ptr<Inst>>&& inst,
           std::map<std::string, ConstValue>&& local_const, uint32_t stack_size)
      : name(name),
        ty(ty),
        inst(std::move(inst)),
        local_const(std::move(local_const)),
        stack_size(stack_size) {}
  std::string name;
  std::vector<std::unique_ptr<Inst>> inst;
  std::map<std::string, ConstValue> local_const;
  uint32_t stack_size;

  std::shared_ptr<mir::types::FunctionTy> ty;

  void display(std::ostream& o) const override;
  Function(Function&&) = default;
  Function(const Function&) = delete;
};

struct ArmCode final : public prelude::Displayable {
  std::vector<std::unique_ptr<Function>> functions;
  std::map<std::string, ConstValue> consts;

  void display(std::ostream& o) const override;
};

}  // namespace arm
