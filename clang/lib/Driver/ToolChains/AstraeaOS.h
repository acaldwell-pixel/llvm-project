//===--- AstraeaOS.h - AstraeaOS ToolChain Implementations ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H

#include "Gnu.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace tools {
namespace AstraeaOS {
class LLVM_LIBRARY_VISIBILITY Linker : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("astraeaos::Linker", "ld.lld", TC) {}

  bool hasIntegratedCPP() const override;
  bool isLinkJob() const override;
  bool isMathErrnoDefault() const override;
  bool useRelaxRelocations() const override;
  RuntimeLibType GetDefaultRuntimeLibType() const override {
    return ToolChain::RLT_CompilerRT;
  }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
}
} // namespace AstraeaOS
} // namespace tools
} // namespace driver
} // namespace clang

namespace toolchains {
class LLVM_LIBRARY_VISIBILITY AstraeaOS : public ToolChain {
public:
  AstraeaOS(const Driver &D, const llvm::Triple &Triple,
            const llvm::opt::ArgList &Args);

  bool HasNativeLLVMSupport() const override;
  bool IsIntegratedAssemblerDefault() const override;

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;

  void
  addLibStdCxxIncludePaths(const llvm::opt::ArgList &DriverArgs,
                           llvm::opt::ArgStringList &CC1Args) const override;

  RuntimeLibType
  GetRuntimeLibType(const llvm::opt::ArgList &Args) const override;
  UnwindLibType GetUnwindLibType(const llvm::opt::ArgList &Args) const override;

  void
  addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                        llvm::opt::ArgStringList &CC1Args,
                        Action::OffloadKind DeviceOffloadKind) const override;

  const char *getDefaultLinker() const override{return "ld.lld"};

protected:
  Tool *buildAssembler() const override;
  Tool *builderLinker() const override;
}
} // namespace toolchains
#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H