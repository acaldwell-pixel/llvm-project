//===--- AstraeaOS.h - AstraeaOS ToolChain Implementations -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ASTRAEAOS_H

#include "Gnu.h"
#include "clang/Driver/Toolchain.h"

namespace clang {
  namespace driver {
    namespace toolchains {
      
class LLVM_LIBRARY_VISIBILITY AstraeaOS : public Generic_ELF {
  public:
    AstraeaOS(const Driver &D, const llvm::Triple &Triple,
              const llvm::opt::ArgList &ARgs);
  
  protected:
    Tool *buildAssembler() const override;
    Tool *buildLinker() const override;
};
