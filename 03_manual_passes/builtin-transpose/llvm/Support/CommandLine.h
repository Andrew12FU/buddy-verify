#ifndef LLVM_SUPPORT_COMMANDLINE_H
#define LLVM_SUPPORT_COMMANDLINE_H

namespace llvm {
namespace cl {

struct Desc {};

inline Desc desc(const char *) { return Desc(); }

struct Init {
  Init() = default;

  template <typename T>
  Init(T) {}
};

template <typename T>
inline Init init(T) {
  return Init();
}

} // namespace cl
} // namespace llvm

#endif
