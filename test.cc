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


#define TEST_ALL 1
#define TEST_1 (TEST_ALL || 0)
#define TEST_2 (TEST_ALL || 0)
#define TEST_3 (TEST_ALL || 0)
#define TEST_4 (TEST_ALL || 0)

int main()
{
  InitHeap(Megabytes(32));

#if TEST_1
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

#if TEST_2
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


#if TEST_3
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

#if TEST_4
  {
    Str thing1(32);
    List<Str> list(8);

    Str thing2(32);

    /* thing1 = thing2; */

    /* Str_Ref* reference = list.push(thing1.slice(0,1)); */
    /* Str slice = reference->slice(0,1); */

  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif
}
