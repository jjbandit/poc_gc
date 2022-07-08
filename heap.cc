
template <typename T>
buf_handle<T> Allocate(umm elements, allocation_type Type)
{
  collect();

  umm element_buffer_size = (sizeof(T)*elements);
  umm total_allocation_size = (sizeof(allocation_tag) + element_buffer_size);
  assert(gHeap.at + total_allocation_size < gHeap.size);

  allocation_tag *Tag = (allocation_tag*)(gHeap.memory + gHeap.at);
  Tag->Type = Type;
  Tag->size = element_buffer_size;
  Tag->MAGIC_NUMBER = 0xDEADBEEF;

  assert(Tag->ref_count == 0);

  gHeap.at += total_allocation_size;
  ++gHeap.allocations;
  return buf_handle<T>(reinterpret_cast<T*>(GetBuffer(Tag)));
}


void collect()
{
  printf(" ----- collecting --------\n");
  heap NewZone = AllocateHeap(Megabytes(32));
  VerifyHeapIntegrity(&NewZone);

  u8* current_memory = gHeap.memory;
  umm at = 0;
  while (at < gHeap.at)
  {
    allocation_tag *Tag = (allocation_tag*)(current_memory + at);
    assert(Tag->size);

    if (Tag->pointer_location && Tag->pointer_location[0] != 0)
    {
      switch(Tag->Type)
      {
        case allocation_type::Buffer:
        {
          allocation_tag *NewTag = CopyTagAndBuffer(&NewZone, Tag);
          u8* NewBuffer = GetBuffer(NewTag);

          for (umm ref_index = 0; ref_index < NewTag->ref_count; ++ref_index)
          {
            if (u8** Ref = NewTag->references[ref_index])
            {
              *Ref = NewBuffer;
            }
          }

        } break;

        case allocation_type::Owned_Buffer:
        {
          // It's owned by a container on the heap and will get copied by
          // that container, if it's alive.
        } break;

        case allocation_type::List_Str:
        {
          allocation_tag* NewContainerTag = CopyTagAndBuffer(&NewZone, Tag);
          Str* ContainerBuffer = (Str*)GetBuffer(NewContainerTag);
          assert_TagValidForHeap(&NewZone, NewContainerTag);
          VerifyHeapIntegrity(&NewZone);

          umm element_count = Tag->size/sizeof(Str);
          for (umm element_index = 0; element_index < element_count; ++element_index)
          {
            Str* NewElement = ContainerBuffer + element_index;
            if (NewElement->handle.buffer)
            {
              allocation_tag *ElementTag = GetTag(NewElement->handle.buffer);
              assert(ElementTag->Type == allocation_type::Owned_Buffer);
              assert_PointerValidForHeap(&gHeap, (u8*)ElementTag->pointer_location);
              assert_PointerValidForHeap(&gHeap, (u8*)&ElementTag->pointer_location);

              allocation_tag * NewElementTag = CopyTagAndBuffer(&NewZone, ElementTag, &NewElement->handle.buffer);
              assert_PointerValidForHeap(&NewZone, (u8*)NewElementTag->pointer_location);
              assert_PointerValidForHeap(&NewZone, (u8*)&NewElementTag->pointer_location);
              assert_TagValidForHeap(&NewZone, NewElementTag);
              VerifyHeapIntegrity(&NewZone);

              for (umm ref_index = 0; ref_index < NewElementTag->ref_count; ++ref_index)
              {
                NewElementTag->references[ref_index][0] = NewElement->handle.buffer;
              }

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

  VerifyHeapIntegrity(&NewZone);

  free(gHeap.memory);
  gHeap = NewZone;

  VerifyHeapIntegrity(&NewZone);
  VerifyHeapIntegrity(&gHeap);
}
