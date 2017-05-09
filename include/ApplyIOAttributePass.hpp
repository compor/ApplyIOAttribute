//
//
//

#ifndef APPLYIOATTRIBUTEPASS_HPP
#define APPLYIOATTRIBUTEPASS_HPP

#include "llvm/Pass.h"
// using llvm::ModulePass

// TODO to move to source file
#include "llvm/Analysis/TargetLibraryInfo.h"
// using llvm::LibFunc

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/DerivedTypes.h"
// using llvm::FunctionType

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::CallInst

#include "llvm/IR/IntrinsicInst.h"
// using llvm::IntrinsicInst

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include <cxxabi.h>
// using abi::__cxa_demangle

#include <cstdlib>
// using std::free

#include <memory>
// using std::unique_ptr

#include <set>
// using std::set

namespace llvm {
class Module;
} // namespace llvm end

namespace icsa {

struct malloc_deleter {
  template <typename T> void operator()(T *ptr) const { std::free(ptr); }
};

template <typename T>
using malloc_unique_ptr = std::unique_ptr<T, malloc_deleter>;

class ApplyIOAttribute {
public:
  ApplyIOAttribute(const llvm::TargetLibraryInfo &TLI) : m_TLI{TLI} {
    setupIOFuncs();

    return;
  }

  bool hasIO(const llvm::Function &Func) const {
    for (const auto &bb : Func)
      for (const auto &inst : bb) {
        const auto *calledFunc = getCalledFunction(inst);

        if (calledFunc && calledFunc->hasName()) {
          const auto &funcName = calledFunc->getName();
          llvm::LibFunc::Func TLIFunc;

          if (m_TLI.getLibFunc(funcName, TLIFunc) && m_TLI.has(TLIFunc) &&
              IOLibFuncs.end() != IOLibFuncs.find(TLIFunc)) {
            return true;
          }
        }
      }

    return false;
  }

  bool hasCxxIO(const llvm::Function &Func) const {
    if (Func.getFunctionType()->getNumParams() < 1)
      return false;

    if (!Func.hasName())
      return false;

    const auto &funcName = Func.getName();
    auto status = 0;

    const auto &demangledName = malloc_unique_ptr<char>(
        abi::__cxa_demangle(funcName.data(), 0, 0, &status));

    if (status)
      return false;

    if (std::end(CxxIOFuncs) == CxxIOFuncs.find(demangledName.get()))
      return false;

    const auto *potentialClassTypePtr = Func.getFunctionType()->getParamType(0);
    if (!potentialClassTypePtr->isPointerTy())
      return false;

    const auto *potentialClassType =
        llvm::dyn_cast<llvm::PointerType>(potentialClassTypePtr)
            ->getElementType();

    if (!potentialClassType->isStructTy())
      return false;

    const auto &potentialClassTypeName = potentialClassType->getStructName();

    if (std::end(CxxIOTypes) == CxxIOTypes.find(potentialClassTypeName.data()))
      return false;

    return true;
  }

  bool apply(llvm::Function &func) const {
    func.addFnAttr("icsa-io");

    return true;
  }

private:
  llvm::Function *getCalledFunction(const llvm::Instruction &Inst) const {
    if (llvm::isa<llvm::IntrinsicInst>(Inst))
      return nullptr;

    const auto *callInst = llvm::dyn_cast<llvm::CallInst>(&Inst);
    if (!callInst)
      return nullptr;

    const auto *calledFunc = callInst->getCalledFunction();
    if (!calledFunc)
      return nullptr;

    if (!callInst->getCalledFunction()->isDeclaration())
      return nullptr;

    return const_cast<llvm::Function *>(calledFunc);
  }

