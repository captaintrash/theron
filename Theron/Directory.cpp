// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/AllocatorManager.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>
#include <Theron/IAllocator.h>

#include <Theron/Detail/Directory/Directory.h>


namespace Theron
{
namespace Detail
{


Directory::Directory() :
  mEntryTable(),
  mMutex(),
  mNextIndex(0),
  mFreeList(0)
{
}


Directory::~Directory()
{
}


uint32_t Directory::Register(Entity *const entity)
{
    uint32_t index(0);
    Entry *entry(0);

    mMutex.Lock();

    // Take a previously freed entry from the free list if available.
    if (mFreeList)
    {
        entry = mFreeList;
        mFreeList = mFreeList->mNextFree;
        entry->mNextFree = 0;
        index = entry->mIndex;
    }
    else
    {
        // Indices are offset by one to skip zero, which is reserved for null.
        index = ++mNextIndex;
        void *const memory(mEntryTable.AllocateEntry(index));
        entry = new (memory) Entry();
        entry->mIndex = index;
    }

    entry->mSpinLock.Lock();
    entry->mEntity = entity;
    entry->mSpinLock.Unlock();

    mMutex.Unlock();

    return index;
}


void Directory::Deregister(const uint32_t index)
{
    mMutex.Lock();

    THERON_ASSERT(index);

    // Clear the entry. If the entry is locked then we have to wait for it to be unlocked.
    // This ensures that entities can't be deregistered while they're being processed.
    Entry *const entry(reinterpret_cast<Entry *>(mEntryTable.GetEntry(index)));

    entry->mSpinLock.Lock();
    entry->mEntity = 0;
    entry->mSpinLock.Unlock();

    // Return the entry to the free list for reuse.
    entry->mNextFree = mFreeList;
    mFreeList = entry;

    mMutex.Unlock();
}


} // namespace Detail
} // namespace Theron


