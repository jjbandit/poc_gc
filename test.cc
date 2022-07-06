#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <utility>

#include <unistd.h>

#define NotImplemented() assert(0)
#define InvalidCodePath() assert(0)

#define Kilobytes(n) ((n)<<10)
#define Megabytes(n) ((n)<<20)
#define Gigabytes(n) ((n)<<30)

typedef unsigned long int umm;
typedef unsigned char u8;



#if CALLOC_ALLOC

#define debug_mode_owner_pointer() void* owner;
#define set_location_pointer(buf, owner_in) this->owner = owner_in;
#define get_pointer_location(buf) this->owner

#define Allocate(size, ...) Allocate_Internal(size)

void Deallocate(u8* p)
{
  printf("Deallocating 0x%lx\n", (umm)p);
  free(p);
}

u8* Allocate_Internal(umm size)
{
  printf("Allocating %lu @ ", size);
  u8* Result = (u8*)calloc(size, 1);
  printf("0x%lx\n", (umm)Result);
  return Result;
}

void InitHeap(umm Megabytes) {}

#elif GC_ALLOC

#define debug_mode_owner_pointer()

struct heap
{
  umm allocations;

  umm at;
  umm size;
  u8* memory;
};

heap gHeap;

void InitHeap(umm bytes) {
  gHeap.memory = (u8*)calloc(bytes, 1);
  gHeap.size = bytes;
  gHeap.allocations = 0;
}

enum allocation_type
{
  None,

  Buffer,
  List_Str,

  Owned_Buffer,
};

struct allocation_tag
{
  allocation_type Type;
  u8** pointer_location;
  umm size;

  umm MAGIC_NUMBER;
};

heap AllocateHeap(umm bytes)
{
  heap Result = {
    .memory = (u8*)calloc(bytes, 1),
    .size = bytes,
    .at = 0
  };
  return Result;
}

inline allocation_tag*
GetTag(u8* buffer)
{
  assert(buffer);
  allocation_tag *result = (allocation_tag*)( buffer - sizeof(allocation_tag) );
  assert(result->MAGIC_NUMBER == 0xDEADBEEF);
  return result;
}

inline u8*
GetBuffer(allocation_tag *Tag)
{
  u8* Result = (u8*)Tag + sizeof(allocation_tag);
  return Result;
}

u8* CopyTo(heap *Dest, u8* Buffer, umm Size)
{
  u8* result = (u8*)(Dest->memory + Dest->at);
  memcpy(result, Buffer, Size);
  Dest->at += Size;
  return result;
}

allocation_tag*
CopyTo(heap *Dest, allocation_tag *Tag)
{
  allocation_tag* Result = (allocation_tag *)CopyTo(Dest, (u8*)Tag, sizeof(allocation_tag));

  u8* Allocation = GetBuffer(Tag);
  CopyTo(Dest, Allocation, Tag->size);

  ++Dest->allocations;

  return Result;
}

void assert_HeapEmpty(heap *H)
{
  assert(H->at == 0);
  assert(H->allocations == 0);
}

void assert_PointerValidForHeap(heap *H, u8* pointer_location)
{
  assert(pointer_location >= H->memory);
  assert(pointer_location <= H->memory+H->size);
}

void assert_HeapEnclosesBuffer(heap *H, u8* buffer, umm size)
{
  assert_PointerValidForHeap(H, buffer);
  assert_PointerValidForHeap(H, buffer+size);
}

void assert_TagValidForHeap(heap *H, allocation_tag *T)
{
  assert_HeapEnclosesBuffer(H, (u8*)T, sizeof(allocation_tag));

  u8* buffer = GetBuffer(T);
  assert(T->pointer_location[0] == buffer);
  assert_HeapEnclosesBuffer(H, buffer, T->size);
}

void
VerifyHeapIntegrity(heap *H)
{
  u8* current_memory = H->memory;
  umm at = 0;
  while (at < H->at)
  {
    allocation_tag *Tag = (allocation_tag*)(current_memory + at);
    assert_TagValidForHeap(H, Tag);
    assert(Tag->size);
    u8* current_buffer = GetBuffer(Tag);
    assert(*Tag->pointer_location == current_buffer);

    at += Tag->size + sizeof(allocation_tag);
  }
}

void collect();

