//
//
//

#ifndef APPLYIOATTRIBUTE_HPP
#define APPLYIOATTRIBUTE_HPP

#include "llvm/Analysis/TargetLibraryInfo.h"
// using llvm::LibFunc

#include <string>
// using std::string

#include <vector>
// using std::vector

#include <set>
// using std::set

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

namespace llvm {
class Instruction;
class Loop;
class Function;
class FunctionType;
} // namespace llvm end

namespace icsa {

class ApplyIOAttribute {
public:
  ApplyIOAttribute(const llvm::TargetLibraryInfo &TLI,
                   llvm::StringRef IOAttr = "icsa-io")
      : m_IOAttr{IOAttr}, m_TLI{TLI} {
    setupLibCIOFuncs();
    setupCxxIOFuncs();

    return;
  }

  bool hasIO(const llvm::Function &Func) const;
  bool hasIO(const llvm::Loop &L) const;
  bool apply(llvm::Function &func) const;
  inline llvm::StringRef getIOAttr() const { return m_IOAttr; }

private:
  bool hasCIO(const llvm::Function &Func) const;
  bool hasCxxIO(const llvm::Function &Func) const;

  llvm::Function *getCalledFunction(const llvm::Instruction &Inst) const;
  llvm::Type *getClassFromMethod(const llvm::FunctionType &FuncType) const;
  std::string demangleCxxName(const char *name) const;

  void setupLibCIOFuncs();
  void setupCxxIOFuncs();

  const llvm::TargetLibraryInfo &m_TLI;
  std::set<llvm::LibFunc::Func> m_IOLibFuncs;

  std::vector<std::string> m_CxxIOFuncs;
  std::vector<std::string> m_CxxIOTypes;

  const llvm::StringRef m_IOAttr;
};

} // namespace icsa end

#endif // APPLYIOATTRIBUTE_HPP
