/********************************************************************************
 *
 * Copyright (c) 2018-2022 Advanced Micro Devices, Inc. All rights reserved.
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#ifndef RVS_INCLUDE_RVSMODULE_IF1_H_
#define RVS_INCLUDE_RVSMODULE_IF1_H_

extern "C" {

extern  const char* rvs_module_get_errstring(int error);
extern  int   rvs_module_action_property_set(void* Action, const char* Key,
                                             const char* Val);
extern  int   rvs_module_action_run(void* Action);

// define function pointer types to ease late binding usage
typedef const char* (*t_rvs_module_get_errstring)(int error);
typedef int   (*t_rvs_module_action_property_set)(void* Action, const char* Key,
                                                  const char* Val);
typedef int   (*t_rvs_module_action_run)(void* Action);

}

#endif  // RVS_INCLUDE_RVSMODULE_IF1_H_
