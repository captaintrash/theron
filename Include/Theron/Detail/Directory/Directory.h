// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_DIRECTORY_DIRECTORY_H
#define THERON_DETAIL_DIRECTORY_DIRECTORY_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Allocators/PageTable.h>
#include <Theron/Detail/Threading/Mutex.h>
#include <Theron/Detail/Threading/SpinLock.h>


namespace Theron
{
namespace Detail
{


/**
Reference-counted singleton that makes a collection of registered entities.
*/
class Directory
{
public:

    /**
    A non-copyable entity that can be registered in a directory.
    In order to be registered in the directory, types must derive from this class.
    */
    class Entity
    {
    public:

        inline Entity()
        {
        }

    private:

        Entity(const Entity &other);
        Entity &operator=(const Entity &other);
    };

    /**
    Default constructor.
    */
    Directory();

    /**
    Destructor.
    */
    ~Directory();

    /**
    Registers an entity and returns its unique index, or address.
    */
    uint32_t Register(Entity *const entity);

    /**
    Deregisters a previously registered entity.
    */
    void Deregister(const uint32_t index);

    /**
    Acquires exclusive access to the entity at the given index.
    Any attempts by other threads to Deregister the entity will block until a subsequent call to Release.
    */
    inline Entity *Acquire(const uint32_t index);

    /**
    Releases exclusive access to the entity at the given index.
    */
    inline void Release(const uint32_t index);

private:

    /**
    An entry in the directory, recording the registration of at most one entity.
    */
    class Entry
    {
    public:

        /**
        Default constructor.
        */
        inline Entry() :
          mSpinLock(),
          mIndex(0),
          mEntity(0),
          mNextFree(0)
        {
        }

        mutable SpinLock mSpinLock;                 ///< Thread synchronization object protecting the entry.
        uint32_t mIndex;                            ///< Index of this entry (TODO: Do we really need to store this?)
        Entity *mEntity;                            ///< Pointer to the registered entity.
        Entry *mNextFree;                           ///< For entries in the free list, a pointer to the next free entry.

    private:

        Entry(const Entry &other);
        Entry &operator=(const Entry &other);
    };

    typedef PageTable<Entry, 128> EntryTable;

    EntryTable mEntryTable;             ///< Page-allocated table of entries.
    Mutex mMutex;                       ///< Synchronization object protecting access.
    uint32_t mNextIndex;                ///< Remembers the next available index in the table.
    Entry *mFreeList;                   ///< Stores free entries available for reuse.
};


THERON_FORCEINLINE Directory::Entity *Directory::Acquire(const uint32_t index)
{
    // Lock the entry and get the entity registered with it.
    Entry *const entry(reinterpret_cast<Entry *>(mEntryTable.GetEntry(index)));
    entry->mSpinLock.Lock();
    return entry->mEntity;
}


THERON_FORCEINLINE void Directory::Release(const uint32_t index)
{
    // Unlock the entry, allowing it to be changed by other threads.
    Entry *const entry(reinterpret_cast<Entry *>(mEntryTable.GetEntry(index)));
    entry->mSpinLock.Unlock();
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_DIRECTORY_H

