// NOTE(Jesse): Always use the move constructor.
// This is how apparently returning by value works : `return Str(buffer, length)`

#define RESTRICT_TO_MOVE_OPERATOR(T_NAME__) \
    T_NAME__(const T_NAME__ & obj) = delete; \
    T_NAME__& operator=(T_NAME__ & other) = delete; \
    T_NAME__& operator=(T_NAME__ && other) = delete; \
    T_NAME__(T_NAME__ && source) = default


struct Str {

  RESTRICT_TO_MOVE_OPERATOR(Str);

  Str(int len_init, u8* buf_init) {
    this->len = len_init;
    this->buf = buf_init;
    take_ownership(&this->buf);
    printf("Initialized Str(%lu)    @ 0x%lx\n", this->len, (umm)buf_init);
  }

  Str(int len_init) {
    this->len = len_init;
    this->buf = Allocate(this->len+1, &this->buf, allocation_type::Buffer);
    printf("Initialized Str(%lu)    @ 0x%lx\n", this->len, (umm)this->buf);
  }

  ~Str() {
    if (this->buf && we_own_allocation(&this->buf))
    {
      Deallocate(this->buf);
    }
  }


  Str slice(umm begin, umm end)
  {
    allocation_tag *Tag  = GetTag(buf);

    umm size = end-begin;
    assert(begin == 0);
    assert(size <= Tag->size);

    u8* buffer = Allocate(size+1, ALLOCATION_OWNER_NONE, allocation_type::Buffer);
    take_ownership((u8**)&buffer);

    CopyMemory(buffer, buf, size);
    return Str(size, buffer);
  }


  umm len;
  u8* buf;
};

struct Str_Ref
{
  RESTRICT_TO_MOVE_OPERATOR(Str_Ref);

  Str_Ref(Str* string)
  {
    this->element = string;
    this->ref_number = register_str_reference(string->buf, &this->element);
  }

  ~Str_Ref()
  {
    printf("Leaking!\n");
    /* NotImplemented(); */
  }

  Str* element;
  u8 ref_number;

  Str slice(int start, int end)
  {
    assert_PointerValidForHeap(&gHeap, (u8*)this->element);
    assert_AllocationValidForHeap(&gHeap, this->element->buf);

    printf("ref-slice 0x%lx\n", (umm)this->element->buf);
    return this->element->slice(start, end);
  }
};

template<typename T>
struct List {

  RESTRICT_TO_MOVE_OPERATOR(List);

  List(int len_init) {
    at = 0;
    len = len_init;

    umm buf_len = len*sizeof(T);
    buf = (T*)Allocate(buf_len, (u8**)&buf, allocation_type::List_Str);
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

  umm at;
  umm len;
  T *buf;
};