  void setupIOFuncs() {
    // TODO what about calls that might have other side-effects
    // system()
    // getenv()
    // setenv()
    // unsetenv()
    IOLibFuncs.insert(llvm::LibFunc::under_IO_getc);
    IOLibFuncs.insert(llvm::LibFunc::under_IO_putc);
    IOLibFuncs.insert(llvm::LibFunc::dunder_isoc99_scanf);
    IOLibFuncs.insert(llvm::LibFunc::access);
    IOLibFuncs.insert(llvm::LibFunc::chmod);
    IOLibFuncs.insert(llvm::LibFunc::chown);
    IOLibFuncs.insert(llvm::LibFunc::closedir);
    IOLibFuncs.insert(llvm::LibFunc::fclose);
    IOLibFuncs.insert(llvm::LibFunc::fdopen);
    IOLibFuncs.insert(llvm::LibFunc::feof);
    IOLibFuncs.insert(llvm::LibFunc::ferror);
    IOLibFuncs.insert(llvm::LibFunc::fflush);
    IOLibFuncs.insert(llvm::LibFunc::fgetc);
    IOLibFuncs.insert(llvm::LibFunc::fgetpos);
    IOLibFuncs.insert(llvm::LibFunc::fgets);
    IOLibFuncs.insert(llvm::LibFunc::fileno);
    IOLibFuncs.insert(llvm::LibFunc::fiprintf);
    IOLibFuncs.insert(llvm::LibFunc::flockfile);
    IOLibFuncs.insert(llvm::LibFunc::fopen);
    IOLibFuncs.insert(llvm::LibFunc::fopen64);
    IOLibFuncs.insert(llvm::LibFunc::fprintf);
    IOLibFuncs.insert(llvm::LibFunc::fputc);
    IOLibFuncs.insert(llvm::LibFunc::fputs);
    IOLibFuncs.insert(llvm::LibFunc::fread);
    IOLibFuncs.insert(llvm::LibFunc::fscanf);
    IOLibFuncs.insert(llvm::LibFunc::fseek);
    IOLibFuncs.insert(llvm::LibFunc::fseeko);
    IOLibFuncs.insert(llvm::LibFunc::fseeko64);
    IOLibFuncs.insert(llvm::LibFunc::fsetpos);
    IOLibFuncs.insert(llvm::LibFunc::fstatvfs);
    IOLibFuncs.insert(llvm::LibFunc::fstatvfs64);
    IOLibFuncs.insert(llvm::LibFunc::ftell);
    IOLibFuncs.insert(llvm::LibFunc::ftello);
    IOLibFuncs.insert(llvm::LibFunc::ftello64);
    IOLibFuncs.insert(llvm::LibFunc::ftrylockfile);
    IOLibFuncs.insert(llvm::LibFunc::funlockfile);
    IOLibFuncs.insert(llvm::LibFunc::fwrite);
    IOLibFuncs.insert(llvm::LibFunc::getc);
    IOLibFuncs.insert(llvm::LibFunc::getc_unlocked);
    IOLibFuncs.insert(llvm::LibFunc::getchar);
    IOLibFuncs.insert(llvm::LibFunc::getlogin_r);
    IOLibFuncs.insert(llvm::LibFunc::getpwnam);
    IOLibFuncs.insert(llvm::LibFunc::gets);
    IOLibFuncs.insert(llvm::LibFunc::iprintf);
    IOLibFuncs.insert(llvm::LibFunc::lchown);
    IOLibFuncs.insert(llvm::LibFunc::lstat);
    IOLibFuncs.insert(llvm::LibFunc::lstat64);
    IOLibFuncs.insert(llvm::LibFunc::mkdir);
    IOLibFuncs.insert(llvm::LibFunc::open);
    IOLibFuncs.insert(llvm::LibFunc::open64);
    IOLibFuncs.insert(llvm::LibFunc::opendir);
    IOLibFuncs.insert(llvm::LibFunc::pclose);
    IOLibFuncs.insert(llvm::LibFunc::perror);
    IOLibFuncs.insert(llvm::LibFunc::popen);
    IOLibFuncs.insert(llvm::LibFunc::pread);
    IOLibFuncs.insert(llvm::LibFunc::printf);
    IOLibFuncs.insert(llvm::LibFunc::putc);
    IOLibFuncs.insert(llvm::LibFunc::putchar);
    IOLibFuncs.insert(llvm::LibFunc::puts);
    IOLibFuncs.insert(llvm::LibFunc::pwrite);
    IOLibFuncs.insert(llvm::LibFunc::read);
    IOLibFuncs.insert(llvm::LibFunc::readlink);
    IOLibFuncs.insert(llvm::LibFunc::realpath);
    IOLibFuncs.insert(llvm::LibFunc::remove);
    IOLibFuncs.insert(llvm::LibFunc::rename);
    IOLibFuncs.insert(llvm::LibFunc::rewind);
    IOLibFuncs.insert(llvm::LibFunc::rmdir);
    IOLibFuncs.insert(llvm::LibFunc::scanf);
    IOLibFuncs.insert(llvm::LibFunc::siprintf);
    IOLibFuncs.insert(llvm::LibFunc::stat);
    IOLibFuncs.insert(llvm::LibFunc::stat64);
    IOLibFuncs.insert(llvm::LibFunc::statvfs);
    IOLibFuncs.insert(llvm::LibFunc::statvfs64);
    IOLibFuncs.insert(llvm::LibFunc::tmpfile);
    IOLibFuncs.insert(llvm::LibFunc::tmpfile64);
    IOLibFuncs.insert(llvm::LibFunc::ungetc);
    IOLibFuncs.insert(llvm::LibFunc::unlink);
    IOLibFuncs.insert(llvm::LibFunc::vfprintf);
    IOLibFuncs.insert(llvm::LibFunc::vfscanf);
    IOLibFuncs.insert(llvm::LibFunc::vprintf);
    IOLibFuncs.insert(llvm::LibFunc::vscanf);
    IOLibFuncs.insert(llvm::LibFunc::vsnprintf);
    IOLibFuncs.insert(llvm::LibFunc::vsscanf);
    IOLibFuncs.insert(llvm::LibFunc::write);

    return;
  }

  void setupCxxIOFuncs() {
    CxxIOFuncs.insert("put");
    CxxIOFuncs.insert("write");
    CxxIOFuncs.insert("flush");
    CxxIOFuncs.insert("get");
    CxxIOFuncs.insert("peek");
    CxxIOFuncs.insert("unget");
    CxxIOFuncs.insert("putback");
    CxxIOFuncs.insert("getline");
    CxxIOFuncs.insert("ignore");
    CxxIOFuncs.insert("readsome");
    CxxIOFuncs.insert("sync");
    CxxIOFuncs.insert("open");
    CxxIOFuncs.insert("close");
    CxxIOFuncs.insert("operator<<");
    CxxIOFuncs.insert("operator>>");

    CxxIOTypes.insert("basic_ostream");
    CxxIOTypes.insert("basic_istream");
    CxxIOTypes.insert("basic_iostream");
    CxxIOTypes.insert("basic_ofstream");
    CxxIOTypes.insert("basic_istream");
    CxxIOTypes.insert("basic_fstream");
  }

  const llvm::TargetLibraryInfo &m_TLI;
  std::set<llvm::LibFunc::Func> IOLibFuncs;

  std::set<std::string> CxxIOFuncs;
  std::set<std::string> CxxIOTypes;
};

class ApplyIOAttributePass : public llvm::ModulePass {
public:
  static char ID;

  ApplyIOAttributePass() : llvm::ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // end of include guard: APPLYIOATTRIBUTEPASS_HPP
