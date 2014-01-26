// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_ALLOCATORS_PAGETABLE_H
#define THERON_DETAIL_ALLOCATORS_PAGETABLE_H


#include <new>

#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>


namespace Theron
{
namespace Detail
{


/**
A paged table of entries, with the pages created on demand.
Currently, once allocated, pages are only freed at end-of-day on destruction.
*/
template <class EntryType, uint32_t ENTRIES_PER_PAGE>
class PageTable
{
public:

    /**
    Default constructor.
    */
    PageTable();

    /**
    Destructor.
    */
    ~PageTable();

    /**
    Allocates the entry with the given index and returns a pointer to the memory.
    The table is grown by allocating more pages as required.
    */
    inline void *AllocateEntry(const uint32_t index);

    /**
    Frees the entry with the given index.
    */
    inline void FreeEntry(const uint32_t index);

    /**
    Returns a pointer to the entry with the given index, which must have been previously allocated.
    */
    inline void *GetEntry(const uint32_t index);

private:

    class Page
    {
    public:

        inline Page() :
          mData(0),
          mNext(0)
        {
            IAllocator *const allocator(AllocatorManager::GetCache());
            void *const data(allocator->AllocateAligned(DataSize(), THERON_ALIGNOF(EntryType)));
            mData = reinterpret_cast<uint8_t *>(data);
        }

        inline ~Page()
        {
            IAllocator *const allocator(AllocatorManager::GetCache());
            allocator->Free(mData, DataSize());
        }

        THERON_FORCEINLINE void *GetEntry(const uint32_t index)
        {
            const uint32_t paddedEntrySize(PaddedEntrySize());
            uint8_t *const entry(mData + index * paddedEntrySize);
            THERON_ASSERT(THERON_ALIGNED(entry, THERON_ALIGNOF(EntryType)));
            return reinterpret_cast<void *>(entry);
        }

        THERON_FORCEINLINE Page *GetNext() const
        {
            return mNext;
        }

        THERON_FORCEINLINE void SetNext(Page *const next)
        {
            mNext = next;
        }

    private:

        inline static uint32_t PaddedEntrySize()
        {
            uint32_t entrySize(sizeof(EntryType));
            return THERON_ROUNDUP(entrySize, THERON_ALIGNOF(EntryType));
        }

        inline static uint32_t DataSize()
        {
            return PaddedEntrySize() * ENTRIES_PER_PAGE;
        }

        uint8_t *mData;
        Page *mNext;
    };

    PageTable(const PageTable &other);
    PageTable &operator=(const PageTable &other);

    inline static Page *AllocatePage();

    Page *mHead;                ///< Pointers to first page in a singly-linked list of pages.
};


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
inline PageTable<EntryType, ENTRIES_PER_PAGE>::PageTable() :
  mHead(0)
{
    // Pre-allocate the first page so head is always non-null.
    mHead = AllocatePage();
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
inline PageTable<EntryType, ENTRIES_PER_PAGE>::~PageTable()
{
    IAllocator *const pageAllocator(AllocatorManager::GetCache());

    // Free all allocated pages. Pages are only freed at end of day.
    // Pages must be explicitly destructed since they were allocated in-place.
    Page *page(mHead);
    while (page)
    {
        Page *const next(page->GetNext());

        page->~Page();
        pageAllocator->Free(page, sizeof(Page));

        page = next;
    }
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
THERON_FORCEINLINE void *PageTable<EntryType, ENTRIES_PER_PAGE>::AllocateEntry(const uint32_t index)
{
    // Find the page containing the entry with the given index, creating any missing pages as we go.
    uint32_t offset(index);
    Page *page(mHead);
    Page *prev(0);

    while (offset >= ENTRIES_PER_PAGE)
    {
        // Create this page if it hasn't been created already.
        if (page == 0)
        {
            page = AllocatePage();
            prev->SetNext(page);
        }

        offset -= ENTRIES_PER_PAGE;
        prev = page;
        page = page->GetNext();
    }

    // As a special case, the first entry created in a new page needs to create the page.
    if (page == 0)
    {
        page = AllocatePage();
        prev->SetNext(page);
    }

    THERON_ASSERT(page);
    return page->GetEntry(offset);
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
THERON_FORCEINLINE void PageTable<EntryType, ENTRIES_PER_PAGE>::FreeEntry(const uint32_t /*index*/)
{
    // For now we never free pages once they have been allocated.
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
THERON_FORCEINLINE void *PageTable<EntryType, ENTRIES_PER_PAGE>::GetEntry(const uint32_t index)
{
    // Find the page containing the entry with the given index.
    // The page containing the entry, and all pages preceding it, should have already been allocated.
    // TODO: For now we traverse the allocated pages linearly.
    uint32_t offset(index);
    Page *page(mHead);

    while (offset >= ENTRIES_PER_PAGE)
    {
        offset -= ENTRIES_PER_PAGE;
        page = page->GetNext();
    }

    return page->GetEntry(offset);
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
inline typename PageTable<EntryType, ENTRIES_PER_PAGE>::Page *PageTable<EntryType, ENTRIES_PER_PAGE>::AllocatePage()
{
    IAllocator *const pageAllocator(AllocatorManager::GetCache());

    void *const pageMemory(pageAllocator->Allocate(sizeof(Page)));
    if (pageMemory == 0)
    {
        return 0;
    }

    return new (pageMemory) Page();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_ALLOCATORS_PAGETABLE_H

