#include <memory>
#include <string>
#include <vector>

#include "express_node.hpp"

#ifndef COMPILER_FRONT_SYMBOL_H_
#define COMPILER_FRONT_SYMBOL_H_

namespace front::symbol {

using std::static_pointer_cast;
using std::string;
using std::vector;

using front::express::SharedExNdPtr;

enum class SymbolKind { INT, VID, Array, Function, Ptr };

// Base class for symbols
class Symbol {
public:
  static int symbolId;

  string _name;
  int _layerNum;
  int id;

  static int getNoLayerNum() { return -1; }

  Symbol(string name, int layerNum) : _name(name), _layerNum(layerNum) {}
  virtual SymbolKind kind() const = 0;
  string getName() { return _name; }
  int getId() { return id; }
  int getLayerNum() { return _layerNum; }
  virtual ~Symbol(){};
};

typedef std::shared_ptr<Symbol> SharedSyPtr;

// Int type, variation, param, dimension or ret
class IntSymbol final : public Symbol {
public:
  bool _is_const;
  int _value;
  int _size = 4;

  static SharedSyPtr getHolderIntSymbol() {
    static SharedSyPtr holder(
        new IntSymbol("holder", Symbol::getNoLayerNum(), true));
    return holder;
  }

  IntSymbol(string name, int layer_num, bool is_const, int value = 0)
      : _is_const(is_const), _value(value), Symbol(name, layer_num) {
    id = Symbol::symbolId++;
  }
  virtual SymbolKind kind() const { return SymbolKind::INT; }
  bool isConst() const { return _is_const; }
  int getValue() const { return _value; }
  int getSize() const { return _size; }
  virtual ~IntSymbol() {}
};

// Void type, ret
class VoidSymbol final : public Symbol {
public:
  VoidSymbol() : Symbol("void", -1) {}
  virtual SymbolKind kind() const { return SymbolKind::VID; }

  virtual ~VoidSymbol() {}

private:
};

// Array type
class ArraySymbol final : public Symbol {
public:
  SharedSyPtr _item;
  bool _is_const;
  bool _is_param;
  int _size;
  vector<SharedExNdPtr> _dimensions;
  vector<SharedExNdPtr> _values;

  ArraySymbol(string name, SharedSyPtr item, int layer_num, bool is_const,
              bool is_param)
      : _item(item), _is_const(is_const), _is_param(is_param),
        Symbol(name, layer_num) {
    _size = -1;
    id = Symbol::symbolId++;
  }
  virtual SymbolKind kind() const { return SymbolKind::Array; }
  virtual ~ArraySymbol() {}
  bool isConst() const { return _is_const; }
  bool isParam() const { return _is_param; }
  void addDimension(SharedExNdPtr dimension) {
    _dimensions.push_back(dimension);
  }
  void addValue(SharedExNdPtr value) { _values.push_back(value); }
  int getSize() {
    if (_size >= 0) {
      return _size;
    } else {
      int size = 1;
      for (SharedExNdPtr i : _dimensions) {
        if (i->_type == front::express::NodeType::CNS) {
          size *= i->_value;
        } else {
          _size = 0;
          return _size;
        }
      }
      _size = size * static_pointer_cast<IntSymbol>(_item)->getSize();
      return _size;
    }
  }
  int getLen() {
    if (_size == -1) {
      getSize();
    }

    return _size / static_pointer_cast<IntSymbol>(_item)->getSize();
  }
  bool valid() const { return _values.size() * 4 == _size; }
};

class FunctionSymbol final : public Symbol {
public:
  SymbolKind _ret;
  vector<SharedSyPtr> _params;

  FunctionSymbol(string name, SymbolKind ret, int layerNum = 0)
      : _ret(ret), Symbol(name, layerNum) {
    id = Symbol::symbolId++;
  }
  virtual ~FunctionSymbol() {}
  virtual SymbolKind kind() const { return SymbolKind::Function; }
  SymbolKind getRet() const { return _ret; }
  void addParam(SharedSyPtr param) { _params.push_back(param); }
  vector<SharedSyPtr> &getParams() { return _params; }
};
} // namespace front::symbol

#endif // !COMPILER_FRONT_SYMBOL_H_
