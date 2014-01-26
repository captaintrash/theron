// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Detail/Mailboxes/MailboxCollection.h>


namespace Theron
{
namespace Detail
{


void MailboxCollection::AllocateMailbox(const uint32_t index)
{
    mMutex.Lock();

    void *const memory(mMailboxTable.AllocateEntry(index));
    Mailbox *const mailbox(new (memory) Mailbox());

    mailbox->SetIndex(index);

    mMutex.Unlock();
}


void MailboxCollection::FreeMailbox(const uint32_t index)
{
    mMutex.Lock();

    Mailbox *const mailbox(reinterpret_cast<Mailbox *>(mMailboxTable.GetEntry(index)));
    mailbox->~Mailbox();

    mMailboxTable.FreeEntry(index);

    mMutex.Unlock();
}


} // namespace Detail
} // namespace Theron


