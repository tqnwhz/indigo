#include "ir_generator.hpp"

using namespace front::irGenerator;

irGenerator::irGenerator()
{
    _nowLabelId = 0;
    _nowFuncId = 0;
    _nowGlobalValueId = 0;
    _nowLocalValueId = 1;

    mir::inst::MirFunction func;
    func.name = "@@__Compiler__GlobalInitFunc__Auto__Generated__@@";
    _package.functions[_nowFuncId] = func;
    _funcNameToId[func.name] = _nowFuncId;
    _funcStack.push_back(_nowFuncId);
    _nowFuncId++;
}

LabelId irGenerator::getNewLabelId()
{
    return _nowLabelId++;
}

LabelId irGenerator::getNewTmpValueId()
{
    if (_funcStack.back() == _GlobalInitFuncId)
    {
        _package.global_values[_nowGlobalValueId] = GlobalValue(std::to_string(_nowGlobalValueId));
        _globalValueNameToId[std::to_string(_nowGlobalValueId)] = _nowGlobalValueId;
        return _nowGlobalValueId++;
    }
    else
    {
        // TODO: deal the type tmp value
        _package.functions[_funcStack.back()].variables[_nowLocalValueId] = Variable(SharedTyPtr(new IntTy()), false, true);
        _localValueNameToId[std::to_string(_nowLocalValueId)] = _nowLocalValueId;
        return _nowLocalValueId++;
    }
}

void irGenerator::pushWhile(WhileLabels wl)
{
    _whileStack.push_back(wl);
}

WhileLabels irGenerator::checkWhile()
{
    return _whileStack.back();
}

void irGenerator::popWhile()
{
    _whileStack.pop_back();
}

void irGenerator::ir_op(LeftVal dest, RightVal op1, RightVal op2, mir::inst::Op op)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> op1Value;
    shared_ptr<Value> op2Value;
    shared_ptr<mir::inst::OpInst> opInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    op1Value = rightValueToValue(op1);
    op2Value = rightValueToValue(op2);
     
    opInst = shared_ptr<mir::inst::OpInst>(new mir::inst::OpInst(*destVarId, *op1Value, *op2Value, op));

    _funcIdToInstructions[_funcStack.back()].push_back(opInst);
}


void irGenerator::ir_declare_value(string name, symbol::SymbolKind kind, int len)
{
    if (_funcStack.back() == _GlobalInitFuncId)
    {
        GlobalValue globalValue = GlobalValue(name);
        _package.global_values[_nowGlobalValueId] = globalValue;
        _globalValueNameToId[get<string>(globalValue)] = _nowGlobalValueId;
        _nowGlobalValueId++;
    }
    else
    {
        SharedTyPtr ty;
        switch (kind)
        {
        case front::symbol::SymbolKind::INT:
            ty = SharedTyPtr(new IntTy());
            break;
        case front::symbol::SymbolKind::Array:
            ty = SharedTyPtr(new ArrayTy(SharedTyPtr(new IntTy()), len));
            break;
        case front::symbol::SymbolKind::Ptr:
            ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
            break;
        default:
            break;
        }
        Variable variable(ty, true, false);
        _package.functions[_funcStack.back()].variables[_nowLocalValueId] = variable;
        _localValueNameToId[name] = _nowLocalValueId;
        _nowLocalValueId++;
    }
}

void irGenerator::ir_declare_param(string name, symbol::SymbolKind kind)
{
    SharedTyPtr ty;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
    {
        ir_declare_value(name, kind);
        ty = SharedTyPtr(new IntTy());
        break;
    }
    case front::symbol::SymbolKind::Ptr:
    {
        ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
        Variable variable(ty, true, false);
        _package.functions[_funcStack.back()].variables[_nowLocalValueId] = variable;
        _localValueNameToId[name] = _nowLocalValueId;
        _nowLocalValueId++;
        break;
    }
    default:
        break;
    }

    _package.functions[_funcStack.back()].type->params.push_back(ty);
}

void irGenerator::ir_declare_function(string name, symbol::SymbolKind kind)
{
    mir::inst::MirFunction func;
    shared_ptr<FunctionTy> type;
    SharedTyPtr ret;
    std::vector<SharedTyPtr> params;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
        ret = SharedTyPtr(new IntTy());
        break;
    case front::symbol::SymbolKind::VOID:
        ret = SharedTyPtr(new VoidTy());
        break;
    default:
        break;
    }
    type = shared_ptr<FunctionTy>(new FunctionTy(ret, params));
    func = mir::inst::MirFunction(name, type);

    _funcNameToId[func.name] = _nowFuncId;
    _package.functions[_nowFuncId] = func;
    _funcStack.push_back(_nowFuncId);
    _nowFuncId++;

    _package.functions[_nowFuncId].variables[_VoidVarId] = Variable(SharedTyPtr(new VoidTy()), false, false);

    _nowLocalValueId = 1;
    _localValueNameToId.clear();
}

