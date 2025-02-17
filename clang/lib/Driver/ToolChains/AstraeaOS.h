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
namespace astraeaos {
class LLVM_LIBRARY_VISIBILITY Linker : public Tool {
public:
  Linker(const ToolChain &TC) : Tool("astraeaos::Linker", "ld.lld", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};
class LLVM_LIBRARY_VISIBILITY Linker : public ToolChain {
public:
  Linker(const ToolChain &TC) : Tool("astraeaos::Linker", "linker", TC) {}

  bool hasIntegratedCPP() const override { return false; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};
} // end namespace astraeaos
} // namespace tools

namespace toolchains {
class LLVM_LIBRARY_VISIBILITY AstraeaOS : public ToolChain {
public:
  AstraeaOS(const Driver &D, const llvm::Triple &Triple,
            const llvm::opt::ArgList &Args);

  bool HasNativeLLVMSupport() const override { return true; }
  bool isIntegratedAssemblerDefault() const override { return true; }
  bool isMathErrnoDefault() const override { return false; }
  bool useRelaxRelocations() const override { return true; }
  RuntimeLibType GetDefaultRuntimeLibType() const override{
      return ToolChain::RLT_CompilerRT};
  CXXStdlibType GetDefaultCXXStdlibType() const override {
    return ToolChain::CST_Libcxx;
  };

  bool isUnwindTablesDefault(const llvm::opt::ArgList &Args) const override {
    return true;
  }
  bool isPICDefault() const override { return false; }
  bool isPIEDefault() const override { return true; }
  bool isNoExecStackDefault() const override { return true; }
  void addProfileRTLibs(const llvm::opt::ArgList &Args,
                        llvm::opt::ArgStringList &CmdArgs) const override;

  std::string getMultiarchTriple(const Driver &D,
                                 const llvm::Triple &TargetTriple,
                                 StringRef SysRoot) const override;

  RuntimeLibType
  GetRuntimeLibType(const llvm::opt::ArgList &Args) const override;
  CXXStdlibType GetCXXStdlibType(const llvm::opt::ArgList &Args) const override;

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;
  void
  addLibStdCxxIncludePaths(const llvm::opt::ArgList &DriverArgs,
                           llvm::opt::ArgStringList &CC1Args) const override;
  void AddCXXStdlibLibArgs(const llvm::opt::ArgList &Args,
                           llvm::opt::ArgStringList &CmdArgs) const override;

  const char *getDefaultLinker() const override { return "ld.lld"; }

  SanitizerMask getSupportedSanitizers() const override;
  SanitizerMask getDefaultSanitizers() const override;

protected:
  Tool *buildLinker() const override;
  Tool *buildStaticLibTool() const override;
};
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H