u8* Allocate(umm bytes, u8* owner, allocation_type Type)
{
  collect();

  umm total_allocation_size = (sizeof(allocation_tag) + bytes);
  assert(gHeap.at + total_allocation_size < gHeap.size);

  allocation_tag *Tag = (allocation_tag*)(gHeap.memory + gHeap.at);
  Tag->Type = Type;
  Tag->size = bytes;
  Tag->MAGIC_NUMBER = 0xDEADBEEF;

  gHeap.at += total_allocation_size;
  ++gHeap.allocations;
  return GetBuffer(Tag);
}

void Deallocate(u8* Allocation)
{
  printf("Deallocating 0x%lx\n", (umm)Allocation);
  allocation_tag *Tag = GetTag(Allocation);
  Tag->pointer_location = 0;
}

void
set_location_pointer(u8** pointer_location)
{
  allocation_tag *T = GetTag(*pointer_location);
  T->pointer_location = pointer_location;

  VerifyHeapIntegrity(&gHeap);
}

u8**
get_pointer_location(u8* buffer)
{
  u8 **Result = 0;
  if ( buffer ) { Result = GetTag(buffer)->pointer_location; }
  return Result;
}

bool we_own_allocation(u8 ** buffer)
{
  assert(buffer);
  assert(*buffer);
  bool result = (get_pointer_location(*buffer) == buffer);
  return result;
}


#else
#error "Unspecified allocation strategy.  Exiting."
#endif


// NOTE(Jesse): Always use the move constructor.
// This is how apparently returning by value works : `return Str(buffer, length)`

#define RESTRICT_TO_MOVE_OPERATOR(T_NAME__) \
    T_NAME__(const T_NAME__ & obj) = delete; \
    T_NAME__& operator=(T_NAME__ & other) = delete; \
    T_NAME__& operator=(T_NAME__ && other) = delete; \
    T_NAME__(T_NAME__ && source) = default;


struct Str {

  RESTRICT_TO_MOVE_OPERATOR(Str);

  Str(int len_init, u8* buf_init) {
    len = len_init;
    buf = buf_init;
    set_location_pointer(&buf);
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)buf_init);
  }

  Str(int len_init) {
    len = len_init;
    buf = Allocate(len+1, (u8*)&buf, allocation_type::Buffer);
    set_location_pointer(&buf);
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)buf);
  }

  ~Str() {
    if (buf && we_own_allocation(&buf))
    {
      Deallocate(buf);
    }
  }

  Str slice(int begin, int end)
  {
    umm size = end-begin;
    u8* buffer = Allocate(size+1, (u8*)&buf, allocation_type::Buffer);
    return Str(size, buffer);
  }

  unsigned long int len;
  u8* buf;

  debug_mode_owner_pointer();
};


void
MoveMemory(u8* Dest, u8* Src, umm size)
{
  memcpy(Dest, Src, size);
  memset(Src, 0, size);
}


#if 1
template<typename T>
struct List {

  RESTRICT_TO_MOVE_OPERATOR(List);

  List(int len_init) {
    at = 0;
    len = len_init;

    umm buf_len = len*sizeof(T);
    buf = (T*)Allocate(buf_len, (u8*)&buf, allocation_type::List_Str);
    set_location_pointer((u8**)&buf);

    printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)&buf);
  }

  /* List<T>(int len_init, u8* buf_init) { */
  /*   len = len_init; */
  /*   buf = buf_init; */
  /*   printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)buf); */
  /* } */

  ~List() {
    for (int i = 0; i < len; ++i)
    {
      if (buf[i].buf)
      {
        assert(we_own_allocation(&buf[i].buf));
        Deallocate(buf[i].buf);
      }
    }

    printf("Deallocating List(%ld) @ 0x%lx\n", len, (umm)&buf);
    Deallocate((u8*)buf);
  }

  void push(T *element)
  {
    assert(at < len);
    printf("Pushing list element (%lu) :: Owned by 0x%lx\n", at, (umm)&buf);

    T *bucket = buf+at;

    // NOTE(Jesse): Tried to use std::move here, but it's actually not
    // specified to do anything.  It's a hint to the compiler; it _might_ do
    // something, but it doesn't have to.
    MoveMemory((u8*)bucket, (u8*)element, sizeof(T));

    set_location_pointer(&bucket->buf);

    allocation_tag* Tag = GetTag(bucket->buf);
    Tag->Type = allocation_type::Owned_Buffer;

    at++;

    VerifyHeapIntegrity(&gHeap);
  }

  // NOTE(Jesse): This is here such that we can add elements that are created
  // from temporaries.  This is unsafe, but works because the element is
  // immediately copied into the permanent storage of this container.  Somewhat
  // confusing, definite footgun.  Not sure what we can do about getting around
  // this issue.
  //
  // I guess what I'm trying to say here is only ever write functions that
  // accept const references (temporary variables) for stuff that's stored
  // immediately afterwards.
  //
  void push(const T &element)
  {
    push((T*)&element);
  }

  umm at;
  umm len;
  T *buf;

  debug_mode_owner_pointer();
};
#endif

