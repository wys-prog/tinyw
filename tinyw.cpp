#include <iostream>
#include <string>
#include <vector>

#include "glob.hpp"
#include "sys.hpp"
#include "tasks.hpp"
#include "vec.hpp"
#include "core.hpp"

TinyWDeclStart

  void exec(const std::vector<std::string> &argv) {
    if (argv.size() < 1) therr(func, AnyString("Null argument vector (argv) -- No such task") | argv);
    
    std::vector<std::string> args(argv.begin() + 1, argv.end());
    
    if (argv[0] == "run") {
      Core core;
      try {
        core.Run(argv);
      } catch (const std::exception &e) {
        therr(func, "C++ Exception caught from main thread\n" + std::string(e.what()));
      } catch (...) {
        therr(func, "Unknown exception caught from main thread");
      }
    } else {
      AnyString anyString("Unknown argument: '" + argv[0] + "'");
      therr(func, anyString | argv[0] | " argv[] (internal): " | argv);
    }
  }

  int TinyWylandMain(int argc, char *const argv[]) {
    if (argc < 2) return -1;

    System::ExecutionFile(argv[0]);
    System::CheckAndFixHome();
    System::GetSubCommand({PYTHON, "curl", CLEAR});
    System::LaunchExtentions();

    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) args.push_back(argv[i]);

    try {
      exec(args);
    } catch (const std::runtime_error &e) {
      std::cerr << "> [i:err]: C++ Exception:\t" << e.what();
      return -1;
    } catch (const std::exception &e) {
      std::cerr << "> [i:err]: C++ Exception\t" << e.what();
      return -1;
    } catch (...) {
      std::cerr << "> [i:err]: Unknown Exception" << std::endl;
      return -1;
    }

    return 0;
  }

TinyWDeclEnd

int main(int argc, char *const argv[]) {
  return wylma::wyland::TinyWylandMain(argc, argv);
}