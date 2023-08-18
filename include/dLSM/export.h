// Copyright (c) 2017 The dLSM Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_dLSM_INCLUDE_EXPORT_H_
#define STORAGE_dLSM_INCLUDE_EXPORT_H_

#if !defined(dLSM_EXPORT)

#if defined(dLSM_SHARED_LIBRARY)
#if defined(_WIN32)

#if defined(dLSM_COMPILE_LIBRARY)
#define dLSM_EXPORT __declspec(dllexport)
#else
#define dLSM_EXPORT __declspec(dllimport)
#endif  // defined(dLSM_COMPILE_LIBRARY)

#else  // defined(_WIN32)
#if defined(dLSM_COMPILE_LIBRARY)
#define dLSM_EXPORT __attribute__((visibility("default")))
#else
#define dLSM_EXPORT
#endif
#endif  // defined(_WIN32)

#else  // defined(dLSM_SHARED_LIBRARY)
#define dLSM_EXPORT
#endif

#endif  // !defined(dLSM_EXPORT)

#endif  // STORAGE_dLSM_INCLUDE_EXPORT_H_
