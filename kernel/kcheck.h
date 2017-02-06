/*
 * Copyright (C) 2001, 2002, Arien Malec
 * Copyright (C) 2011-2017, Knut Omang, Oracle Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *
 * This file originates from check.h from the Check C unit test
 * framework, then adapted to build with the linux kernel.
 */

#ifndef KCHECK_H
#define KCHECK_H

#include <net/netlink.h>
#include <linux/version.h>

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GCC_VERSION_AT_LEAST(major, minor) \
((__GNUC__ > (major)) || \
 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#if GCC_VERSION_AT_LEAST(2,95)
#define CK_ATTRIBUTE_UNUSED __attribute__ ((unused))
#else
#define CK_ATTRIBUTE_UNUSED
#endif /* GCC 2.95 */

void flush_assert_cnt(struct sk_buff* skb);

/* opaque type for a test case
 * A TCase represents a test case.  Create with tcase_create, free
 * with tcase_free.
 */
typedef struct TCase TCase;
struct test_dev;

/* Each module client of the test framework is require to
 * declare a test handle __test_handle via the macro TEST_INIT_HANDLE (below)
 */
struct test_handle;

/* This extern refers to the handle declared by the required call to
 * TEST_INIT_HANDLE (one per module)
 */
extern struct test_handle __test_handle;

/* type for a test function */
typedef void (*TFun) (struct sk_buff *, struct test_dev* tdev, int, u32);

struct __test_desc
{
	const char* tclass; /* Test class name */
	const char* name;   /* Test name */
	const char* file;   /* File that implements test */
	TFun fun;
};

/* This should really be a test device agnostic type.. */
struct sif_dev;

/* Create a test case */
TCase* tcase_create (const char *name);

TCase* tcase_find (const char *name);

/* Add a test function to a test case (macro version) */
#define tcase_add_test(td) \
	_tcase_add_test(td##_setup, &__test_handle, 0, 0, 0, 1)

/* Add a looping test function to a test case (macro version)

   The test will be called in a for(i = s; i < e; i++) loop with each
   iteration being executed in a new context. The loop variable 'i' is
   available in the test.
 */
#define tcase_add_loop_test(td,s,e)				\
	_tcase_add_test(td##_setup, &__test_handle, 0,0,(s),(e))

/* Add a test function to a test case
  (function version -- use this when the macro won't work
*/
void _tcase_add_test(struct __test_desc td, struct test_handle *th,
		int _signal, int allowed_exit_value, int start, int end);

/* Internal function to mark the start of a test function */
void tcase_fn_start (const char *fname, const char *file, int line);

/* Add a test previously created with TEST() or TEST_F() */
#define ADD_TEST(__testname)\
	tcase_add_test(__testname)

#define ADD_LOOP_TEST(__testname, from, to)			\
	tcase_add_loop_test(__testname, from, to)

/* Remove a test previously added with ADD_TEST */
#define DEL_TEST(__testname)\
	tcase_del_test(__testname)

/* A test_handle identifies the calling module:
 * Declare one in the module global scope using
 *  TEST_INIT_HANDLE()
 *  and call TEST_CLEANUP() upon unload
 */

struct test_handle {
	struct list_head test_list;
};

void _tcase_cleanup(struct test_handle *th);

#define TEST_INIT_HANDLE() \
	struct test_handle __test_handle = { \
		.test_list = LIST_HEAD_INIT(__test_handle.test_list) \
	};
#define TEST_CLEANUP() \
	_tcase_cleanup(&__test_handle)

/* Start a unit test with TEST(suite_name,unit_name)
*/
#define TEST(__testsuite, __testname)\
	static void __testname(struct sk_buff * skb,\
			struct test_dev *tdev,\
			int _i, u32 _value);		    \
	struct __test_desc __testname##_setup = \
        { .tclass = "" # __testsuite "", .name = "" # __testname "",\
	  .fun = __testname, .file = __FILE__ };					    \
	\
	static void __testname(struct sk_buff * skb, struct test_dev* tdev, \
			int _i, u32 _value)

/* Start a unit test using a fixture
 * NB! Note the intentionally missing start parenthesis on DECLARE_F!
 *   Prep:
 *      DECLARE_F(fixture_name)
 *            <attributes>
 *      };
 *      INIT_F(fixture_name,setup,teardown)
 *
 *   Usage:
 * 	TEST_F(fixture_name,unit_name)
 *      {
 *          <test code>
 *      }
 *
 *   setup must set ctx->ok to true to have the test itself executed
 */

#define DECLARE_F(__fixture)\
	struct __fixture {\
		void (*setup) (struct sk_buff *, struct test_dev*, struct __fixture*); \
		void (*teardown) (struct sk_buff *, struct __fixture*);\
		bool ok;

#define INIT_F(__fixture,__setup,__teardown) \
	static struct __fixture __fixture##_template = {\
		.setup = __setup, \
		.teardown = __teardown,	 \
		.ok = false,\
	}


#define TEST_F(__fixture, __testsuite, __testname) \
	static void __testname##_body(struct sk_buff*,struct __fixture*,int,u32); \
	static void __testname(struct sk_buff * skb, struct test_dev* tdev, int _i, u32 _value); \
	struct __test_desc __testname##_setup = \
        { .tclass = "" # __testsuite "", .name = "" # __testname "", .fun = __testname };\
	\
	static void __testname(struct sk_buff * skb, struct test_dev* tdev, \
		int _i, u32 _value)				\
	{\
		struct __fixture ctx = __fixture##_template;\
		ctx.setup(skb,tdev,&ctx);\
		if (!ctx.ok) return;\
		__testname##_body(skb,&ctx,_i,_value);	\
		ctx.teardown(skb,&ctx);\
	}\
	static void __testname##_body(struct sk_buff * skb,\
			struct __fixture* ctx,\
			int _i, u32 _value)

/* Fail the test case unless expr is true */
/* The space before the comma sign before ## is essential to be compatible
   with gcc 2.95.3 and earlier.
*/
#define fail_unless_msg(expr, format, ...)			\
        _fail_unless(skb, expr, __FILE__, __LINE__,		\
        format , ## __VA_ARGS__, NULL)

#define fail_unless(expr, ...)\
        _fail_unless(skb, expr, __FILE__, __LINE__,		\
        "Failure '"#expr"' occurred " , ## __VA_ARGS__, NULL)

/* Fail the test case if expr is true */
/* The space before the comma sign before ## is essential to be compatible
   with gcc 2.95.3 and earlier.
*/

/* FIXME: these macros may conflict with C89 if expr is
   FIXME:   strcmp (str1, str2) due to excessive string length. */
#define fail_if(expr, ...)\
        _fail_unless(skb, !(expr), __FILE__, __LINE__,		\
        "Failure '"#expr"' occurred " , ## __VA_ARGS__, NULL)

/* Always fail */
#define fail(...) _fail_unless(skb, 0, __FILE__, __LINE__, "Failed" , ## __VA_ARGS__, NULL)

/* Non macro version of #fail_unless, with more complicated interface
 * returns nonzero if ok, 0 otherwise
 */
long _fail_unless (struct sk_buff *skb, int result, const char *file,
		int line, const char *expr, ...);

/* New check fail API. */
#define ck_abort() ck_abort_msg(NULL)
#define ck_abort_msg fail
#define ck_assert(C) fail_unless(C)
#define ck_assert_msg fail_unless_msg

/* Integer comparsion macros with improved output compared to fail_unless(). */
/* O may be any comparion operator. */
#define _ck_assert_int_goto(X, O, Y, _lbl)		\
	do { int x = (X); int y = (Y);\
		if (!ck_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%x, "#Y"==0x%x", x, y)) \
			goto _lbl;\
	} while (0)

#define _ck_assert_int(X, O, Y) \
	do { int x = (X); int y = (Y);\
		ck_assert_msg(x O y,\
		  "Assertion '"#X#O#Y"' failed: "#X"==0x%x, "#Y"==0x%x", x, y);\
	} while (0)

#define _ck_assert_int_ret(X, O, Y)\
	do { int x = (X); int y = (Y);\
		if (!ck_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 return;\
	} while (0)

#define _ck_assert_long_goto(X, O, Y, _lbl)		\
	do { long x = (X); long y = (Y);\
		if (!ck_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 goto _lbl;\
	} while (0)

#define _ck_assert_long_ret(X, O, Y)\
	do { long x = (X); long y = (Y);\
		if (!ck_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 return;\
	} while (0)

/* O may be any comparion operator. */
#define _ck_assert_long(X, O, Y) \
	do { long x = (X); long y = (Y);\
		ck_assert_msg(x O y,\
		  "Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y);\
	} while (0)

/* String comparsion macros with improved output compared to fail_unless() */
#define _ck_assert_str_eq(X, O, Y)			\
	do { const char* x = (X); const char* y = (Y);\
		ck_assert_msg(strcmp(x,y) == 0,\
		  "Assertion '"#X#O#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"",\
		  x, y);\
	} while (0)

#endif /* KCHECK_H */