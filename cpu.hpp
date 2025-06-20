#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstring>
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

class CPU {
private:
  DynamicLibrary lib;
  Tfunc_CPUStart EStart;
  Tfunc_SignVoid EStop;

public:
  void start() {
    EStart();
  }

  void init(const fs::path &file, 
    Tfunc_MemoryGetPointer GetPointer, 
    Tfunc_MemoryGetSize GetSize, 
    const std::vector<std::string> &argv) {
    if (!lib.Open(file)) {
      std::cerr << "\n> Cannot open " << file << std::endl;
      std::cerr << "> [i:err]: " << lib.Error() << std::endl;
      return;
    }

    EStart = (Tfunc_CPUStart)lib.GetSymbol("start");
    EStop  = (Tfunc_SignVoid)lib.GetSymbol("stop");
    auto Einit = (Tfunc_CPUInit)lib.GetSymbol("init");

    if (!EStart || !Einit || !EStop) {
      std::stringstream errmsg;
      errmsg << "Failed to load required symbols from " << file << "\n"
      "handles:\n"
      "* EStart:\t" << is_true(EStart == nullptr) << "\n"
      "* Einit:\t" << is_true(Einit == nullptr) << "\n";
      therr(func, errmsg.str());
    }

    std::vector<char*> cstr_argv;
    cstr_argv.reserve(argv.size());
    for (const auto& arg : argv) {
      cstr_argv.push_back(cstrdup(arg.c_str())); 
    }

    Einit(GetPointer, GetSize, cstr_argv.size(), cstr_argv.data());

    for (auto ptr : cstr_argv) {
      free(ptr);
    }
  }

  void stop() { return EStop(); }
};

TinyWDeclEnd