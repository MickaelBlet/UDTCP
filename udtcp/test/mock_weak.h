/**
 * Mock Weak
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * Copyright (c) 2019 BLET Mickaël.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _MOCK_WEAK_H_
# define _MOCK_WEAK_H_

# ifdef MOCK_WEAK_DLFCN
# include <dlfcn.h>
# endif
# include <gmock/gmock.h>

# ifndef __GNUC__
#  ifndef __attribute__
#   define __attribute__(X) /* do nothing */
#  endif
# endif

# define MOCK_WEAK_PRIMITIVE_CAT_(x, y) x ## y
# define MOCK_WEAK_CAT_(x, y) MOCK_WEAK_PRIMITIVE_CAT_(x, y)

# define MOCK_WEAK_REAPEAT_(N, ...) MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, N), _)(__VA_ARGS__)
# define MOCK_WEAK_REAPEAT_0_(M, ...)
# define MOCK_WEAK_REAPEAT_1_(M, ...) M(1,__VA_ARGS__)
# define MOCK_WEAK_REAPEAT_2_(M, ...) MOCK_WEAK_REAPEAT_1_(M, __VA_ARGS__), M(2, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_3_(M, ...) MOCK_WEAK_REAPEAT_2_(M, __VA_ARGS__), M(3, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_4_(M, ...) MOCK_WEAK_REAPEAT_3_(M, __VA_ARGS__), M(4, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_5_(M, ...) MOCK_WEAK_REAPEAT_4_(M, __VA_ARGS__), M(5, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_6_(M, ...) MOCK_WEAK_REAPEAT_5_(M, __VA_ARGS__), M(6, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_7_(M, ...) MOCK_WEAK_REAPEAT_6_(M, __VA_ARGS__), M(7, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_8_(M, ...) MOCK_WEAK_REAPEAT_7_(M, __VA_ARGS__), M(8, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_9_(M, ...) MOCK_WEAK_REAPEAT_8_(M, __VA_ARGS__), M(9, __VA_ARGS__)
# define MOCK_WEAK_REAPEAT_10_(M, ...) MOCK_WEAK_REAPEAT_9_(M, __VA_ARGS__), M(10, __VA_ARGS__)

# define MOCK_WEAK_ARG_NAME_(N, ...) _##N
# define MOCK_WEAK_ARG_DECLARATION_(N, ...) GMOCK_ARG_(, N, __VA_ARGS__) MOCK_WEAK_ARG_NAME_(N)

# ifdef MOCK_WEAK_DLFCN
#  define MOCK_WEAK_(_lvl, _name, ...) \
struct MOCK_WEAK_CAT_(Mock_weak_, _name) { \
  typedef GMOCK_RESULT_(,__VA_ARGS__) (*func_t)(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_DECLARATION_, __VA_ARGS__)); \
  MOCK_WEAK_CAT_(Mock_weak_, _name)() : isActive(true), real((func_t)dlsym(RTLD_NEXT, #_name)) {} \
  MOCK_WEAK_CAT_(MOCK_METHOD, _lvl)(_name, __VA_ARGS__); \
  static Mock_weak_##_name &instance() { static Mock_weak_##_name singleton; return singleton; } \
  bool isActive; \
  func_t real; \
}; \
GMOCK_RESULT_(,__VA_ARGS__) __attribute__((weak)) _name(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_DECLARATION_, __VA_ARGS__)) { \
  if (Mock_weak_##_name::instance().isActive) \
    return Mock_weak_##_name::instance()._name(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_NAME_, __VA_ARGS__)); \
  return Mock_weak_##_name::instance().real(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_NAME_, __VA_ARGS__)); \
}
#  define MOCK_WEAK_ENABLE(_name) MOCK_WEAK_CAT_(Mock_weak_, _name)::instance().isActive = true;
#  define MOCK_WEAK_DISABLE(_name) MOCK_WEAK_CAT_(Mock_weak_, _name)::instance().isActive = false;
# else
#  define MOCK_WEAK_(_lvl, _name, ...) \
struct MOCK_WEAK_CAT_(Mock_weak_, _name) { \
  MOCK_WEAK_CAT_(MOCK_METHOD, _lvl)(_name, __VA_ARGS__); \
  static Mock_weak_##_name &instance() { static Mock_weak_##_name singleton; return singleton; } \
}; \
__attribute__((weak)) \
GMOCK_RESULT_(,__VA_ARGS__) _name(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_DECLARATION_, __VA_ARGS__)) { \
  return Mock_weak_##_name::instance()._name(MOCK_WEAK_CAT_(MOCK_WEAK_CAT_(MOCK_WEAK_REAPEAT_, _lvl), _)(MOCK_WEAK_ARG_NAME_, __VA_ARGS__)); \
}
# endif

# define MOCK_WEAK_INSTANCE(_name) MOCK_WEAK_CAT_(Mock_weak_, _name)::instance()

# define MOCK_WEAK_METHOD0(_name, ...) MOCK_WEAK_(0, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD1(_name, ...) MOCK_WEAK_(1, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD2(_name, ...) MOCK_WEAK_(2, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD3(_name, ...) MOCK_WEAK_(3, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD4(_name, ...) MOCK_WEAK_(4, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD5(_name, ...) MOCK_WEAK_(5, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD6(_name, ...) MOCK_WEAK_(6, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD7(_name, ...) MOCK_WEAK_(7, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD8(_name, ...) MOCK_WEAK_(8, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD9(_name, ...) MOCK_WEAK_(9, _name, __VA_ARGS__)
# define MOCK_WEAK_METHOD10(_name, ...) MOCK_WEAK_(10, _name, __VA_ARGS__)

# define MOCK_WEAK_DECLTYPE_METHOD0(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(0, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD1(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(1, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD2(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(2, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD3(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(3, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD4(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(4, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD5(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(5, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD6(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(6, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD7(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(7, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD8(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(8, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD9(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(9, _name, decltype_##_name)
# define MOCK_WEAK_DECLTYPE_METHOD10(_name) using decltype_##_name = decltype(_name); MOCK_WEAK_(10, _name, decltype_##_name)

# define MOCK_WEAK_EXPECT_CALL(_name, _params) EXPECT_CALL(MOCK_WEAK_INSTANCE(_name), _name _params)

#endif // _MOCK_WEAK_H_