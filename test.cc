#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <utility>

#include <unistd.h>

#include "types.h"
#include "heap.h"
#include "datastructures.h"

#include "heap.cc"





void ExampleFunction(const Str &input)
{
  printf("---------- function start\n");
  Str thing(32);
  collect();
  printf("---------- collection complete\n");
  return;
}


#define TEST_ALL 0
#define TEST_a (TEST_ALL || 1)
#define TEST_b (TEST_ALL || 1)
#define TEST_c (TEST_ALL || 1)
#define TEST_d (TEST_ALL || 0)

int main()
{
  InitHeap(Megabytes(32));

#if TEST_a
  printf("__ STARTING __ TEST 1 __\n");
  {
    Str thing1(32);
    printf("---------- allocated\n");
    Str s2 = thing1.slice(0,1);
    ExampleFunction(thing1);
    printf("---------- function complete\n");
    collect();

    ExampleFunction(thing1.slice(0,1));
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#if TEST_b
  printf("__ STARTING __ TEST 2 __\n");
  {
    Str thing1(32);
    collect();

    List<Str> list(8);
    collect();

    // Correctly asserts at runtime.  Error message could be better.
#if 0
    list.push(&thing1);
    list.push(&thing1);
#endif

    int i = 0;
    while (i++ < 3)
    {
      Str thing2(32);
      collect();
      list.push(&thing2);
    }
    collect();
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif


#if TEST_c
  printf("__ STARTING __ TEST 3 __\n");
  {
    Str thing(32);
    collect();
    List<Str> list(8);
    collect();

    int i = 0;
    while (i++ < 7)
    {
      printf("slice --------------------- slice\n");
      Str thing2 = thing.slice(0, 2);
      list.push(thing2);
      collect();
    }
    collect();
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#if TEST_d
  printf("__ STARTING __ TEST 4 __\n");
  {
    Str thing1(32);
    List<Str> list(8);

    Str_Ref reference = list.push(thing1);

  collect();
    { Str slice = reference.slice(0,1); }
  collect();
    /* { Str slice = reference.slice(0,1); } */
  collect();

  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif
}
