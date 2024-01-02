/*
 *  Copyright (c) 2023 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* GENERATED FILE: DO NOT EDIT! */

#ifndef VPX_FRAMEWORK_HEADERS_VPX_VPX_CONFIG_H_
#define VPX_FRAMEWORK_HEADERS_VPX_VPX_CONFIG_H_

#if defined __aarch64__
#define VPX_FRAMEWORK_TARGET "arm64-darwin-gcc"
#include "VPX/vpx/arm64-darwin-gcc/vpx_config.h"
#elif defined __ARM_ARCH_7A__
#define VPX_FRAMEWORK_TARGET "armv7-darwin-gcc"
#include "VPX/vpx/armv7-darwin-gcc/vpx_config.h"
#elif defined __ARM_ARCH_7S__
#define VPX_FRAMEWORK_TARGET "armv7s-darwin-gcc"
#include "VPX/vpx/armv7s-darwin-gcc/vpx_config.h"
#elif defined __i386__
#define VPX_FRAMEWORK_TARGET "x86-iphonesimulator-gcc"
#include "VPX/vpx/x86-iphonesimulator-gcc/vpx_config.h"
#elif defined __x86_64__
#define VPX_FRAMEWORK_TARGET "x86_64-iphonesimulator-gcc"
#include "VPX/vpx/x86_64-iphonesimulator-gcc/vpx_config.h"
#endif

#endif  // VPX_FRAMEWORK_HEADERS_VPX_VPX_CONFIG_H_