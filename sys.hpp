#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>
#include <filesystem>
#include <unordered_map>

#if defined(_WIN32)
  #include <windows.h>
  #define LIB_HANDLE HMODULE
  #define LOAD_LIBRARY(name) LoadLibraryA(name)
  #define GET_PROC_ADDR GetProcAddress
  #define CLOSE_LIBRARY FreeLibrary
  #define LIB_EXTENTION ".dll"
#elif defined(__APPLE__)
  #include <dlfcn.h>
  #define LIB_HANDLE void*
  #define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
  #define GET_PROC_ADDR dlsym
  #define CLOSE_LIBRARY dlclose
  #define LIB_EXTENTION ".dylib"
#else
  #include <dlfcn.h>
  #define LIB_HANDLE void*
  #define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
  #define GET_PROC_ADDR dlsym
  #define CLOSE_LIBRARY dlclose
  #define LIB_EXTENTION ".so"
#endif


#include "glob.hpp"
#include "tasks.hpp"
#include "vec.hpp"
#include ".libcont.hpp"

TinyWDeclStart

class DynamicLibrary {
  LIB_HANDLE handle_ = nullptr;
  std::string loaded_name_;

public:
  static std::string BuildLibName(const std::string &base) {

    if (!base.ends_with(LIB_EXTENTION)) {
#if defined(_WIN32)
      std::string name = base + LIB_EXTENTION;
#else
      std::string name = base + LIB_EXTENTION;
#endif
      return name;
    }
    

    return base;
  }

  bool Open(const std::string& base_name) {
    Close();
    std::string lib_name = BuildLibName(base_name);
    if (lib_name.empty()) return false;
    handle_ = LOAD_LIBRARY(lib_name.c_str());
    if (handle_) loaded_name_ = lib_name;
    return handle_ != nullptr;
  }

  void* GetSymbol(const std::string& symbol) {
    if (!handle_) return nullptr;
    return (void*)GET_PROC_ADDR(handle_, symbol.c_str());
  }

  void Close() {
    if (handle_) {
      CLOSE_LIBRARY(handle_);
      handle_ = nullptr;
      loaded_name_.clear();
    }
  }

  std::string Name() const { return loaded_name_; }
  bool IsOpen() const { return handle_ != nullptr; }

  std::string Error() {
    #if defined(_WIN32)
      DWORD errorMessageID = ::GetLastError();
      if (errorMessageID == 0)
        return "";
      LPSTR messageBuffer = nullptr;
      size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
      std::string message(messageBuffer, size);
      LocalFree(messageBuffer);
      return message;
    #else
      const char* err = dlerror();
      return err ? std::string(err) : "";
    #endif
  }

  DynamicLibrary() = default;

  DynamicLibrary(const fs::path &path) { Open(path); }

  ~DynamicLibrary() { Close(); }

  typedef struct {
    bool IsOpen;
    std::string Error;
    fs::path Path;
  } OpenStatus;

  static OpenStatus TestLib(const fs::path &lib) {
    DynamicLibrary tester(lib);
    
    return OpenStatus {
      .IsOpen = tester.IsOpen(), 
      .Error  = tester.Error(), 
      .Path = fs::absolute(lib)
    };
  }
};

typedef struct {
  fs::path       path;
  DynamicLibrary lib;
} CoreObject;

struct ExtentionObject {
  fs::path       path;
  DynamicLibrary lib;
};

typedef void(*ExtentionFunc)();

class System {
private:
  static std::vector<std::filesystem::path> GetFoldersToCreate() {
    return {
      "lib/", "lib/runtime/", "lib/static/", "lib/sys/", 
      "bin/", "bin/vm/", "bin/modules/", "bin/etc/", 
      "dev/", "dev/tools/", "dev/bin/", 
      "include/", "include/tinyw/",
      "vm/", 
      "settings/",
      "extentions/",
    };
  }

public:
  static std::string ExecutionFile(const std::filesystem::path &path = "") {
    static std::filesystem::path MyPath;
    if (path == "") return MyPath;
    
    if (std::filesystem::exists(std::filesystem::absolute(path))) {
      MyPath = std::filesystem::absolute(path);
    } else {
      std::cerr << "[e]: No such file: " << path << std::endl;
    }

    return MyPath;
  }

