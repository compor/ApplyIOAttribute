//
//
//

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

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include <cxxabi.h>
// using abi::__cxa_demangle

#include <cstdlib>
// using std::free

#include <algorithm>
// using std::find_if

#include <memory>
// using std::unique_ptr

#include "ApplyIOAttribute.hpp"

template <typename T> struct rm_const_ptr { using type = T; };
template <typename T> struct rm_const_ptr<const T *> { using type = T *; };
template <typename T> using rm_const_ptr_t = typename rm_const_ptr<T>::type;

struct malloc_deleter {
  template <typename T> void operator()(T *ptr) const { std::free(ptr); }
};

template <typename T>
using malloc_unique_ptr = std::unique_ptr<T, malloc_deleter>;

namespace icsa {

bool ApplyIOAttribute::hasIO(const llvm::Function &Func) const {
  for (const auto &bb : Func)
    for (const auto &inst : bb) {
      const auto *calledFunc = getCalledFunction(inst);
      if (calledFunc)
        return hasCIO(*calledFunc) || hasCxxIO(*calledFunc);
    }

  return false;
}

bool ApplyIOAttribute::apply(llvm::Function &func) const {
  func.addFnAttr(this->getIOAttr());

  return true;
}

//
// private methods
//

bool ApplyIOAttribute::hasCIO(const llvm::Function &Func) const {
  if (!Func.hasName())
    return false;

  const auto &funcName = Func.getName();
  llvm::LibFunc::Func TLIFunc;

  if (m_TLI.getLibFunc(funcName, TLIFunc) && m_TLI.has(TLIFunc) &&
      IOLibFuncs.end() != IOLibFuncs.find(TLIFunc)) {
    return true;
  }

  return false;
}

llvm::Type *
ApplyIOAttribute::getClassFromMethod(const llvm::FunctionType &FuncType) const {
  const auto *potentialClassTypePtr = FuncType.getParamType(0);
  if (!potentialClassTypePtr->isPointerTy())
    return nullptr;

  const auto *classType =
      llvm::dyn_cast<llvm::PointerType>(potentialClassTypePtr)
          ->getElementType();

  if (!classType->isStructTy())
    return nullptr;

  return rm_const_ptr_t<decltype(classType)>(classType);
}

std::string ApplyIOAttribute::demangleCxxName(const char *name) const {
  std::string demangledName{""};
  auto status = 0;

  const auto &demangledNamePtr =
      malloc_unique_ptr<char>(abi::__cxa_demangle(name, 0, 0, &status));

  if (!status)
    demangledName = demangledNamePtr.get();

  return demangledName;
}

bool ApplyIOAttribute::hasCxxIO(const llvm::Function &Func) const {
  if (Func.getFunctionType()->getNumParams() < 1 || !Func.hasName())
    return false;

  const auto &funcName = demangleCxxName(Func.getName().data());
  const auto &found1 = std::find_if(
      std::begin(CxxIOFuncs), std::end(CxxIOFuncs), [&funcName](const auto &e) {
        return std::string::npos != funcName.find(e);
      });

  if (found1 == std::end(CxxIOFuncs))
    return false;

  const auto *classType = getClassFromMethod(*Func.getFunctionType());
  if (!classType)
    return false;

  const auto &classTypeName = classType->getStructName();
  const auto &found2 =
      std::find_if(std::begin(CxxIOTypes), std::end(CxxIOTypes),
                   [&classTypeName](const auto &e) {
                     return llvm::StringRef::npos != classTypeName.find(e);
                   });

  if (found2 == std::end(CxxIOTypes))
    return false;

  return true;
}

llvm::Function *
ApplyIOAttribute::getCalledFunction(const llvm::Instruction &Inst) const {
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

void ApplyIOAttribute::setupIOFuncs() {
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

void ApplyIOAttribute::setupCxxIOFuncs() {
  CxxIOFuncs.push_back("put");
  CxxIOFuncs.push_back("write");
  CxxIOFuncs.push_back("flush");
  CxxIOFuncs.push_back("get");
  CxxIOFuncs.push_back("peek");
  CxxIOFuncs.push_back("unget");
  CxxIOFuncs.push_back("putback");
  CxxIOFuncs.push_back("getline");
  CxxIOFuncs.push_back("ignore");
  CxxIOFuncs.push_back("readsome");
  CxxIOFuncs.push_back("sync");
  CxxIOFuncs.push_back("open");
  CxxIOFuncs.push_back("close");
  CxxIOFuncs.push_back("operator<<");
  CxxIOFuncs.push_back("operator>>");

  // ctors
  CxxIOFuncs.push_back("basic_ostream");
  CxxIOFuncs.push_back("basic_istream");
  CxxIOFuncs.push_back("basic_iostream");
  CxxIOFuncs.push_back("basic_ofstream");
  CxxIOFuncs.push_back("basic_istream");
  CxxIOFuncs.push_back("basic_fstream");

  CxxIOTypes.push_back("basic_ostream");
  CxxIOTypes.push_back("basic_istream");
  CxxIOTypes.push_back("basic_iostream");
  CxxIOTypes.push_back("basic_ofstream");
  CxxIOTypes.push_back("basic_istream");
  CxxIOTypes.push_back("basic_fstream");
}

} // namespace icsa end