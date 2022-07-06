
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

u8*
CopyTo(heap *Dest, u8* Buffer, umm Size)
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

void
MoveMemory(u8* Dest, u8* Src, umm size)
{
  memcpy(Dest, Src, size);
  memset(Src, 0, size);
}
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

