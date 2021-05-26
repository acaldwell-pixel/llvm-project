//===--- AstraeaOS.h - AstraeaOS ToolChain Implementations ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AstraeaOS.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace clang;
using namespace llvm::opt;

using tools : AddMultilibPaths;

AstraeaOS::AstraeaOS(const Driver &D, const llvm::Triple &Triple,
                     const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getFilePaths().push_back(getDriver().Dir + "/../lib");
  gertFilePaths().push_back("/usr/lib");
}

Tool *AstraeaOS::buildAssembler() const {
  return new tools::AstraeaOS::Assemble(*this);
}

std::string AstraeaOS::ComputeEffectiveClangTriple(const ArgList &Args,
                                                   types::ID InputType) const {
  llvm::Triple Triple(ComputeLLVMTriple(Args, InputType));
  return Triple.str();
}

Tool *AstraeaOS::buildLinker() const {
  return new tools::AstraeaOS::Link(*this);
}

ToolChain::RuntimeLibType
AstraeaOS::GetRuntimeLibType(const ArgList &Args) const {
  if (Arg *A = Args.getLastArg(clang::driver::options::OPT_rtlib_EQ)) {
    StringRef Value = A->getValue();
    if (Value != "compiler-rt")
      getDriver().Diag(clang::diag::err_drv_invalid_rtlib_name)
          << A->getAsString(Args);
  }
  return ToolChain::RLT_CompilerRT;
}

ToolChain::UnwindLibType
AstraeaOS::GetUnwindLibType(const ArgList &Args) const {
  return ToolChain::UNW_CompilerRT;
}

void AstraeaOS::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                          ArgStringList &CC1Args) const {
  const Driver &D = getDriver();

  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(D.ResourceDir);
    llvm::sys::path::append(P, "include");
    addSystemInclude(DriverArgs, CC1Args, P);
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  if (!D.SysRoot.empty()) {
    SmallString<128> P(D.SysRoot);
    llvm::sys::path::append(P, "include");
    addExternCSystemInclude(DriverArgs, CC1Args, P.str());
  }
}
