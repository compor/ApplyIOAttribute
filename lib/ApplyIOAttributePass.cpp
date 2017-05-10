//
//
//

#define DEBUG_TYPE "applyioattribute"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc

#include "llvm/Support/FileSystem.h"
// using llvm::raw_fd_ostream
// using llvm::sys::fs::OpenFlags

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <set>
// using std::set

#include <string>
// using std::string

#include <system_error>
// using std::error_code

#include "ApplyIOAttributePass.hpp"

#ifndef NDEBUG
#define PLUGIN_OUT llvm::outs()
//#define PLUGIN_OUT llvm::nulls()

// convenience macro when building against a NDEBUG LLVM
#undef DEBUG
#define DEBUG(X)                                                               \
  do {                                                                         \
    X;                                                                         \
  } while (0);
#else // NDEBUG
#define PLUGIN_OUT llvm::dbgs()
#endif // NDEBUG

#define PLUGIN_ERR llvm::errs()

// plugin registration for opt

char icsa::ApplyIOAttributePass::ID = 0;
static llvm::RegisterPass<icsa::ApplyIOAttributePass>
    X("apply-io-attribute", "apply IO attribute pass", false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerApplyIOAttributePass(const llvm::PassManagerBuilder &Builder,
                             llvm::legacy::PassManagerBase &PM) {
  PM.add(new icsa::ApplyIOAttributePass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterApplyIOAttributePass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                                 registerApplyIOAttributePass);

//

static llvm::cl::opt<std::string>
    ReportStats("aioattr-stats",
                llvm::cl::desc("apply IO attribute stats report filename"));

namespace icsa {

static long NumAttributeApplications = 0;
static std::set<std::string> FunctionsAltered;

void ApplyIOAttributePass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
  AU.setPreservesCFG();

  return;
}

bool ApplyIOAttributePass::runOnModule(llvm::Module &M) {
  bool hasChanged = false;
  const auto &TLI = getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
  ApplyIOAttribute aioattr(TLI);

  for (auto &func : M)
    if (!func.isDeclaration() && !func.hasFnAttribute(aioattr.getIOAttr()) &&
        aioattr.hasIO(func)) {
      hasChanged |= aioattr.apply(func);

      if (!ReportStats.empty()) {
        NumAttributeApplications++;
        FunctionsAltered.insert(func.getName());
      }
    }

  if (!ReportStats.empty()) {
    std::error_code err;
    llvm::raw_fd_ostream report(ReportStats, err, llvm::sys::fs::F_Text);

    if (err)
      PLUGIN_ERR << "could not open file: \"" << ReportStats
                 << "\" reason: " << err.message() << "\n";
    else {
      report << NumAttributeApplications << "\n";

      for (const auto &name : FunctionsAltered)
        report << name << "\n";

      report.close();
    }
  }

  return hasChanged;
}

} // namespace icsa end
