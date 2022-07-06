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
