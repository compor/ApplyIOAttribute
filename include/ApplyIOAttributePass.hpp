//
//
//

#ifndef APPLYIOATTRIBUTEPASS_HPP
#define APPLYIOATTRIBUTEPASS_HPP

#include "llvm/Pass.h"
// using llvm::ModulePass

namespace llvm {
class Module;
} // namespace llvm end

namespace icsa {

class ApplyIOAttributePass : public llvm::ModulePass {
public:
  static char ID;

  ApplyIOAttributePass() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // end of include guard: APPLYIOATTRIBUTEPASS_HPP
