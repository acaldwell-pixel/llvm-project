//===--- AstraeaOS.h - AstraeaOS ToolChain Implementations ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AstraeaOS.h"
#include "CommonArgs.h"
#include "InputInfo.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Options.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Path.h"

using namespace clang::driver;
using namespace clang::driver::tools;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

void astraeaos::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                     const InputInfo &Output,
                                     const InputInfoList &Inputs,
                                     const ArgList &Args,
                                     const char *LinkingOutput) const {
  const toolchains::AstraeaOS &ToolChain =
      static_cast<const toolchains::AstraeaOS &>(getToolChain());
  const Driver &D = ToolChain.getDriver();

  ArgStringList CmdArgs;

  Args.ClaimAllArgs(options::OPT_g_Group);
  Args.ClaimAllArgs(options::OPT_emit_llvm);
  Args.ClaimAllArgs(options::OPT_w);

  CmdArgs.push_back("-z");
  CmdArgs.push_back("now");

  const char *Exec = Args.MakeArgString(ToolChain.GetLinkerPath());
  if (llvm::sys::path::filename(Exec).equals_lower("ld.lld") ||
      llvm::sys::path::stem(Exec).equals_lower("ld.lld")) {
    CmdArgs.push_back("-z");
    CmdArgs.push_back("rodynamic");
    CmdArgs.push_back("-z");
    CmdArgs.push_back("separate-loadable-segments");
    CmdArgs.push_back("--pack-dyn-relocs=relr");
  }

  if (!D.SysRoot.empty())
    CmdArgs.push_back(Args.MakeArgString("--sysroot=" + D.SysRoot));

  if (!Args.hasArg(options::OPT_shared) && !Args.hasArg(options::OPT_r))
    CmdArgs.push_back("-pie");

  if (Args.hasArg(options::OPT_rdynamic))
    CmdArgs.push_back("-export-dynamic");

  if (Args.hasArg(options::OPT_s))
    CmdArgs.push_back("-s");

  if (Args.hasArg(options::OPT_r)) {
    CmdArgs.push_back("-r");
  } else {
    CmdArgs.push_back("--build-id");
    //    CmdArgs.push_back("--hash-style=gnu");
  }

  CmdArgs.push_back("--eh-frame-hdr");

  if (Args.hasArg(options::OPT_static))
    CmdArgs.push_back("-Bstatic");
  else if (Args.hasArg(options::OPT_shared))
    CmdArgs.push_back("-shared");

  const SanitizerArgs &SanArgs = ToolChain.getSanitizerArgs();

  if (!Args.hasArg(options::OPT_shared)) {
    std::string Dyld = D.DyldPrefix;
    if (SanArgs.needsAsanRt() && SanArgs.needsSharedRt())
      Dyld += "asan/";
    if (SanArgs.needsHwasanRt() && SanArgs.needsSharedRt())
      Dyld += "hwasan/";
    if (SanArgs.needsTsanRt() && SanArgs.needsSharedRt())
      Dyld += "tsan/";
    Dyld += "ld.so.1";
    CmdArgs.push_back("-dynamic-linker");
    CmdArgs.push_back(Args.MakeArgString(Dyld));
  }

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
    if (!Args.hasArg(options::OPT_shared)) {
      CmdArgs.push_back(Args.MakeArgString(ToolChain.GetFilePath("Scrt1.o")));
    }
  }

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  Args.AddAllArgs(CmdArgs, options::OPT_u);

  ToolChain.AddFilePathLibArgs(Args, CmdArgs);

  if (D.isUsingLTO()) {
    assert(!Inputs.empty() && "Must have at least one input.");
    addLTOOptions(ToolChain, Args, CmdArgs, Output, Inputs[0],
                  D.getLTOMode() == LTOK_Thin);
  }

  bool NeedsSanitizerDeps = addSanitizerRuntimes(ToolChain, Args, CmdArgs);
  bool NeedsXRayDeps = addXRayRuntime(ToolChain, Args, CmdArgs);
  AddLinkerInputs(ToolChain, Inputs, Args, CmdArgs, JA);
  ToolChain.addProfileRTLibs(Args, CmdArgs);

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    if (Args.hasArg(options::OPT_static))
      CmdArgs.push_back("-Bdynamic");

    if (D.CCCIsCXX()) {
      if (ToolChain.ShouldLinkCXXStdlib(Args)) {
        bool OnlyLibstdcxxStatic = Args.hasArg(options::OPT_static_libstdcxx) &&
                                   !Args.hasArg(options::OPT_static);
        CmdArgs.push_back("--push-state");
        CmdArgs.push_back("--as-needed");
        if (OnlyLibstdcxxStatic)
          CmdArgs.push_back("-Bstatic");
        ToolChain.AddCXXStdlibLibArgs(Args, CmdArgs);
        if (OnlyLibstdcxxStatic)
          CmdArgs.push_back("-Bdynamic");
        CmdArgs.push_back("-lm");
        CmdArgs.push_back("--pop-state");
      }
    }

    if (NeedsSanitizerDeps)
      linkSanitizerRuntimeDeps(ToolChain, CmdArgs);

    //    if (NeedsXRayDeps)
    //      linkXRayRuntimeDeps(ToolChain, CmdArgs);

    AddRunTimeLibs(ToolChain, D, CmdArgs, Args);

    if (Args.hasArg(options::OPT_pthread) || Args.hasArg(options::OPT_pthreads))
      CmdArgs.push_back("-lpthread");

    if (Args.hasArg(options::OPT_fsplit_stack))
      CmdArgs.push_back("--wrap=pthread_create");

    if (!Args.hasArg(options::OPT_nolibc))
      CmdArgs.push_back("-lc");
  }

  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         Exec, CmdArgs, Inputs, Output));
}

