#include <stdio.h>
#include <stdlib.h>
#include "lib/ktf_run.h"
#include "lib/debug.h"

/* This program is a generic
 * user level application to run kernel tests
 * provided by modules subscribing to ktf services:
 */

int main (int argc, char** argv)
{
  testing::GTEST_FLAG(output) = "xml:ktest.xml";
  testing::InitGoogleTest(&argc,argv);

  return RUN_ALL_TESTS();
}
