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
// using llvm::raw_fd_ostream

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::OpenFlags

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <set>
// using std::set

#include <string>
// using std::string

#include <fstream>
// using std::ifstream

#include <system_error>
// using std::error_code

#include <cstring>
// using std::strncmp

#include "Config.hpp"

#include "BWList.hpp"

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

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_DESC(x)                                                    \
  x " (version: " STRINGIFY(APPLYIOATTRIBUTE_VERSION) ")"

char icsa::ApplyIOAttributePass::ID = 0;
static llvm::RegisterPass<icsa::ApplyIOAttributePass>
    X("apply-io-attribute", PRJ_CMDLINE_DESC("apply IO attribute pass"), false,
      false);

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

static llvm::cl::opt<std::string> ReportStatsFilename(
    "aioattr-stats",
    llvm::cl::desc("apply IO attribute stats report filename"));

static llvm::cl::opt<std::string>
    FuncWhileListFilename("aioattr-fn-whitelist",
                          llvm::cl::desc("function whitelist"));

namespace icsa {

namespace {

long NumFunctionsProcessed = 0;
long NumAttributeApplications = 0;
std::set<std::string> FunctionsAltered;

void ReportStats(void) {
  PLUGIN_OUT << NumFunctionsProcessed << "\n";
  PLUGIN_OUT << NumAttributeApplications << "\n";

  for (const auto &name : FunctionsAltered)
    PLUGIN_OUT << name << "\n";

  return;
}

void ReportStats(const char *Filename) {
  const char *stdout_marker = "--";
  if (0 == std::strncmp(stdout_marker, Filename, strlen(stdout_marker))) {
    ReportStats();

    return;
  }

  std::error_code err;

  llvm::raw_fd_ostream report(Filename, err, llvm::sys::fs::F_Text);

  if (err)
    PLUGIN_ERR << "could not open file: \"" << ReportStatsFilename
               << "\" reason: " << err.message() << "\n";
  else {
    report << NumFunctionsProcessed << "\n";
    report << NumAttributeApplications << "\n";

    for (const auto &name : FunctionsAltered)
      report << name << "\n";
  }

  return;
}

} // namespace anonymous end

void ApplyIOAttributePass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
  AU.setPreservesCFG();

  return;
}

bool ApplyIOAttributePass::runOnModule(llvm::Module &M) {
  bool shouldReportStats = !ReportStatsFilename.empty();
  bool hasChanged = false;
  const auto &TLI = getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
  ApplyIOAttribute aioattr(TLI);

  BWList funcWhileList;
  if (!FuncWhileListFilename.empty()) {
    std::ifstream funcWhiteListFile{FuncWhileListFilename};

    if (funcWhiteListFile.is_open()) {
      funcWhileList.addRegex(funcWhiteListFile);
      funcWhiteListFile.close();
    } else
      PLUGIN_ERR << "could not open file: \'" << FuncWhileListFilename
                 << "\'\n";
  }

  for (auto &func : M) {
    if (!FuncWhileListFilename.empty() &&
        !funcWhileList.matches(func.getName().data()))
      continue;

    if (func.isDeclaration())
      continue;

    if (shouldReportStats)
      NumFunctionsProcessed++;

    if (!func.hasFnAttribute(aioattr.getIOAttr()) && aioattr.hasIO(func)) {
      hasChanged |= aioattr.apply(func);

      if (shouldReportStats) {
        NumAttributeApplications++;
        FunctionsAltered.insert(func.getName());
      }
    }
  }

  if (shouldReportStats)
    ReportStats(ReportStatsFilename.c_str());

  return hasChanged;
}

} // namespace icsa end
