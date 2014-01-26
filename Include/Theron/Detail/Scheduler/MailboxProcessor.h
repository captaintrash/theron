// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
#define THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Handlers/FallbackHandlerCollection.h>
#include <Theron/Detail/Mailboxes/Mailbox.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Messages/MessageCreator.h>
#include <Theron/Detail/Scheduler/WorkerContext.h>


namespace Theron
{


class Actor;


namespace Detail
{


/**
Processes mailboxes that have received messages.
*/
class MailboxProcessor
{
public:

    inline static void Process(WorkerContext *const workerContext, Mailbox *const mailbox);
    
private:

    MailboxProcessor(const MailboxProcessor &other);
    MailboxProcessor &operator=(const MailboxProcessor &other);
};


THERON_FORCEINLINE void MailboxProcessor::Process(WorkerContext *const workerContext, Mailbox *const mailbox)
{
    // Load the context data from the worker thread's mailbox context.
    MailboxContext *const mailboxContext(&workerContext->mMailboxContext);
    Directory *const actorDirectory(mailboxContext->mActorDirectory);
    FallbackHandlerCollection *const fallbackHandlers(mailboxContext->mFallbackHandlers);
    IAllocator *const messageAllocator(mailboxContext->mMessageAllocator);

    THERON_ASSERT(fallbackHandlers);
    THERON_ASSERT(messageAllocator);

    // Remember the mailbox we're processing in the context so we can query it.
    mailboxContext->mMailbox = mailbox;

    // Lookup the actor registered against the mailbox.
    // Acquire exclusive access to prevent the actor from being deregistered.
    const uint32_t mailboxIndex(mailbox->GetIndex());
    Actor *const actor(static_cast<Actor *>(actorDirectory->Acquire(mailboxIndex)));

    // Get the first queued message.
    // At this point the mailbox shouldn't be enqueued in any other work items,
    // even if it contains more than one enqueued message. This ensures that
    // each mailbox is only processed by one worker thread at a time.
    mailbox->Queue().Lock();
    IMessage *const message(mailbox->Queue().Front());
    mailbox->Queue().Unlock();

    // If an actor is registered at the mailbox then process it.
    if (actor)
    {
        actor->ProcessMessage(mailboxContext, fallbackHandlers, message);
    }
    else
    {
        fallbackHandlers->Handle(message);
    }

    // Pop the message we just processed from the mailbox, then check whether the
    // mailbox is now empty, and reschedule the mailbox if it's not.
    // The locking of the mailbox here and in the main scheduling ensures that
    // mailboxes are always enqueued if they have unprocessed messages, but at most
    // once at any time.
    mailbox->Queue().Lock();

    mailbox->Queue().Pop();
    if (!mailbox->Queue().Empty())
    {
        mailboxContext->mScheduler->Schedule(mailboxContext, mailbox);
    }

    mailbox->Queue().Unlock();

    actorDirectory->Release(mailboxIndex);

    // Destroy the message, but only after we've popped it from the queue.
    MessageCreator::Destroy(messageAllocator, message);
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
