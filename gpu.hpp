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

class GPU {
private:
  DynamicLibrary lib;
  Tfunc_GPUSendBytes ESendBytes;
  Tfunc_GPUStart     EStart;
  Tfunc_SignVoid     EStop;

public:
  void init(const fs::path &file, const std::vector<std::string> &argv) {
    if (!lib.Open(file)) {
      std::cerr << "\n> Cannot open " << file << std::endl;
      std::cerr << "> [i:err]: " << lib.Error() << std::endl;
      return;
    }

    ESendBytes = (Tfunc_GPUSendBytes)lib.GetSymbol("send_bytes");
    EStart     = (Tfunc_GPUStart)lib.GetSymbol("start");
    EStop      = (Tfunc_SignVoid)lib.GetSymbol("stop");
    auto Einit = (Tfunc_InitArgv)lib.GetSymbol("init");

    if (!EStart || !ESendBytes || !Einit || !EStop) {
      std::stringstream errmsg;
      errmsg << "Failed to load required symbols from " << file << "\n"
      "handles:\n"
      "* EStart:\t" << is_true(EStart == nullptr) << "\n"
      "* ESendBytes:\t" << is_true(ESendBytes == nullptr) << "\n"
      "* Einit:\t" << is_true(Einit == nullptr) << "\n";
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

  void send_bytes(uint8_t *Bytes, uint64_t Len) {
    ESendBytes(Bytes, Len);
  }

  void start() { EStart(); }
  void stop() { EStop(); }
};

TinyWDeclEnd