/*
 * Copyright (c) 2024 Jan Filip
 *
 * Licensed under the [Name of the License, e.g., MIT License, Apache
 * License 2.0]. You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
 *
 *     [URL to the License, e.g., https://opensource.org/licenses/MIT]
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: main.cpp
 * Author: Jan Filip
 * Description: test file
 *
 */

#ifndef CAMELOTLIB_CAMELOTLIB_CAMELOTINTERFACE_H_
#define CAMELOTLIB_CAMELOTLIB_CAMELOTINTERFACE_H_

#define CAMELOT_EXPORTS

#ifdef CAMELOT_EXPORTS
#define CAMELOT_API __declspec(dllexport)
#else
#define CAMELOT_API __declspec(ddllimport)
#endif

#include <Camelot/CamelotAPI/ICamelot.h>

typedef void (*Camelot_Init_Func)(ICamelot** camelot);

extern "C" CAMELOT_API void buildCamelot(ICamelot** camelot);

#endif  // CAMELOTLIB_CAMELOTLIB_CAMELOTINTERFACE_H_
