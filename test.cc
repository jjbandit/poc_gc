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

#define Sz_For_Memcopy(const_str)  (u8 *)const_str, sizeof(const_str)


void PrintString(const Str_Ref & s)
{
  printf("%s\n", s.handle.buffer);
}

void PrintString(Str & s)
{
  printf("%s\n", s.handle.buffer);
}

void PrintString(u8 * buffer)
{
  printf("%s\n", buffer);
}


int main()
{
  InitHeap(Megabytes(32));

  {
    // Allocate 32 bytes, and register buf.buffer (the pointer member in buf_handle) w/ the GC
    //
    // This registration happens in the buf_handle constructor so the GC knows that we have
    // a pointer on the stack into heap memory.
    //
    // In this example, the function is take_ownership, but in the Oil code it would be gHeap.PushRoot()
    //
    buf_handle<u8> buf = Allocate<u8>(32, allocation_type::Buffer);

    printf(" -- buf allocated\n");

    // Copy a constant string into our allocated buffer.  We pass the raw
    // pointer because CopyMemory() does not allocate.
    CopyMemory(buf.buffer, Sz_For_Memcopy("value"));

    printf(" -- value copied\n");

    // Call a function that allocates.  Notice it takes a handle.  The handle
    // constructor will get called in slice(), which in turn registers the
    // arguments address with the GC.  This means that slice can allocate
    // without fear of forgetting to register stack roots, because the handle
    // constructor takes care of it automatically.
    buf_handle<u8> buf_sliced = slice_buffer(buf, 0, 2);

    printf(" -- value sliced\n");

    // Buffers are still valid.  During the allocation (collection) the handles
    // had taken care of telling the GC about all the pointers on the stack,
    // which it updated for us.
    //
    // PrintString doesn't allocate, so we can pass raw pointers
    PrintString(buf.buffer);
    PrintString(buf_sliced.buffer);

    printf(" -- values printed\n");
  }

  // Scope exited, handles destructors run and unregister those allocation w/ the collector

  collect(); // Collector reclaims those buffers


#if 0

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
