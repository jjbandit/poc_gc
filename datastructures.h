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

    printf("------------------------------------------- 0x%lx\n", (umm)buf);
    u8* buffer = Allocate(size+1, (u8*)&buf, allocation_type::Buffer);
    printf("------------------------------------------- 0x%lx\n", (umm)buf);
    return Str(size, buffer);
  }

  unsigned long int len;
  u8* buf;
};

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

  Str* push(T *element)
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


    return bucket;
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
  Str* push(const T &element)
  {
    return push((T*)&element);
  }

  umm at;
  umm len;
  T *buf;
};

