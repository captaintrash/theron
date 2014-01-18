// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_DIRECTORY_DIRECTORY_H
#define THERON_DETAIL_DIRECTORY_DIRECTORY_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/PageTable.h>
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
    Registers an entity and returns its allocated index.
    */
    static uint32_t Register(Entity *const entity);

    /**
    Deregisters a previously registered entity.
    */
    static void Deregister(const uint32_t index);

    /**
    Acquires exclusive access to the entity at the given index.
    Any attempts by other threads to Deregister the entity will block until a subsequent call to Release.
    */
    inline static Entity *Acquire(const uint32_t index);

    /**
    Releases exclusive access to the entity at the given index.
    */
    inline static bool Release(const uint32_t index);

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
          mEntity(0),
          mPinCount(0)
        {
        }

        /**
        Lock the entry, acquiring exclusive access.
        */
        THERON_FORCEINLINE void Lock() const
        {
            mSpinLock.Lock();
        }

        /**
        Unlock the entry, relinquishing exclusive access.
        */
        THERON_FORCEINLINE void Unlock() const
        {
            mSpinLock.Unlock();
        }

        /**
        Deregisters any entity registered at this entry.
        */
        THERON_FORCEINLINE void Free()
        {
            THERON_ASSERT(mPinCount == 0);
            mEntity = 0;
        }

        /**
        Registers the given entity at this entry.
        */
        THERON_FORCEINLINE void SetEntity(Entity *const entity)
        {
            THERON_ASSERT(mPinCount == 0);
            mEntity = entity;
        }

        /**
        Returns a pointer to the entity registered at this entry.
        \return A pointer to the registered entity, or zero if no entity is registered.
        */
        THERON_FORCEINLINE Entity *GetEntity() const
        {
            return mEntity;
        }

        /**
        Pins the entry, preventing the registered entry from being changed.
        */
        THERON_FORCEINLINE void Pin()
        {
            ++mPinCount;
        }

        /**
        Unpins the entry, allowing the registered entry to be changed.
        */
        THERON_FORCEINLINE void Unpin()
        {
            THERON_ASSERT(mPinCount > 0);
            --mPinCount;
        }

        /**
        Returns true if the entry has been pinned more times than unpinned.
        */
        THERON_FORCEINLINE bool IsPinned() const
        {
            return (mPinCount > 0);
        }

    private:

        Entry(const Entry &other);
        Entry &operator=(const Entry &other);

        mutable SpinLock mSpinLock;                 ///< Thread synchronization object protecting the entry.
        Entity *mEntity;                            ///< Pointer to the registered entity.
        uint32_t mPinCount;                         ///< Number of times this entity has been pinned and not unpinned.
    };

    typedef PageTable<Entry, 128> EntryTable;

    /**
    Gets a reference to the entry with the given index.
    The entry contains data about the entity (if any) registered at the index.
    */
    inline static Entry &Lookup(const uint32_t index);

    static EntryTable *smEntryTable;            ///< Pointer to the singleton page table.
    static Mutex smMutex;                       ///< Synchronization object protecting access.
    static uint32_t smReferenceCount;           ///< Counts the number of entities registered.
    static uint32_t smNextIndex;                 ///< For now we hand out mailboxes consecutively.
};


THERON_FORCEINLINE Directory::Entity *Directory::Acquire(const uint32_t index)
{
    Entity *entity(0);
    Entry &entry(Lookup(index));

    // Pin the entry and lookup the framework registered at the index.
    entry.Lock();
    entry.Pin();
    entity = entry.GetEntity();
    entry.Unlock();

    return entity;
}


THERON_FORCEINLINE bool Directory::Release(const uint32_t index)
{
    Entry &entry(Lookup(index));

    // Unpin the entry, allowing it to be changed by other threads.
    entry.Lock();
    entry.Unpin();
    entry.Unlock();

    return true;
}


THERON_FORCEINLINE Directory::Entry &Directory::Lookup(const uint32_t index)
{
    THERON_ASSERT(smEntryTable);
    THERON_ASSERT(index);

    return *smEntryTable->GetEntry(index);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_DIRECTORY_H

