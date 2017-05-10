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

namespace icsa {

class ApplyIOAttribute {
public:
  ApplyIOAttribute(const llvm::TargetLibraryInfo &TLI,
                   llvm::StringRef IOAttr = "icsa-io")
      : m_IOAttr{IOAttr}, m_TLI{TLI} {
    setupIOFuncs();
    setupCxxIOFuncs();

    return;
  }

  bool hasIO(const llvm::Function &Func) const;

  bool apply(llvm::Function &func) const;

  inline llvm::StringRef getIOAttr() const { return m_IOAttr; }

private:
  bool hasCIO(const llvm::Function &Func) const;
  bool hasCxxIO(const llvm::Function &Func) const;

  llvm::Function *getCalledFunction(const llvm::Instruction &Inst) const;
  llvm::Type *getClassFromMethod(const llvm::FunctionType &FuncType) const;
  std::string demangleCxxName(const char *name) const;

  void setupIOFuncs();
  void setupCxxIOFuncs();

  const llvm::TargetLibraryInfo &m_TLI;
  std::set<llvm::LibFunc::Func> IOLibFuncs;

  std::vector<std::string> CxxIOFuncs;
  std::vector<std::string> CxxIOTypes;

  const llvm::StringRef m_IOAttr;
};

} // namespace icsa end

#endif // APPLYIOATTRIBUTE_HPP