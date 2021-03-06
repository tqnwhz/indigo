#pragma once

#include <string>
#include <unordered_map>

#include "symbol.hpp"

#ifndef COMPOLIER_FRONT_SYMBOL_TABLE_H_
#define COMPOLIER_FRONT_SYMBOL_TABLE_H_

namespace front::symbolTable {
using front::symbol::SharedSyPtr;
using std::string;
using std::unordered_map;
using std::vector;

class SymbolTable {
public:
  SymbolTable();
  ~SymbolTable() {}
  void push_symbol(SharedSyPtr symbol);
  void pop_symbol();
  void pop_layer_symbols(unsigned int layer);
  bool empty() const;
  size_t size() const;
  SharedSyPtr find_least_layer_symbol(string name);

private:
  vector<SharedSyPtr> symbol_stack;
  unordered_map<string, vector<SharedSyPtr>> symbol_map;
  SharedSyPtr holder_symbol;
};
} // namespace front::symbolTable

#endif // !COMPOLIER_FRONT_SYMBOL_TABLE_H_
