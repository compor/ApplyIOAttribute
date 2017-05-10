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
      m_IOLibFuncs.end() != m_IOLibFuncs.find(TLIFunc)) {
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
      std::begin(m_CxxIOFuncs), std::end(m_CxxIOFuncs), [&funcName](const auto &e) {
        return std::string::npos != funcName.find(e);
      });

  if (found1 == std::end(m_CxxIOFuncs))
    return false;

  const auto *classType = getClassFromMethod(*Func.getFunctionType());
  if (!classType)
    return false;

  const auto &classTypeName = classType->getStructName();
  const auto &found2 =
      std::find_if(std::begin(m_CxxIOTypes), std::end(m_CxxIOTypes),
                   [&classTypeName](const auto &e) {
                     return llvm::StringRef::npos != classTypeName.find(e);
                   });

  if (found2 == std::end(m_CxxIOTypes))
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

void ApplyIOAttribute::setupLibCIOFuncs() {
  // TODO what about calls that might have other side-effects
  // system()
  // getenv()
  // setenv()
  // unsetenv()
  m_IOLibFuncs.insert(llvm::LibFunc::under_IO_getc);
  m_IOLibFuncs.insert(llvm::LibFunc::under_IO_putc);
  m_IOLibFuncs.insert(llvm::LibFunc::dunder_isoc99_scanf);
  m_IOLibFuncs.insert(llvm::LibFunc::access);
  m_IOLibFuncs.insert(llvm::LibFunc::chmod);
  m_IOLibFuncs.insert(llvm::LibFunc::chown);
  m_IOLibFuncs.insert(llvm::LibFunc::closedir);
  m_IOLibFuncs.insert(llvm::LibFunc::fclose);
  m_IOLibFuncs.insert(llvm::LibFunc::fdopen);
  m_IOLibFuncs.insert(llvm::LibFunc::feof);
  m_IOLibFuncs.insert(llvm::LibFunc::ferror);
  m_IOLibFuncs.insert(llvm::LibFunc::fflush);
  m_IOLibFuncs.insert(llvm::LibFunc::fgetc);
  m_IOLibFuncs.insert(llvm::LibFunc::fgetpos);
  m_IOLibFuncs.insert(llvm::LibFunc::fgets);
  m_IOLibFuncs.insert(llvm::LibFunc::fileno);
  m_IOLibFuncs.insert(llvm::LibFunc::fiprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::flockfile);
  m_IOLibFuncs.insert(llvm::LibFunc::fopen);
  m_IOLibFuncs.insert(llvm::LibFunc::fopen64);
  m_IOLibFuncs.insert(llvm::LibFunc::fprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::fputc);
  m_IOLibFuncs.insert(llvm::LibFunc::fputs);
  m_IOLibFuncs.insert(llvm::LibFunc::fread);
  m_IOLibFuncs.insert(llvm::LibFunc::fscanf);
  m_IOLibFuncs.insert(llvm::LibFunc::fseek);
  m_IOLibFuncs.insert(llvm::LibFunc::fseeko);
  m_IOLibFuncs.insert(llvm::LibFunc::fseeko64);
  m_IOLibFuncs.insert(llvm::LibFunc::fsetpos);
  m_IOLibFuncs.insert(llvm::LibFunc::fstatvfs);
  m_IOLibFuncs.insert(llvm::LibFunc::fstatvfs64);
  m_IOLibFuncs.insert(llvm::LibFunc::ftell);
  m_IOLibFuncs.insert(llvm::LibFunc::ftello);
  m_IOLibFuncs.insert(llvm::LibFunc::ftello64);
  m_IOLibFuncs.insert(llvm::LibFunc::ftrylockfile);
  m_IOLibFuncs.insert(llvm::LibFunc::funlockfile);
  m_IOLibFuncs.insert(llvm::LibFunc::fwrite);
  m_IOLibFuncs.insert(llvm::LibFunc::getc);
  m_IOLibFuncs.insert(llvm::LibFunc::getc_unlocked);
  m_IOLibFuncs.insert(llvm::LibFunc::getchar);
  m_IOLibFuncs.insert(llvm::LibFunc::getlogin_r);
  m_IOLibFuncs.insert(llvm::LibFunc::getpwnam);
  m_IOLibFuncs.insert(llvm::LibFunc::gets);
  m_IOLibFuncs.insert(llvm::LibFunc::iprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::lchown);
  m_IOLibFuncs.insert(llvm::LibFunc::lstat);
  m_IOLibFuncs.insert(llvm::LibFunc::lstat64);
  m_IOLibFuncs.insert(llvm::LibFunc::mkdir);
  m_IOLibFuncs.insert(llvm::LibFunc::open);
  m_IOLibFuncs.insert(llvm::LibFunc::open64);
  m_IOLibFuncs.insert(llvm::LibFunc::opendir);
  m_IOLibFuncs.insert(llvm::LibFunc::pclose);
  m_IOLibFuncs.insert(llvm::LibFunc::perror);
  m_IOLibFuncs.insert(llvm::LibFunc::popen);
  m_IOLibFuncs.insert(llvm::LibFunc::pread);
  m_IOLibFuncs.insert(llvm::LibFunc::printf);
  m_IOLibFuncs.insert(llvm::LibFunc::putc);
  m_IOLibFuncs.insert(llvm::LibFunc::putchar);
  m_IOLibFuncs.insert(llvm::LibFunc::puts);
  m_IOLibFuncs.insert(llvm::LibFunc::pwrite);
  m_IOLibFuncs.insert(llvm::LibFunc::read);
  m_IOLibFuncs.insert(llvm::LibFunc::readlink);
  m_IOLibFuncs.insert(llvm::LibFunc::realpath);
  m_IOLibFuncs.insert(llvm::LibFunc::remove);
  m_IOLibFuncs.insert(llvm::LibFunc::rename);
  m_IOLibFuncs.insert(llvm::LibFunc::rewind);
  m_IOLibFuncs.insert(llvm::LibFunc::rmdir);
  m_IOLibFuncs.insert(llvm::LibFunc::scanf);
  m_IOLibFuncs.insert(llvm::LibFunc::siprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::stat);
  m_IOLibFuncs.insert(llvm::LibFunc::stat64);
  m_IOLibFuncs.insert(llvm::LibFunc::statvfs);
  m_IOLibFuncs.insert(llvm::LibFunc::statvfs64);
  m_IOLibFuncs.insert(llvm::LibFunc::tmpfile);
  m_IOLibFuncs.insert(llvm::LibFunc::tmpfile64);
  m_IOLibFuncs.insert(llvm::LibFunc::ungetc);
  m_IOLibFuncs.insert(llvm::LibFunc::unlink);
  m_IOLibFuncs.insert(llvm::LibFunc::vfprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::vfscanf);
  m_IOLibFuncs.insert(llvm::LibFunc::vprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::vscanf);
  m_IOLibFuncs.insert(llvm::LibFunc::vsnprintf);
  m_IOLibFuncs.insert(llvm::LibFunc::vsscanf);
  m_IOLibFuncs.insert(llvm::LibFunc::write);

  return;
}

