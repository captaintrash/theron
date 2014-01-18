// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H
#define THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/PageTable.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>


namespace Theron
{
namespace Detail
{


/**
A collection of addressable mailboxes.
The point of this class is really just to hide the implementation.
*/
class MailboxCollection
{
public:

    inline MailboxCollection();

    /**
    Allocates a mailbox in the collection and returns its index.
    */
    inline uint32_t AllocateMailbox();

    /**
    Frees the mailbox with the given index.
    */
    inline void FreeMailbox(const uint32_t index);

    /**
    Gets a reference to the mailbox with the given index.
    */
    inline Mailbox &GetMailbox(const uint32_t index);

private:

    typedef PageTable<Mailbox, 1024> MailboxTable;

    MailboxTable mMailboxTable;
    uint32_t mNextIndex;
};


inline MailboxCollection::MailboxCollection() : mNextIndex(0)
{
}


THERON_FORCEINLINE uint32_t MailboxCollection::AllocateMailbox()
{
    // Indices are offset by one to skip zero, which is reserved for null.
    return ++mNextIndex;
}


THERON_FORCEINLINE void MailboxCollection::FreeMailbox(const uint32_t /*index*/)
{
}


THERON_FORCEINLINE Mailbox &MailboxCollection::GetMailbox(const uint32_t index)
{
    THERON_ASSERT(index);
    return *mMailboxTable.GetEntry(index);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H

