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


Directory::EntryTable *Directory::smEntryTable = 0;
Mutex Directory::smMutex;
uint32_t Directory::smReferenceCount = 0;
uint32_t Directory::smNextIndex = 0;


uint32_t Directory::Register(Entity *const entity)
{
    smMutex.Lock();

    // Create the singleton instance if this is the first reference.
    if (smReferenceCount++ == 0)
    {
        IAllocator *const allocator(AllocatorManager::GetCache());
        void *const memory(allocator->AllocateAligned(sizeof(EntryTable), THERON_CACHELINE_ALIGNMENT));

        if (memory == 0)
        {
            return 0;
        }

        smEntryTable = new (memory) EntryTable();
    }

    THERON_ASSERT(smEntryTable);

    // Allocate and initialize the entry.
    // Indices are offset by one to skip zero, which is reserved for null.
    const uint32_t index(++smNextIndex);
    Entry &entry(*smEntryTable->GetEntry(index));
    entry.Lock();
    entry.SetEntity(entity);
    entry.Unlock();

    smMutex.Unlock();

    return index;
}


void Directory::Deregister(const uint32_t index)
{
    smMutex.Lock();

    THERON_ASSERT(smEntryTable);
    THERON_ASSERT(index);

    // Clear the entry.
    // If the entry is pinned then we have to wait for it to be unpinned.
    Entry &entry(*smEntryTable->GetEntry(index));

    bool deregistered(false);
    while (!deregistered)
    {
        entry.Lock();

        if (!entry.IsPinned())
        {
            entry.Free();
            deregistered = true;
        }

        entry.Unlock();
    }

    // Destroy the singleton instance if this was the last reference.
    if (--smReferenceCount == 0)
    {
        IAllocator *const allocator(AllocatorManager::GetCache());
        smEntryTable->~EntryTable();
        allocator->Free(smEntryTable, sizeof(EntryTable));
    }

    smMutex.Unlock();
}


} // namespace Detail
} // namespace Theron