void irGenerator::ir_leave_function()
{
    _funcStack.pop_back();
}


void irGenerator::ir_ref(LeftVal dest, LeftVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<VarId> srcVarId;
    shared_ptr<mir::inst::Inst> refInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));

    refInst = shared_ptr<mir::inst::RefInst>(new mir::inst::RefInst(*destVarId, *srcVarId));
     
    _funcIdToInstructions[_funcStack.back()].push_back(refInst);
}

void irGenerator::ir_offset(LeftVal dest, LeftVal ptr, RightVal offset)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<VarId> ptrVarId;
    shared_ptr<Value> offsetValue;
    shared_ptr<mir::inst::PtrOffsetInst> offInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    ptrVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(ptr)));
    offsetValue = rightValueToValue(offset);

    offInst = shared_ptr<mir::inst::PtrOffsetInst>(new mir::inst::PtrOffsetInst(*destVarId, *ptrVarId, *offsetValue));

    _funcIdToInstructions[_funcStack.back()].push_back(offInst);
}

void irGenerator::ir_assign(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::AssignInst> assginInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    assginInst = shared_ptr<mir::inst::AssignInst>(new mir::inst::AssignInst(*destVarId, *srcValue));
}

void irGenerator::ir_load(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::LoadInst> loadInt;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    loadInt = shared_ptr<mir::inst::LoadInst>(new mir::inst::LoadInst(*srcValue, *destVarId));

    _funcIdToInstructions[_funcStack.back()].push_back(loadInt);
}

void irGenerator::ir_store(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::StoreInst> storeInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    storeInst = shared_ptr<mir::inst::StoreInst>(new mir::inst::StoreInst(*srcValue, *destVarId));

    _funcIdToInstructions[_funcStack.back()].push_back(storeInst);
}

void irGenerator::ir_function_call(string retName, symbol::SymbolKind kind, string funcName, std::vector<RightVal> params)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<VarId> funcVarId;
    std::vector<Value> paramValues;
    shared_ptr<mir::inst::CallInst> callInst;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
        destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(retName)));
        break;
    case front::symbol::SymbolKind::VOID:
        destVarId = shared_ptr<VarId>(new VarId(_VoidVarId));
        break;
    default:
        break;
    }

    funcVarId = shared_ptr<VarId>(new VarId(_funcNameToId[funcName]));

    for (auto var : params)
    {
        paramValues.push_back(*rightValueToValue(var));
    }

    callInst = shared_ptr<mir::inst::CallInst>(new mir::inst::CallInst(*destVarId, *funcVarId, paramValues));

    _funcIdToInstructions[_funcStack.back()].push_back(callInst);
}

void irGenerator::ir_jump(mir::inst::JumpInstructionKind kind, LabelId bbTrue, LabelId bbFalse,
    std::optional<string> condRetName, mir::inst::JumpKind jumpKind)
{
    std::optional<VarId> crn;
    shared_ptr<mir::inst::JumpInstruction> jumpInst;
    
    crn = VarId(LeftValueToLabelId(condRetName.value()));

    jumpInst = shared_ptr<mir::inst::JumpInstruction>(new mir::inst::JumpInstruction(kind, bbTrue, bbFalse, crn, jumpKind));

    _funcIdToInstructions[_funcStack.back()].push_back(jumpInst);
}

void irGenerator::ir_label(LabelId label)
{
    shared_ptr<JumpLabelId> jumpLabelId;

    jumpLabelId = shared_ptr<JumpLabelId>(new JumpLabelId(label));

    _funcIdToInstructions[_funcStack.back()].push_back(jumpLabelId);
}

LabelId irGenerator::LeftValueToLabelId(LeftVal leftVal)
{
    LabelId id;
    switch (leftVal.index())
    {
    case 0:
         id = get<LabelId>(leftVal);
        break;
    case 1:
        id = nameToLabelId(get<string>(leftVal));
        break;
    default:
        break;
    }
    return id;
}

shared_ptr<Value> irGenerator::rightValueToValue(RightVal& rightValue)
{
    shared_ptr<Value> value;
    value = shared_ptr<Value>(new Value());

    switch (rightValue.index())
    {
    case 0:
        value->emplace<int32_t>(get<0>(rightValue));
        break;
    case 2:
        value->emplace<VarId>(VarId(nameToLabelId(get<2>(rightValue))));
        break;
    default:
        break;
    }

    return value;
}


LabelId irGenerator::nameToLabelId(string name)
{
    LabelId id = -1;
    if (_localValueNameToId.count(name) == 1)
    {
        id = _localValueNameToId[name];
    }
    else if (_localValueNameToId.count(name) == 1)
    {
        id = _localValueNameToId[name];
    }
    return id;
}