// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H
#define THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Allocators/PageTable.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Threading/Mutex.h>


namespace Theron
{


class Actor;


namespace Detail
{


/**
A collection of addressable mailboxes.
*/
class MailboxCollection
{
public:

    /**
    Allocates the mailbox with the given index.
    */
    void AllocateMailbox(const uint32_t index);

    /**
    Frees the mailbox with the given index.
    */
    void FreeMailbox(const uint32_t index);

    /**
    Gets a reference to the mailbox with the given index.
    The mailbox must have been previously allocated.
    */
    inline Mailbox &GetMailbox(const uint32_t index);

private:

    typedef PageTable<Mailbox, 1024> MailboxTable;

    Mutex mMutex;                   ///< Protects access to allocation and freeing of mailboxes.
    MailboxTable mMailboxTable;     ///< Paged mailbox allocator.
};


THERON_FORCEINLINE Mailbox &MailboxCollection::GetMailbox(const uint32_t index)
{
    THERON_ASSERT(index);
    return *reinterpret_cast<Mailbox *>(mMailboxTable.GetEntry(index));
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_MAILBOXES_MAILBOXCOLLECTION_H

