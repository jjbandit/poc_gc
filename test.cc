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





Str replace(buf_ref<u8> Source, buf_ref<u8> ReplacementPattern)
{
  printf("---------- function start\n");

  collect();

  Str Result(1, Allocate<u8>(4, allocation_type::Buffer));
  CopyMemory(Result.handle.buffer, Source.buffer, 1);
  printf("---------- collection complete\n");
  return Result;
}


#define TEST_ALL 0
#define TEST_e (TEST_ALL || 1)
#define TEST_f (TEST_ALL || 1)
#define TEST_a (TEST_ALL || 1)
#define TEST_b (TEST_ALL || 1)
#define TEST_c (TEST_ALL || 1)
#define TEST_d (TEST_ALL || 1)

#define Sz(const_str) sizeof(const_str), const_str


void PrintString(const Str_Ref & s)
{
  printf("%s\n", s.handle.buffer);
}

void PrintString(Str & s)
{
  printf("%s\n", s.handle.buffer);
}

int main()
{
  InitHeap(Megabytes(32));

  /* buffer_handle<u8> buf = Allocate<u8>(32); // Allocate 32 bytes */

#if 1
#if TEST_e
  printf("__ STARTING __ TEST e __\n");
  {
    Str thing1(Sz("thing1"));

    /* thing1 = 0; */
    /* Str thing2 = thing1; */

    /* PrintString(thing1); */
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#if TEST_f
  printf("__ STARTING __ TEST f __\n");
  for (int i = 0; i < 5; ++i)
  {
    Str thing1(Sz("thing1"));
    collect();
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#if TEST_a
  printf("__ STARTING __ TEST a __\n");
  {
    Str foobar(Sz("foo bar"));
    collect();

    printf("-----------------\n");

    Str foo = foobar.slice(0, 3);
    collect();

    printf("-----------------\n");

    PrintString(foobar);
    PrintString(foo);

    Str baz = Str(Sz("baz"));
    /* Str bazbar = foobar.replace(Str_Ref(foo), Str_Ref(baz)); */
    /* PrintString(bazbar); */
    collect();

    /* ExampleFunction(thing1.slice(0,1)); */
  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#if TEST_b
  printf("__ STARTING __ TEST b __\n");
  {
    Str thing1(Sz("thing1"));

    List<Str> list(8);

#if 1
    PrintString(thing1);

    auto thing1_ref = list.push(&thing1);
    PrintString(thing1_ref);

    thing1_ref[0] = 'a';

    collect();
    PrintString(thing1_ref);
    collect();
    PrintString(thing1_ref);
#else
    // Correctly asserts at runtime.  Error message could be better.
    list.push(&thing1);
    list.push(&thing1);
#endif

    /* int i = 0; */
    /* while (i++ < 3) */
    /* { */
    /*   Str thing2(Sz("thing2")); */
    /*   collect(); */
    /*   list.push(&thing2); */
    /* } */
    /* collect(); */

  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif


#if TEST_c
  printf("__ STARTING __ TEST 3 __\n");
  {
    Str thing(Sz("01234567789"));
    collect();
    List<Str> list(8);
    collect();

    int i = 0;
    while (i++ < 7)
    {
      printf("slice --------------------- slice\n");
      Str thing2 = thing.slice(0, i);
      collect();
      /* PrintString(thing2); */
      /* collect(); */
      /* list.push(&thing2); */
      /* collect(); */
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

    Str_Ref reference = list.push(&thing1);

  collect();
    /* { Str slice = reference.slice(0,1); } */
  collect();
    /* { Str slice = reference.slice(0,1); } */
  collect();

  }
  collect();
  assert_HeapEmpty(&gHeap);
#endif

#endif

}