void ApplyIOAttribute::setupCxxIOFuncs() {
  m_CxxIOFuncs.push_back("put");
  m_CxxIOFuncs.push_back("write");
  m_CxxIOFuncs.push_back("flush");
  m_CxxIOFuncs.push_back("get");
  m_CxxIOFuncs.push_back("peek");
  m_CxxIOFuncs.push_back("unget");
  m_CxxIOFuncs.push_back("putback");
  m_CxxIOFuncs.push_back("getline");
  m_CxxIOFuncs.push_back("ignore");
  m_CxxIOFuncs.push_back("readsome");
  m_CxxIOFuncs.push_back("sync");
  m_CxxIOFuncs.push_back("open");
  m_CxxIOFuncs.push_back("close");
  m_CxxIOFuncs.push_back("operator<<");
  m_CxxIOFuncs.push_back("operator>>");

  // ctors
  m_CxxIOFuncs.push_back("basic_ostream");
  m_CxxIOFuncs.push_back("basic_istream");
  m_CxxIOFuncs.push_back("basic_iostream");
  m_CxxIOFuncs.push_back("basic_ofstream");
  m_CxxIOFuncs.push_back("basic_istream");
  m_CxxIOFuncs.push_back("basic_fstream");

  m_CxxIOTypes.push_back("basic_ostream");
  m_CxxIOTypes.push_back("basic_istream");
  m_CxxIOTypes.push_back("basic_iostream");
  m_CxxIOTypes.push_back("basic_ofstream");
  m_CxxIOTypes.push_back("basic_istream");
  m_CxxIOTypes.push_back("basic_fstream");
}

} // namespace icsa end
