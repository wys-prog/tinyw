#pragma once

#include <mutex>
#include <stack>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>

#include "glob.hpp"

TinyWDeclStart

class GenericTask {
	std::string _title;
	std::function<void(std::function<void(float)>)> _work;
	std::atomic<float> _progress = 0.0f;

public:
	GenericTask(const std::string& title,
	            std::function<void(std::function<void(float)>)> work)
		: _title(title), _work(work) {}

	const std::string& get_title() const { return _title; }
	float get_progress() const { return _progress.load(); }

	void run() {
		_work([this](float p) {
			_progress.store(std::clamp(p, 0.0f, 1.0f));
		});
		_progress.store(1.0f);
	}
};

void run_task_with_ui(GenericTask& task) {
  std::string what;
  std::mutex what_mutex;
  std::atomic<bool> throw_requested{false};
  std::thread runner([&]() { 
    try {
      task.run();
      return;
    } catch (const std::runtime_error &e) {
      std::stringstream ss;
      ss << "\n> C++ Exception caught from thread " << std::this_thread::get_id() <<
                   "\n> what():"
                   << e.what() << std::endl;
      {
        std::lock_guard<std::mutex> lock(what_mutex);
        what = ss.str();
      }
    } catch (const std::exception &e) {
      std::stringstream ss;
      ss << "\n> C++ Exception caught from thread " << std::this_thread::get_id() <<
      "\n> what():"
      << e.what() << std::endl;
      {
        std::lock_guard<std::mutex> lock(what_mutex);
        what = ss.str();
      }
    } catch (...) {
      std::stringstream ss;
      ss << "\n> C++ Exception caught from thread " << std::this_thread::get_id() <<
      "\n> <unknown exception>" << std::endl;
      {
        std::lock_guard<std::mutex> lock(what_mutex);
        what = ss.str();
      }
    }

    throw_requested.store(true);
  });

  const int bar_width = 24;

  while (task.get_progress() < 1.0f) {
    float progress = task.get_progress();
    int pos = static_cast<int>(progress * bar_width);

    std::cout << "\r" << "> [";
    for (int i = 0; i < bar_width; ++i) {
      if (i < pos) std::cout << "#";
      else std::cout << ".";
    }
    
    std::cout << "] " << std::setw(5) << std::setfill(' ') << std::fixed << std::setprecision(1) << (progress * 100.0f) << "%" 
        " | " << task.get_title()
        << std::flush;

    if (throw_requested.load()) {
      std::lock_guard<std::mutex> lock(what_mutex);
      std::cout << what << std::endl;
      exit(-1);
    }
      
      
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  runner.join();

  std::cout  << "\r" << "> [########################] 100.0% | " << task.get_title() << "\n";
}

void run_tasks(const std::vector<GenericTask*> &tasks) {
  for (auto task:tasks) {
    run_task_with_ui(*task);
  }
}


GenericTask copy_file(const std::string &title, const std::string &file1, const std::string &file2, const size_t buffer_size = 1) {
  return GenericTask(title, [&](auto report_progress) {
    std::ifstream input(file1);
    std::ofstream out(file2, std::ios::binary);
      
    input.seekg(0, std::ios::end);
    auto total = input.tellg();
    input.seekg(0);
    
    char *buffer = new char[buffer_size];
    size_t copied = 0;
    
    while (input && out) {
      input.read(buffer, buffer_size);
      auto bytes = input.gcount();
      out.write(buffer, bytes);
      copied += bytes;
      report_progress(float(copied) / total);
    }

    delete[] buffer;
  });
}

template <typename stringT>
GenericTask create_directories(const std::string &title, const std::vector<stringT> &DIRs) {
  return GenericTask(title, [=](auto report_progress) {
    size_t total = DIRs.size();
    size_t done = 0;

    for (const auto &dir : DIRs) {
      (std::filesystem::create_directories(dir));

      ++done;
      report_progress(static_cast<float>(done) / total);
    }

  });
}

GenericTask has_features(const std::string& title,
                         const std::vector<std::string>& CMDs,
                         std::unordered_map<std::string, int>& out) {
  return GenericTask(title, [&](auto report_progress) {
    for (size_t i = 0; i < CMDs.size(); i++) {
      const auto& cmd = CMDs[i];
#if defined(_WIN32)
      std::string test_cmd = "where " + cmd + " >nul 2>&1";
#else
      std::string test_cmd = "command -v " + cmd + " >/dev/null 2>&1";
#endif
      out[cmd] = std::system(test_cmd.c_str());
      report_progress(static_cast<float>(i + 1) / CMDs.size());
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });
}

GenericTask heavy_computation_task(const std::string& title, size_t iterations = 1000) {
  return GenericTask(title, [=](auto report_progress) {
    for (size_t i = 0; i < iterations; ++i) {
      report_progress(static_cast<float>(i) / iterations);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    report_progress(1.0f);
  });
}

TinyWDeclEnd