  static std::string GetSystemHome() {
  #if defined(_WIN32)
    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
      return std::string(homeDrive) + std::string(homePath);
    }
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
      return std::string(userProfile);
    }
    return "";
  #else
    const char* home = std::getenv("HOME");
    if (home) {
      return std::string(home);
    }
    return "";
  #endif
  }

  static fs::path GetHome() {
    std::filesystem::path home = GetSystemHome();

    if (!std::filesystem::exists(home / ".tinyw/")) {
      std::filesystem::path prefix = home / ".tinyw/";
      std::filesystem::create_directories(prefix);

      std::ifstream input(ExecutionFile());
      if (!input) {
        std::cerr << "[e]: Unable to get execution file path --Maybe System not inited--" << std::endl;
        return home / ".tinyw";
      }
      input.close();

      auto copy_task = copy_file("copying execution file", ExecutionFile(), prefix / "bin" / "tinyw");
      auto dir_task  = create_directories("creating directories", add_fs_prefix(prefix, GetFoldersToCreate()));
      auto home_task = create_directories("creating home  ", std::vector{prefix});
      
      vec<GenericTask*> tasks{};
      tasks | &home_task | &dir_task | &copy_task;
      run_tasks(tasks);

      auto cpy_built = copy_built_in_files(prefix / "include" / "tinyw/");
      run_task_with_ui(cpy_built);
    }

    return home / ".tinyw";
  }

  static std::unordered_map<std::string, int> GetSubCommand(const std::vector<std::string> &cmd = {}) {
    static std::unordered_map<std::string, int> out;
    if (cmd.empty()) return out;
    auto task = has_features("checking features", cmd, out);
    run_task_with_ui(task);
    return out;
  }

  static bool HasAllSubCommands(const std::vector<std::string> &cmd) {
    auto subcmds = GetSubCommand(cmd);
    for (const auto& [_, value] : subcmds) {
      if (value != 0) return false;
    }
    return true;
  }

  static std::vector<fs::path> GetEntries(const fs::path &at) {
    std::vector<fs::path> e;
    for (const auto &el:fs::directory_iterator(at)) 
      if (el.is_regular_file()) e.push_back(fs::absolute(el));
    return e;
  }

  static std::unordered_map<std::string, CoreObject> GetCores(bool reload = false) {
    std::unordered_map<std::string, CoreObject> cores{};
    if (!cores.empty() && !reload) return cores;

    auto home = GetHome();
    auto entries = GetEntries(home / "bin");
    auto task = GenericTask("scanning ~t/bin/", [&](auto report_progress){
      for (size_t i = 0; i < entries.size(); i++) {
        auto path = entries[i];

        if (fs::exists(path)) {
          cores[path.filename()] = CoreObject{
            .path = fs::absolute(path), 
            .lib = DynamicLibrary(path.parent_path() / path.filename())
          };
        }
        
        report_progress((float)(i) / entries.size());
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }

      report_progress(1.0f);
    });

    run_task_with_ui(task);

    return cores;
  }

  static std::unordered_map<std::string, ExtentionObject> GetExtentions(bool reload = false) {
    static std::unordered_map<std::string, ExtentionObject> extens;
    if (!extens.empty() && !reload) return extens;

    auto home = GetHome();
    auto entries = GetEntries(home / "extentions/");
    auto task = GenericTask("launch extentions", [&](auto report_progress){
      for (size_t i = 0; i < entries.size(); i++) {
        auto path = entries[i];

        if (fs::exists(path)) {
          extens[path.filename()] = ExtentionObject{
            .path = path, 
            .lib = DynamicLibrary(path)
          };
        }
        
        report_progress((float)(i) / entries.size());
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }

      report_progress(1.0f);
    });

    run_task_with_ui(task);

    std::cout << "> found " << extens.size() << " extentions" << std::endl;

    return extens;
  }

  static void LaunchExtentions() {
    auto extentions = GetExtentions(true);
    auto loaded = size_t(0);
    auto opened = size_t(0);

    auto task = GenericTask("loading extentions", [&](auto report_progress){
      size_t done = 0;
      for (auto &key:extentions) {
        key.second.lib.Open(key.second.path);
        if (key.second.lib.IsOpen()) {
          ExtentionFunc fh = (ExtentionFunc)key.second.lib.GetSymbol("entry");
          opened++;

          if (fh != nullptr) {
            fh();
            loaded++;
          }
        }

        report_progress((float)done++ / extentions.size());
      }

      report_progress(1.0f);
    });

    run_task_with_ui(task);

    std::cout << "> loaded " << loaded << " extentions" << std::endl;
    std::cout << "> opened " << opened << " extentions" << std::endl;
  }

  static void CheckAndFixHome() {
    auto home = GetHome();
    std::vector<fs::path> expected_dirs = add_fs_prefix(home, GetFoldersToCreate());
    auto created = size_t(0);

    auto task = GenericTask("checking home", [&](auto report_progress) {
      size_t total = expected_dirs.size();
      size_t done = 0;
      for (const auto& dir : expected_dirs) {
        if (!fs::exists(dir)) {
          std::error_code ec;
          fs::create_directories(dir, ec);
          created++;
        }

        ++done;
        report_progress(static_cast<float>(done) / total);
      }
    });

    run_task_with_ui(task);
    std::cout << "> " << created << " directories created" << std::endl;
  }
};



TinyWDeclEnd