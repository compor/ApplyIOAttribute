//
//
//

#ifndef APPLYIOATTRIBUTEPASS_HPP
#define APPLYIOATTRIBUTEPASS_HPP

#include "llvm/Pass.h"
// using llvm::ModulePass

#include "ApplyIOAttribute.hpp"

namespace llvm {
class Module;
class Function;
class Type;
class FunctionType;
} // namespace llvm end

namespace icsa {

class ApplyIOAttributePass : public llvm::ModulePass {
public:
  static char ID;

  ApplyIOAttributePass() : llvm::ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // APPLYIOATTRIBUTEPASS_HPP

