/**！
 Copyright (c) 2014-present, Facebook, Inc.
 All rights reserved.
 
 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef POP_POPDefines_h
#define POP_POPDefines_h

#import <Availability.h>

#ifdef __cplusplus
# define POP_EXTERN_C_BEGIN extern "C" {
# define POP_EXTERN_C_END   }
#else
# define POP_EXTERN_C_BEGIN
# define POP_EXTERN_C_END
#endif

//数组个数
#define POP_ARRAY_COUNT(x) sizeof(x) / sizeof(x[0])

#if defined (__cplusplus) && defined (__GNUC__)
# define POP_NOTHROW __attribute__ ((nothrow))
#else
# define POP_NOTHROW
#endif

//如果要使用SceneKit 添加POP_USE_SCENEKIT=1到预编译宏中
#if defined(POP_USE_SCENEKIT)
# if TARGET_OS_MAC || TARGET_OS_IPHONE
#  define SCENEKIT_SDK_AVAILABLE 1
# endif
#endif

#endif
