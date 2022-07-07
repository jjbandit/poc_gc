// NOTE(Jesse): Always use the move constructor.
// This is how apparently returning by value works : `return Str(buffer, length)`

#define RESTRICT_TO_MOVE_OPERATOR(T_NAME__) \
    T_NAME__(const T_NAME__ & obj) = delete; \
    T_NAME__& operator=(T_NAME__ & other) = delete; \
    T_NAME__& operator=(T_NAME__ && other) = delete; \
    T_NAME__(T_NAME__ && source) = default

template <typename T>
struct buf_ref
{
  RESTRICT_TO_MOVE_OPERATOR(buf_ref);

  buf_ref(T *element_init):
    element(element_init),
    ref_number(register_reference<u8>(element, &element))
  {
    printf("buf_ref_construct 0x%lx\n", (umm)element_init);
  }

  ~buf_ref()
  {
    printf("buf_ref_destruct  0x%lx\n", (umm)element);

    // NOTE(Jesse): Once that's done we also need to put logic in to search for an open slot
    // when allocating, instead of just incrementing and overwriting.
    printf("TODO(Jesse): This needs to pull the reference off the allocation_tag list\n");
  }

  T* element;
  u8 ref_number;
};

template <typename T>
struct buf_handle
{
  buf_handle& operator=(buf_handle & other) = delete;
  buf_handle& operator=(buf_handle && other) = delete;
  buf_handle(buf_handle & obj) = delete;

  buf_handle(buf_handle && source):
    element(source.element)
  {
    printf("buf_handle move construct 0x%lx\n", (umm)source.element);
    take_ownership((u8**)&element);
  }

  buf_handle(T *element_init):
    element(element_init)
  {
    printf("buf_handle construct 0x%lx\n", (umm)element_init);
    take_ownership((u8**)&element);
  }

  ~buf_handle()
  {
    printf("buf_handle_destruct  0x%lx\n", (umm)element);
    if (element && we_own_allocation((u8**)&element))
    {
      Deallocate((u8*)element);
    }
    else
    {
      // NOTE(Jesse): We transferred ownership to another container.
    }
  }

  T *element;
};


struct Str
{
  RESTRICT_TO_MOVE_OPERATOR(Str);

  Str(int len_init, buf_handle<u8> buffer):
    len(len_init),
    buf(std::move(buffer))
  {
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)buf.element);
  }

  Str(int len_init, u8* buffer):
    len(len_init),
    buf(buffer)
  {
    printf("Initialized Str(%lu)    @ 0x%lx\n", len, (umm)buf.element);
  }

  Str(int len_init):
    len(len_init),
    buf(Allocate<u8>(len+1, allocation_type::Buffer))
  {
    printf("Initialized Str(%lu)\n", len);
  }

  ~Str()
  {
    printf("Destroyed   Str(%lu)\n", len);
  }

  /* Str slice(umm begin, umm end) */
  /* { */
  /*   return ::slice(buf, begin, end); */
  /* } */

  umm len;
  buf_handle<u8> buf;
};

Str slice(buf_ref<u8> src, umm begin, umm end)
{
  printf("slice start\n");
  allocation_tag *Tag  = GetTag(src.element);

  umm size = end-begin;
  assert(begin == 0);
  assert(size <= Tag->size);

  auto buf = Allocate<u8>(size+1, allocation_type::Buffer);
  CopyMemory(buf.element, src.element, size);

  printf("slice end\n");

  return Str(size, std::move(buf));
;
}

struct Str_Ref
{
  RESTRICT_TO_MOVE_OPERATOR(Str_Ref);

  Str_Ref(Str* string)
  {
    this->element = string;
    this->ref_number = register_reference<Str>(string->buf.element, &this->element);
  }

  ~Str_Ref()
  {
    printf("Leaking!\n");
    NotImplemented();
  }

  Str* element;
  u8 ref_number;

  /* Str slice(int start, int end) */
  /* { */
  /*   assert_PointerValidForHeap(&gHeap, (u8*)this->element); */
  /*   assert_AllocationValidForHeap(&gHeap, this->element->buf); */
  /*   printf("ref-slice 0x%lx\n", (umm)this->element->buf); */
  /*   return this->element->slice(start, end); */
  /* } */

};

template<typename T>
struct List
{
  RESTRICT_TO_MOVE_OPERATOR(List);

  umm at;
  umm len;
  buf_handle<T> buf;

  List(int elements):
    at(0),
    len(elements),
    buf(Allocate<T>(elements, allocation_type::List_Str))
  {
    printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)&buf.element);
  }

  /* List<T>(int len_init, u8* buf_init) { */
  /*   len = len_init; */
  /*   buf = buf_init; */
  /*   printf("Initialized List(%lu) @ 0x%lx\n", len, (umm)buf); */
  /* } */

  ~List() {
    for (int i = 0; i < len; ++i)
    {
      if (buf.element[i].buf.element)
      {
        assert(we_own_allocation(&buf.element[i].buf.element));
        Deallocate<u8>(buf.element[i].buf.element);
      }
    }
    printf("Deallocating List(%ld) @ 0x%lx\n", len, (umm)&buf);
    Deallocate<Str>(buf.element);
  }

#if 0
  Str_Ref push(T *element)
  {
    assert(at < len);
    printf("Pushing list element (%lu) :: Owned by 0x%lx\n", at, (umm)&buf);

    T *bucket = buf+at;

    MoveMemory((u8*)bucket, (u8*)element, sizeof(T));

    take_ownership(&bucket->buf);

    allocation_tag* Tag = GetTag(bucket->buf);
    Tag->Type = allocation_type::Owned_Buffer;

    at++;

    VerifyHeapIntegrity(&gHeap);

    return Str_Ref(bucket);
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
  Str_Ref push(const T &element)
  {
    return push((T*)&element);
  }
#endif
};
