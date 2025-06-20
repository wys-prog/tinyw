#pragma once

#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <exception>
#include <filesystem>

#include "glob.hpp"
#include "tasks.hpp"
#include "vec.hpp"
#include "sys.hpp"

#include "cpu.hpp"
#include "gpu.hpp"
#include "mem.hpp"

TinyWDeclStart

class Core {
private:
  CPU MyCPU;
  GPU MyGPU;
  Memory MyMemory;
  std::atomic<bool> should_stop{false};

  void Open(const fs::path &cpu, const fs::path &gpu, const fs::path &memory, 
            const std::vector<std::string> &args_cpu, const std::vector<std::string> &args_gpu, 
            const std::vector<std::string> &args_mem) {
    auto task = GenericTask("init core", [&](auto progress_report){
      decl_scope {
        std::stack<std::string> errors{};
        auto TestCPU = DynamicLibrary::TestLib(cpu);
        progress_report(0.1f);
        if (!TestCPU.IsOpen) errors.push("CPU file `" + TestCPU.Path.string() + "`\n");

        auto TestGPU = DynamicLibrary::TestLib(gpu);
        progress_report(0.2f);
        if (!TestGPU.IsOpen) errors.push("GPU file `" + TestGPU.Path.string() + "`\n");

        auto TestMem = DynamicLibrary::TestLib(memory);
        progress_report(0.3f);
        if (!TestMem.IsOpen) errors.push("MEM file `" + TestMem.Path.string() + "`\n");

        if (!errors.empty()) therr(func, AnyString(errors));
      };

      progress_report(0.4f);
      MyMemory.init(memory, args_mem);

      progress_report(0.7f);
      MyGPU.init(gpu, args_gpu);

      progress_report(0.85f);
      MyCPU.init(cpu, MyMemory.get_EGetPointer(), MyMemory.get_EGetSize(), args_cpu);

      progress_report(1.0f);
    });

    run_task_with_ui(task);
  }

  void HandleArguments(const std::vector<std::string> &core_args, 
                      const std::vector<std::string> &cpu_args, 
                      const std::vector<std::string> &gpu_args, 
                      const std::vector<std::string> &mem_args
                    ) {
    fs::path cpu = "", gpu = "", mem = "";
    
    for (size_t i = 0; i + 1 < cpu_args.size(); ++i) {
      if (cpu_args[i] == "file") cpu = cpu_args[++i];
    }
    for (size_t i = 0; i + 1 < gpu_args.size(); ++i) {
      if (gpu_args[i] == "file") gpu = gpu_args[++i];
    }
    for (size_t i = 0; i + 1 < mem_args.size(); ++i) {
      if (mem_args[i] == "file") mem = mem_args[++i];
    }
    
    for (size_t i = 0; i < core_args.size(); ++i) {
      if (core_args[i] == "-stdout" && i + 1 < core_args.size()) {
      std::string filename = core_args[++i];
      if (fs::exists(filename)) 
        freopen(filename.c_str(), "w", stdout);
      else therr(func, "Failed to open stdout file: " + filename);
      } else if (core_args[i] == "-stderr" && i + 1 < core_args.size()) {
      std::string filename = core_args[++i];
      if (fs::exists(filename)) 
        freopen(filename.c_str(), "w", stderr);
      else therr(func, "Failed to open stderr file: " + filename);
      } else if (core_args[i] == "-stdin" && i + 1 < core_args.size()) {
      std::string filename = core_args[++i];
      if (fs::exists(filename)) 
        freopen(filename.c_str(), "r", stdin);
      else therr(func, "Failed to open stdin file: " + filename);
      }
    }

    if (!fs::exists(cpu)) {
      fs::path cpu_with_ext = cpu;
      cpu_with_ext += LIB_EXTENTION;
      if (!fs::exists(cpu_with_ext))
      therr(func, "CPU file: " + cpu.string() + " or " + cpu_with_ext.string() + " not found\nCannot continue..");
      cpu = cpu_with_ext;
    }
    if (!fs::exists(gpu)) {
      fs::path gpu_with_ext = gpu;
      gpu_with_ext += LIB_EXTENTION;
      if (!fs::exists(gpu_with_ext))
      therr(func, "GPU file: " + gpu.string() + " or " + gpu_with_ext.string() + " not found\nCannot continue..");
      gpu = gpu_with_ext;
    }
    if (!fs::exists(mem)) {
      fs::path mem_with_ext = mem;
      mem_with_ext += LIB_EXTENTION;
      if (!fs::exists(mem_with_ext))
      therr(func, "MEM file: " + mem.string() + " or " + mem_with_ext.string() + " not found\nCannot continue..");
      mem = mem_with_ext;
    }
    
    Open(cpu, gpu, mem, cpu_args, gpu_args, mem_args);
  }

  void Start() {
    std::exception_ptr cpu_exc = nullptr;
    std::exception_ptr gpu_exc = nullptr;

    std::thread cpu_thread([&] {
      try {
        MyCPU.start();
        should_stop.load();
        MyCPU.stop();
      } catch (...) {
        cpu_exc = std::current_exception();
        should_stop = true; 
      }
    });

    std::thread gpu_thread([&] {
      try {
        MyGPU.start();
        // GPU can't ask for shutting down
        MyGPU.stop();
      } catch (...) {
        gpu_exc = std::current_exception();
        should_stop = true; 
      }
    });

    cpu_thread.join();
    gpu_thread.join();
    MyMemory.clear();

    if (cpu_exc) std::rethrow_exception(cpu_exc);
    if (gpu_exc) std::rethrow_exception(gpu_exc);
  }

public:
  void Run(const std::vector<std::string> &args) {
    std::vector<std::string> core_args; // Prefix: -core:$ARG
    std::vector<std::string> cpu_args;  // Prefix: -cpu:$ARG
    std::vector<std::string> gpu_args;  // Prefix: -gpu:$ARG
    std::vector<std::string> mem_args;  // Prefix: -mem:$ARG
    fs::path exec_file;

    for (size_t i = 0; i < args.size(); i++) {      
      auto arg = to_lowercase(args[i]);
      if (arg == "-file" && i + 1 < args.size()) {
      exec_file = args[++i];
      } else if ((arg == "-core" || arg == "-cpu" || arg == "-gpu" || arg == "-mem") && i + 2 < args.size()) {
        if (arg == "-core") {
          core_args.push_back(args[++i]);
          core_args.push_back(args[++i]);
        } else if (arg == "-cpu") {
          cpu_args.push_back(args[++i]);
          cpu_args.push_back(args[++i]);
        } else if (arg == "-gpu") {
          gpu_args.push_back(args[++i]);
          gpu_args.push_back(args[++i]);
        } else if (arg == "-mem") {
          mem_args.push_back(args[++i]);
          mem_args.push_back(args[++i]);
        }
      } else {
      core_args.push_back(args[i]);
      }
    }
  
    if (!fs::exists(exec_file)) 
    therr(func, "File: " + exec_file.string() + " not found\nCannot continue..");
      
    exec_file = fs::absolute(exec_file);

    try_x(
      HandleArguments(core_args, cpu_args, gpu_args, mem_args);
    );

    Start();
  }

};

TinyWDeclEnd