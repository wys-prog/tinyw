#pragma once

#define TinyWDeclStart namespace wylma { namespace wyland {
#define TinyWDeclEnd } }
#define TinyWDecl(x) TinyWDeclStart x TinyWDeclEnd

#define TinyWBackendVersion V1
#define TinyWBackendStartDecl TinyWDeclStart namespace backend { namespace TinyWBackendVersion {
#define TinyWBackendEndDecl   } } TinyWDeclEnd

#define TinyWBackGet(x) backend::TinyWBackendVersion::x

#define nameof(x) #x
#define typeof(x) typeid(x)
#define okay(x) x
#define hahahah(x) x
#define decl_scope 

#if defined(__GNUC__) || defined(__clang__)
#define func __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define func __FUNCSIG__
#else
#define func __func__
#endif

#ifdef _WIN32
#define cstrdup _strdup
#else
#define cstrdup strdup
#endif 

#ifdef _WIN32
#define PYTHON "python"
#define CLEAR  "cls"
#else
#define PYTHON "python3"
#define CLEAR  "clear"
#endif

#define try_x(x) do { \
    try { \
        x; \
    } catch (const std::runtime_error &e) { \
        therr(func, std::string("C++ Exception caught: ") + e.what()); \
    } catch (const std::exception &e) { \
        therr(func, std::string("C++ Exception caught: ") + e.what()); \
    } catch (...) { \
        therr(func, "Unknown C++ Exception caught"); \
    } \
} while(0)

#include <filesystem>

TinyWDecl(okay(namespace fs = std::filesystem;))
TinyWDecl(
  typedef uint8_t*(*Tfunc_MemoryGetPointer)();
  typedef uint64_t(*Tfunc_MemoryGetSize)();
  typedef void(*Tfunc_SignVoid)();
  typedef void(*Tfunc_InitArgv)(uint64_t, char *const[]);
  typedef void(*Tfunc_GPUSendBytes)(uint8_t*, uint64_t);
  typedef void(*Tfunc_GPUStart)();
  typedef void(*Tfunc_CPUStart)();
  typedef void(*Tfunc_CPUInit)(Tfunc_MemoryGetPointer, Tfunc_MemoryGetSize, uint64_t, char *const[]);
)

TinyWDecl(
  inline std::string to_lowercase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
             [](unsigned char c){ return std::tolower(c); });
    return result;
  }
)


TinyWDecl(
  std::string is_true(bool WHAT_AGAIN) { 
  return WHAT_AGAIN ? "true" : "false";
  }
)

#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <set>
#include <list>
#include <map>
#include <queue>
#include <deque>

TinyWBackendStartDecl

class AnyString {
private:
  std::shared_ptr<std::ostringstream> oss;

  template <typename T>
  static T AnyTrim(const T &t) { return t; }

  static std::string AnyTrim(const std::string &str) {
    auto first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    auto last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
  }

  static std::string AnyTrim(float f) {
    return std::to_string(f) + "f";
  }

public:
  AnyString() : oss(std::make_shared<std::ostringstream>()) {}

  // Copie
  AnyString(const AnyString& other) : oss(std::make_shared<std::ostringstream>(other.oss->str())) {}

  // Affectation
  AnyString& operator=(const AnyString& other) {
    if (this != &other) {
      oss = std::make_shared<std::ostringstream>(other.oss->str());
    }
    return *this;
  }

  template <typename T>
  AnyString(const T &e) : oss(std::make_shared<std::ostringstream>()) {
    *oss << (e);
  }

  template <typename T>
  AnyString(const std::vector<T>& vec) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "{\n\t";
    for (size_t i = 0; i < vec.size(); ++i) {
      *oss << AnyTrim(vec[i]);
      if (i + 1 != vec.size()) *oss << ", ";
    }
    *oss << "\n}";
  }

  template <typename T>
  AnyString(const std::list<T>& lst) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "[";
    auto it = lst.begin();
    while (it != lst.end()) {
      *oss << AnyTrim(*it);
      if (++it != lst.end()) *oss << ", ";
    }
    *oss << "]";
  }

  template <typename T>
  AnyString(const std::set<T>& s) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "{";
    auto it = s.begin();
    while (it != s.end()) {
      *oss << AnyTrim(*it);
      if (++it != s.end()) *oss << ", ";
    }
    *oss << "}";
  }

  template <typename K, typename V>
  AnyString(const std::map<K, V>& m) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "{";
    auto it = m.begin();
    while (it != m.end()) {
      *oss << "[" << AnyTrim(it->first) << ": " << AnyTrim(it->second) << "]";
      if (++it != m.end()) *oss << ", ";
    }
    *oss << "}";
  }

  template <typename T>
  AnyString(const std::deque<T>& dq) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "[";
    for (size_t i = 0; i < dq.size(); ++i) {
      *oss << AnyTrim(dq[i]);
      if (i + 1 != dq.size()) *oss << ", ";
    }
    *oss << "]";
  }

  template <typename T>
  AnyString(const std::queue<T>& q) : oss(std::make_shared<std::ostringstream>()) {
    std::queue<T> copy = q;
    *oss << "[";
    while (!copy.empty()) {
      *oss << AnyTrim(copy.front());
      copy.pop();
      if (!copy.empty()) *oss << ", ";
    }
    *oss << "]";
  }

  template <typename T>
  AnyString(const std::stack<T>& s) : oss(std::make_shared<std::ostringstream>()) {
    std::stack<T> copy = s;
    std::vector<T> elems;
    while (!copy.empty()) {
      elems.push_back(copy.top());
      copy.pop();
    }
    *oss << "{\n";
    for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
      *oss << AnyTrim(*it);
      if (it + 1 != elems.rend()) *oss << ", ";
    }
    *oss << "\n}";
  }

  template <typename T>
  AnyString(const std::unordered_set<T>& s) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "{\n";
    auto it = s.begin();
    while (it != s.end()) {
      *oss << AnyTrim(*it);
      if (++it != s.end()) *oss << ", ";
    }
    *oss << "\n}";
  }

  template <typename K, typename V>
  AnyString(const std::unordered_map<K, V>& m) : oss(std::make_shared<std::ostringstream>()) {
    *oss << "{\n";
    auto it = m.begin();
    while (it != m.end()) {
      *oss << AnyTrim(it->first) << ": " << AnyTrim(it->second);
      if (++it != m.end()) *oss << ", ";
    }
    *oss << "\n}";
  }

  AnyString(const std::exception &e) : oss(std::make_shared<std::ostringstream>()) {
    *oss << e.what();
  }

  std::string str() const {
    return oss->str();
  }

  template <typename T>
  AnyString &operator|(const T &elem) {
    *oss << AnyString(elem).str();
    return *this;
  }
};

inline std::string TinyWBackfmt_err(const std::string &from, const AnyString &what) {
  std::stringstream ss;
  ss << "\n> [i:err]: " << from << ":\n";
  std::istringstream iss(what.str());
  std::string line;
  while (std::getline(iss, line)) {
    ss << "> " << line << std::endl;
  }

  return(ss.str());
}

inline void TinyWBacktherr(const std::string &from, const AnyString &what) {
  throw std::runtime_error(TinyWBackfmt_err(from, what));
}

TinyWBackendEndDecl


// Public Redefinitions

TinyWDeclStart

using AnyString = TinyWBackGet(AnyString);

inline void therr(const std::string &from, const AnyString &what) {
  return TinyWBackGet(TinyWBacktherr)(from, what);
}

inline std::string fmt_err(const std::string &from, const AnyString &what) { 
  return TinyWBackGet(TinyWBackfmt_err)(from, what);
}

TinyWDeclEnd