// NOTE(Jesse): new_pointer_location is a pointer to a new container buffer.
// When containers get copied they need to update their children to point to
// the new memory location of the container.  This param is what does that.
allocation_tag *CopyMemoryToNewHeap(heap *Heap, allocation_tag *Tag, u8** new_pointer_location = 0)
{
  u8* current_buffer = GetBuffer(Tag);
  allocation_tag *NewTag = CopyTo(Heap, Tag);

  u8* new_location = GetBuffer(NewTag);
  assert(*NewTag->pointer_location == current_buffer);

  if (new_pointer_location)
  {
    NewTag->pointer_location = new_pointer_location;
  }

  *NewTag->pointer_location = new_location;

  printf("Persisted (%lu) bytes @ 0x%lx -> 0x%lx\n", NewTag->size, (umm)current_buffer, (umm)new_location);

  return NewTag;
}

void collect()
{
  heap NewZone = AllocateHeap(Megabytes(32));
  VerifyHeapIntegrity(&NewZone);

  {
    u8* current_memory = gHeap.memory;
    umm at = 0;
    while (at < gHeap.at)
    {
      allocation_tag *Tag = (allocation_tag*)(current_memory + at);
      assert(Tag->size);

      if (Tag->pointer_location)
      {
        switch(Tag->Type)
        {
          case allocation_type::Buffer:
          {
            CopyMemoryToNewHeap(&NewZone, Tag);
          } break;

          case allocation_type::Owned_Buffer:
          {
            // It's owned by a container on the heap and will get copied by
            // that container, if it's alive.
          } break;

          case allocation_type::List_Str:
          {
            allocation_tag* NewContainerTag = CopyMemoryToNewHeap(&NewZone, Tag);
            Str* ContainerBuffer = (Str*)GetBuffer(NewContainerTag);
            assert_TagValidForHeap(&NewZone, NewContainerTag);
            VerifyHeapIntegrity(&NewZone);

            umm element_count = Tag->size/sizeof(Str);
            for (umm element_index = 0; element_index < element_count; ++element_index)
            {
              Str* ElementBuffer = ContainerBuffer + element_index;
              if (ElementBuffer->buf)
              {
                allocation_tag *ElementTag = GetTag(ElementBuffer->buf);
                assert(ElementTag->Type == allocation_type::Owned_Buffer);
                assert_PointerValidForHeap(&gHeap, (u8*)ElementTag->pointer_location);
                assert_PointerValidForHeap(&gHeap, (u8*)&ElementTag->pointer_location);

                allocation_tag * NewElementTag = CopyMemoryToNewHeap(&NewZone, ElementTag, &ElementBuffer->buf);
                assert_PointerValidForHeap(&NewZone, (u8*)NewElementTag->pointer_location);
                assert_PointerValidForHeap(&NewZone, (u8*)&NewElementTag->pointer_location);
                assert_TagValidForHeap(&NewZone, NewElementTag);
                VerifyHeapIntegrity(&NewZone);
              }
            }

          } break;

          case None: { InvalidCodePath(); } break;
        }

        VerifyHeapIntegrity(&NewZone);
      }
      else
      {
        printf("Collected (%lu) bytes @ 0x%lx\n", Tag->size, (umm)GetBuffer(Tag));
      }

      at += Tag->size + sizeof(allocation_tag);
    }
  }

  VerifyHeapIntegrity(&NewZone);

  free(gHeap.memory);
  gHeap = NewZone;

  VerifyHeapIntegrity(&NewZone);
  VerifyHeapIntegrity(&gHeap);
}


void ExampleFunction(const Str &input)
{
  printf("---------- function start\n");
  Str thing(32);
  collect();
  printf("---------- collection complete\n");
  return;
}

int main()
{
  InitHeap(Megabytes(32));

#if 1
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

#if 1
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


#if 1
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

}
