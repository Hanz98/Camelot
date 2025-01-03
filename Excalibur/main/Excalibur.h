/*
 * Copyright 2024 Jan Filip
 *
 * Licensed under the MIT License. You may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: Excalibur.cpp
 * Author: Jan Filip
 *
 */

#ifndef EXCALIBUR_MAIN_EXCALIBUR_H_
#define EXCALIBUR_MAIN_EXCALIBUR_H_

#define EXCALIBUR_EXPORTS

#if defined(_WIN32) || defined(_WIN64)
#define EXCALIBUR_EXPORTS

#ifdef EXCALIBUR_EXPORTS
#define EXCALIBUR_API __declspec(dllexport)
#else
#define EXCALIBUR_API __declspec(dllimport)
#endif
#else
#ifdef EXCALIBUR_EXPORTS
#define EXCALIBUR_API __attribute__((visibility("default")))
#else
#define EXCALIBUR_API
#endif
#endif

#include <Camelot/API/main/ICamelot.h>

typedef void (*Camelot_Init_Func)(ICamelot **camelot);

extern "C" EXCALIBUR_API void buildCamelot(ICamelot **camelot);

#endif  // EXCALIBUR_MAIN_EXCALIBUR_H_
