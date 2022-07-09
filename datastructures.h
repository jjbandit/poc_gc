// NOTE(Jesse): Always use the move constructor.
// This is how _appearing_ to return by value works : `return Str(buffer, length)`
#define RESTRICT_TO_MOVE_OPERATOR(T_NAME__) \
    T_NAME__(const T_NAME__ & obj) = delete; \
    T_NAME__& operator=(T_NAME__ & other) = delete; \
    T_NAME__& operator=(T_NAME__ && other) = delete; \
    T_NAME__(T_NAME__ && source) = default

template <typename T>
class buf_ref
{
  public:
  RESTRICT_TO_MOVE_OPERATOR(buf_ref);

  T* buffer;
  u8 ref_number;

  buf_ref(buf_handle<T> & handle);

  buf_ref(T *buf_init):
    buffer(buf_init),
    ref_number(register_reference<u8>(buffer, &buffer))
  {
    printf("buf_ref_construct 0x%lx\n", (umm)buf_init);
  }

  ~buf_ref()
  {
    printf("buf_ref_destruct  0x%lx\n", (umm)buffer);

    allocation_tag *Tag = GetTag(buffer);
    assert(Tag->references[ref_number] == &buffer);
    Tag->references[ref_number] = 0;
  }
};

template <typename T>
struct buf_handle
{
  T *buffer;

  buf_handle& operator=(buf_handle & other) = delete;
  buf_handle& operator=(buf_handle && other) = delete;
  buf_handle(buf_handle & obj) = delete;

  buf_handle(buf_handle && handle):
    buffer(handle.buffer)
  {
    printf("buf_handle move construct 0x%lx\n", (umm)buffer);
    take_ownership((u8**)&buffer);
  }

  buf_handle(T *buf_init):
    buffer(buf_init)
  {
    printf("buf_handle construct 0x%lx\n", (umm)buffer);
    take_ownership((u8**)&buffer);
  }

  ~buf_handle()
  {
    printf("buf_handle_destruct  0x%lx\n", (umm)buffer);
    if (buffer && we_own_allocation((u8**)&buffer))
    {
      Deallocate((u8*)buffer);
    }
    else
    {
      // NOTE(Jesse): We transferred ownership to another container.
    }
  }

  bool operator=(int new_buf)
  {
    assert(new_buf == 0);
    Deallocate((u8*)buffer);
    return false;
  }

  T & operator[](int index)
  {
    return this->buffer[index];
  }
};

template <typename T>
buf_ref<T>::buf_ref(buf_handle<T> & handle) :
  buffer(handle.buffer),
  ref_number(register_reference<u8>(buffer, &buffer))
{
  printf("buf_ref_construct 0x%lx\n", (umm)buffer);
}

buf_handle<u8> slice_buffer(buf_ref<u8> src, umm begin, umm end);
struct Str_Ref;

struct Str
{
  RESTRICT_TO_MOVE_OPERATOR(Str);

  umm len;
  buf_handle<u8> handle;

  Str(int len_init, buf_handle<u8> buffer):
    len(len_init),
    handle(std::move(buffer))
  {
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)handle.buffer);
  }

  // NOTE(Jesse): If we were more clever about .. something .. we wouldn't have
  // to copy const strings onto the heap.  I'm not sure what the best way of
  // handling this is, but surely there is one.
  Str(int len_init, const char* buffer):
    len(len_init),
    handle(Allocate<u8>(len+1, allocation_type::Buffer))
  {
    CopyMemory(handle.buffer, (u8*)buffer, len_init);
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)handle.buffer);
  }

  Str(int len_init, u8* buffer):
    len(len_init),
    handle(buffer)
  {
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)handle.buffer);
  }

  Str(int len_init):
    len(len_init),
    handle(Allocate<u8>(len+1, allocation_type::Buffer))
  {
    printf("Initialized Str(%lu)\n", len);
  }

  u8* buffer()
  {
    return handle.buffer;
  }

  ~Str()
  {
    printf("Destroyed   Str(%lu)\n", len);
  }

  Str slice(umm begin, umm end)
  {
    assert(begin == 0);
    assert(end < len);
    return Str(len, ::slice_buffer(this->handle, begin, end));
  }

  Str replace(const Str_Ref &, const Str_Ref &);
};

