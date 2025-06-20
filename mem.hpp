#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "glob.hpp"
#include "tasks.hpp"
#include "vec.hpp"
#include "sys.hpp"

TinyWDeclStart

class Memory {
private:
  DynamicLibrary lib;
  Tfunc_MemoryGetSize    EGetSize;
  Tfunc_MemoryGetPointer EGetPointer;
  Tfunc_SignVoid         EClear;

public:
  Tfunc_MemoryGetSize get_EGetSize() const { return EGetSize; }
  Tfunc_MemoryGetPointer get_EGetPointer() const { return EGetPointer; }

  uint64_t GetSize() { return EGetSize(); }
  uint8_t *GetPointer() { return EGetPointer(); }

  void init(const fs::path &path, const std::vector<std::string> &argv) {
    if (!lib.Open(path)) {
      std::cerr << "> Cannot open file " << path << std::endl;
      std::cerr << "> [i:err]: " << lib.Error() << std::endl;
      return;
    }

    EGetSize    = (Tfunc_MemoryGetSize)lib.GetSymbol("get_size");
    EGetPointer = (Tfunc_MemoryGetPointer)lib.GetSymbol("get_pointer");
    EClear      = (Tfunc_SignVoid)lib.GetSymbol("clear");
    auto Einit  = (Tfunc_InitArgv)lib.GetSymbol("init");

    if (!EGetSize || !EGetPointer || !Einit || !EClear) {
      std::stringstream errmsg;
      errmsg << "Failed to load required symbols from " << path << "\n"
      "handles:\n"
      "* EGetPointer:\t" << is_true(EGetPointer == nullptr) << "\n"
      "* EGetSize:\t" << is_true(EGetSize == nullptr) << "\n"
      "* Einit: \t" << is_true(Einit == nullptr) << "\n";
      therr(func, errmsg.str());
    }

    std::vector<char*> cstr_argv;
    cstr_argv.reserve(argv.size());
    for (const auto& arg : argv) {
      cstr_argv.push_back(cstrdup(arg.c_str()));
    }

    Einit(cstr_argv.size(), cstr_argv.data());

    for (auto ptr : cstr_argv) {
      free(ptr);
    }
  }

  void clear() { return EClear(); }
};

TinyWDeclEnd