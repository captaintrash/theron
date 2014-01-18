// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_CONTAINERS_PAGETABLE_H
#define THERON_DETAIL_CONTAINERS_PAGETABLE_H


#include <new>

#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Threading/Mutex.h>


namespace Theron
{
namespace Detail
{


/**
A paged table of entries, with the pages created on demand.
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
    Gets a reference to the entry with the given index.
    The table is grown if required by allocating more pages.
    */
    inline EntryType *GetEntry(const uint32_t index);

private:

    struct Page
    {
        inline Page() : mNext(0)
        {
        }

        EntryType mEntries[ENTRIES_PER_PAGE];
        Page *mNext;
    };

    PageTable(const PageTable &other);
    PageTable &operator=(const PageTable &other);

    inline static Page *AllocatePage();

    mutable Mutex mMutex;       ///< Ensures thread-safe access to the instance data.
    Page *mHead;                ///< Pointers to first page in a singly-linked list of pages.
};


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
inline PageTable<EntryType, ENTRIES_PER_PAGE>::PageTable() :
  mMutex(),
  mHead(0)
{
    // Pre-allocate the first page so head is always non-null.
    IAllocator *const pageAllocator(AllocatorManager::GetCache());
    void *const pageMemory(pageAllocator->AllocateAligned(sizeof(Page), THERON_CACHELINE_ALIGNMENT));
    mHead = new (pageMemory) Page();
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
        Page *const next(page->mNext);

        page->~Page();
        pageAllocator->Free(page, sizeof(Page));

        page = next;
    }
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
THERON_FORCEINLINE EntryType *PageTable<EntryType, ENTRIES_PER_PAGE>::GetEntry(const uint32_t index)
{
    mMutex.Lock();

    // Find the page containing the entry with the given index, creating any missing pages as we go.
    // TODO: For now we traverse the allocated pages linearly.
    uint32_t offset(index);
    Page *page(mHead);
    Page *prev(0);

    while (offset >= ENTRIES_PER_PAGE)
    {
        // Create this page if it hasn't been created already.
        if (page == 0)
        {
            page = AllocatePage();
            prev->mNext = page;
        }

        offset -= ENTRIES_PER_PAGE;
        prev = page;
        page = page->mNext;
    }

    // As a special case, the first entry created in a new page needs to create the page.
    if (page == 0)
    {
        page = AllocatePage();
        prev->mNext = page;
    }

    mMutex.Unlock();

    return page->mEntries + offset;
}


template <class EntryType, uint32_t ENTRIES_PER_PAGE>
inline typename PageTable<EntryType, ENTRIES_PER_PAGE>::Page *PageTable<EntryType, ENTRIES_PER_PAGE>::AllocatePage()
{
    IAllocator *const pageAllocator(AllocatorManager::GetCache());

    void *const pageMemory(pageAllocator->AllocateAligned(sizeof(Page), THERON_CACHELINE_ALIGNMENT));
    if (pageMemory == 0)
    {
        return 0;
    }

    return new (pageMemory) Page();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_CONTAINERS_PAGETABLE_H