/// AstraeaoS - AstraeaOS tool chain which can call as(1) and ld(1) directly.
AstraeaOS::AstraeaOS(const Driver &D, const llvm::Triple &Triple,
                     const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getProgramPaths().push_back(getDriver().getInstalledDir());
  if (getDriver().getInstalledDir() != D.Dir)
    getProgramPaths().push_back(D.Dir);

  if (!D.SysRoot.empty()) {
    SmallString<128> P(D.SysRoot);
    llvm::sys::path::append(P, "lib");
    getFilePaths().push_back(std::string(P.str()));
  }

  std::string AstraeaOS::ComputeEffectiveClangTriple(
      const ArgList &Args, types::ID InputType) const {
    llvm::Triple Triple(ComputeLLVMTriple(Args, InputType));
    return Triple.str();
  }

  Tool *AstraeaOS::buildLinker() const {
    return new tools::astraeaos::Linker(*this);
  }

  ToolChain::RuntimeLibType Fuchsia::GetRuntimeLibType(const ArgList &Args)
      const {
    if (Arg *A = Args.getLastArg(clang::driver::options::OPT_rtlib_EQ)) {
      StringRef Value = A->getValue();
      if (Value != "compiler-rt")
        getDriver().Diag(clang::diag::err_drv_invalid_rtlib_name)
            << A->getAsString(Args);
    }

    return ToolChain::RLT_CompilerRT;
  }

  ToolChain::CXXStdlibType Fuchsia::GetCXXStdlibType(const ArgList &Args)
      const {
    if (Arg *A = Args.getLastArg(options::OPT_stdlib_EQ)) {
      StringRef Value = A->getValue();
      if (Value != "libc++")
        getDriver().Diag(diag::err_drv_invalid_stdlib_name)
            << A->getAsString(Args);
    }

    return ToolChain::CST_Libcxx;
  }

  void AstraeaOS::addClangTargetOptions(const ArgList &DriverArgs,
                                        ArgStringList &CC1Args,
                                        Action::OffloadKind) const {
    if (!DriverArgs.hasFlag(options::OPT_fuse_init_array,
                            options::OPT_fno_use_init_array, true))
      CC1Args.push_back("-fno-use-init-array");
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

    // Check for configure-time C include directories.
    StringRef CIncludeDirs(C_INCLUDE_DIRS);
    if (CIncludeDirs != "") {
      SmallVector<StringRef, 5> dirs;
      CIncludeDirs.split(dirs, ":");
      for (StringRef dir : dirs) {
        StringRef Prefix =
            llvm::sys::path::is_absolute(dir) ? "" : StringRef(D.SysRoot);
        addExternCSystemInclude(DriverArgs, CC1Args, Prefix + dir);
      }
      return;
    }

    if (!D.SysRoot.empty()) {
      SmallString<128> P(D.SysRoot);
      llvm::sys::path::append(P, "include");
      addExternCSystemInclude(DriverArgs, CC1Args, P.str());
    }
  }

  void AstraeaOS::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs,
                                               ArgStringList &CC1Args) const {
    if (DriverArgs.hasArg(options::OPT_nostdlibinc) ||
        DriverArgs.hasArg(options::OPT_nostdincxx))
      return;

    const Driver &D = getDriver();
    std::string Target = getTripleString();

    auto AddCXXIncludePath = [&](StringRef Path) {
      std::string Version = detectLibcxxVersion(Path);
      if (Version.empty())
        return;

      // First add the per-target include path.
      SmallString<128> TargetDir(Path);
      llvm::sys::path::append(TargetDir, Target, "c++", Version);
      if (getVFS().exists(TargetDir))
        addSystemInclude(DriverArgs, CC1Args, TargetDir);

      // Second add the generic one.
      SmallString<128> Dir(Path);
      llvm::sys::path::append(Dir, "c++", Version);
      addSystemInclude(DriverArgs, CC1Args, Dir);
    };

    switch (GetCXXStdlibType(DriverArgs)) {
    case ToolChain::CST_Libcxx: {
      SmallString<128> P(D.Dir);
      llvm::sys::path::append(P, "..", "include");
      AddCXXIncludePath(P);
      break;
    }

    default:
      llvm_unreachable("invalid stdlib name");
    }
  }

  void AstraeaOS::AddCXXStdlibLibArgs(const ArgList &Args,
                                      ArgStringList &CmdArgs) const {
    switch (GetCXXStdlibType(Args)) {
    case ToolChain::CST_Libcxx:
      CmdArgs.push_back("-lc++");
      break;

    case ToolChain::CST_Libstdcxx:
      llvm_unreachable("invalid stdlib name");
    }
  }

  SanitizerMask AstraeaOS::getSupportedSanitizers() const {
    SanitizerMask Res = ToolChain::getSupportedSanitizers();
    Res |= SanitizerKind::Address;
    Res |= SanitizerKind::HWAddress;
    Res |= SanitizerKind::PointerCompare;
    Res |= SanitizerKind::PointerSubtract;
    Res |= SanitizerKind::Fuzzer;
    Res |= SanitizerKind::FuzzerNoLink;
    Res |= SanitizerKind::Leak;
    Res |= SanitizerKind::SafeStack;
    Res |= SanitizerKind::Scudo;
    Res |= SanitizerKind::Thread;
    return Res;
  }

  SanitizerMask Fuchsia::getDefaultSanitizers() const {
    SanitizerMask Res;
    switch (getTriple().getArch()) {
    case llvm::Triple::x86_64:
      Res |= SanitizerKind::SafeStack;
      break;
    default:
      // TODO: Enable SafeStack on RISC-V once tested.
      break;
    }
    return Res;
  }

  void AstraeaOS::addProfileRTLibs(const llvm::opt::ArgList &Args,
                                 llvm::opt::ArgStringList &CmdArgs) const {
    // Add linker option -u__llvm_profile_runtime to cause runtime
    // initialization module to be linked in.
    if (needsProfileRT(Args))
      CmdArgs.push_back(Args.MakeArgString(
          Twine("-u", llvm::getInstrProfRuntimeHookVarName())));
    ToolChain::addProfileRTLibs(Args, CmdArgs);
  }
}