buf_handle<u8> slice_buffer(buf_ref<u8> src, umm begin, umm end)
{
  printf("slice start\n");
  allocation_tag *Tag  = GetTag(src.buffer);

  umm size = end-begin;
  assert(begin == 0);
  assert(size <= Tag->size);

  auto handle = Allocate<u8>(size+1, allocation_type::Buffer);
  CopyMemory(handle.buffer, src.buffer, size);

  printf("slice end\n");

  return handle;
}

struct Str_Ref
{
  RESTRICT_TO_MOVE_OPERATOR(Str_Ref);

  buf_ref<u8> handle;
  umm len;

  Str_Ref(Str &s):
    handle(s.handle)
  {
  }

  Str_Ref(u8* buffer):
    handle(buffer)
  {
  }

  Str_Ref(buf_handle<u8> & buf_init):
    handle(buf_ref<u8>(buf_init))
  {
  }

  Str_Ref(buf_ref<u8> & buf_init):
    handle(std::move(buf_init))
  {
  }

  ~Str_Ref()
  {
    printf("Leaking!\n");
    /* NotImplemented(); */
  }

  u8& operator[](int i)
  {
    return handle.buffer[i];
  }

  u8* buffer()
  {
    return handle.buffer;
  }

  /* Str slice(int start, int end) */
  /* { */
  /*   assert_PointerValidForHeap(&gHeap, (u8*)this->buffer); */
  /*   assert_AllocationValidForHeap(&gHeap, this->buffer->handle); */
  /*   printf("ref-slice 0x%lx\n", (umm)this->buffer->handle); */
  /*   return this->buffer->slice(start, end); */
  /* } */

};


Str replace(const Str_Ref &src, const Str_Ref &search_pattern, const Str_Ref &replace_pattern)
{
  printf("slice start\n");

  allocation_tag *Tag  = GetTag(src.handle.buffer);
  umm size = Tag->size;

  assert(search_pattern.len < size);

  auto handle = Allocate<u8>(size, allocation_type::Buffer);
  CopyMemory(handle.buffer, search_pattern.handle.buffer, size);

  printf("slice end\n");

  return Str(size, std::move(handle));
}

Str Str::replace(const Str_Ref &search_pattern, const Str_Ref &replace_pattern)
{
  return ::replace(Str_Ref(handle), search_pattern, replace_pattern);
}












template<typename T>
struct List
{
  RESTRICT_TO_MOVE_OPERATOR(List);

  umm at;
  umm len;
  buf_handle<T> handle;

  List(int elements):
    at(0),
    len(elements),
    handle(Allocate<T>(elements, allocation_type::List_Str))
  {
    printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)&handle.buffer);
  }

  /* List<T>(int len_init, u8* buf_init) { */
  /*   len = len_init; */
  /*   handle = buf_init; */
  /*   printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)handle); */
  /* } */

  ~List() {
    for (int i = 0; i < len; ++i)
    {
      if (handle[i].buffer())
      {
        assert(we_own_allocation(&handle.buffer[i].handle.buffer));
        Deallocate<u8>(&handle.buffer[i].handle);
      }
    }
    printf("Deallocating List(%ld) @ 0x%lx\n", len, (umm)&handle);
    Deallocate<Str>(&handle);
  }

  Str_Ref push(T *buffer)
  {
    assert(at < len);
    printf("Pushing list buffer (%lu) :: Owned by 0x%lx\n", at, (umm)&handle);

    T *bucket = handle.buffer + at;

    MoveMemory((u8*)bucket, (u8*)buffer, sizeof(T));

    take_ownership((u8**)&bucket->handle);

    allocation_tag* Tag = GetTag(bucket->handle.buffer);
    Tag->Type = allocation_type::Owned_Buffer;

    at++;

    VerifyHeapIntegrity(&gHeap);

    return Str_Ref(bucket->handle);
  }

#if 0
  // NOTE(Jesse): This is here such that we can add elements that are created
  // from temporaries.  This is unsafe, but works because the buffer is
  // immediately copied into the permanent storage of this container.  Somewhat
  // confusing, definite footgun.  Not sure what we can do about getting around
  // this issue.
  //
  // I guess what I'm trying to say here is only ever write functions that
  // accept const references (temporary variables) for stuff that's stored
  // immediately afterwards.
  //
  Str_Ref push(const T &buffer)
  {
    return push((T*)&buffer);
  }
#endif
};
