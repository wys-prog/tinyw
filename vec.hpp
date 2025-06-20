#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

#include "glob.hpp"

TinyWDeclStart

template<typename T>
struct vec {
  std::vector<T> data;

  vec<T>& operator|(const T& value) {
    data.push_back(value);
    return *this;
  }

  operator std::vector<T>() const {
    return data;
  }
};

template <typename T>
void add_all(const T &Aelem, std::vector<T> &ref) {
  for (auto &elem:ref) {
    elem += Aelem;
  }
}

std::vector<fs::path> add_fs_prefix(const fs::path &prefix, const std::vector<fs::path> &paths) {
  std::vector<fs::path> v{};
  for (const auto&p:paths) v.push_back(prefix / p);
  return v;
}

TinyWDeclEnd