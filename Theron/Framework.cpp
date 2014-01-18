// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#include <new>

#include <Theron/Actor.h>
#include <Theron/Assert.h>
#include <Theron/AllocatorManager.h>
#include <Theron/Defines.h>
#include <Theron/EndPoint.h>
#include <Theron/Framework.h>
#include <Theron/IAllocator.h>
#include <Theron/Receiver.h>

#include <Theron/Detail/Directory/Directory.h>
#include <Theron/Detail/Scheduler/BlockingMonitor.h>
#include <Theron/Detail/Scheduler/MailboxQueue.h>
#include <Theron/Detail/Scheduler/NonBlockingMonitor.h>
#include <Theron/Detail/Scheduler/Scheduler.h>
#include <Theron/Detail/Network/Index.h>
#include <Theron/Detail/Network/NameGenerator.h>
#include <Theron/Detail/Strings/String.h>
#include <Theron/Detail/Threading/Utils.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4996)  // function or variable may be unsafe
#endif //_MSC_VER


namespace Theron
{


void Framework::Initialize()
{
    mScheduler = CreateScheduler();

    // Set up the scheduler.
    mScheduler->Initialize(mParams.mThreadCount);

    // Set up the default fallback handler, which catches and reports undelivered messages.
    SetFallbackHandler(&mDefaultFallbackHandler, &Detail::DefaultFallbackHandler::Handle);
     
    // Register the framework and get a non-zero index for it, unique within the local process.
    mIndex = Detail::Directory::Register(this);
    THERON_ASSERT(mIndex);

    // If the framework name wasn't set explicitly then generate a default name.
    if (mName.IsNull())
    {
        char buffer[16];
        Detail::NameGenerator::Generate(buffer, mIndex);
        mName = Detail::String(buffer);
    }
}


void Framework::Release()
{
    // Deregister the framework.
    Detail::Directory::Deregister(mIndex);

    mScheduler->Release();
    DestroyScheduler(mScheduler);
    mScheduler = 0;
}


Detail::IScheduler *Framework::CreateScheduler()
{
    typedef Detail::MailboxQueue<Detail::BlockingMonitor> BlockingQueue;
    typedef Detail::MailboxQueue<Detail::NonBlockingMonitor> NonBlockingQueue;

    typedef Detail::Scheduler<BlockingQueue> BlockingScheduler;
    typedef Detail::Scheduler<NonBlockingQueue> NonBlockingScheduler;

    IAllocator *const allocator(AllocatorManager::GetCache());
    void *schedulerMemory(0);

    if (mParams.mYieldStrategy == YIELD_STRATEGY_CONDITION)
    {
        schedulerMemory = allocator->AllocateAligned(
            sizeof(BlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);
    }
    else
    {
        schedulerMemory = allocator->AllocateAligned(
            sizeof(NonBlockingScheduler),
            THERON_CACHELINE_ALIGNMENT);
    }

    THERON_ASSERT_MSG(schedulerMemory, "Failed to allocate scheduler");

    if (mParams.mYieldStrategy == YIELD_STRATEGY_CONDITION)
    {
        return new (schedulerMemory) BlockingScheduler(
            &mMailboxes,
            &mFallbackHandlers,
            &mMessageAllocator,
            &mSharedMailboxContext,
            mParams.mNodeMask,
            mParams.mProcessorMask,
            mParams.mThreadPriority,
            mParams.mYieldStrategy);
    }
    else
    {
        return new (schedulerMemory) NonBlockingScheduler(
            &mMailboxes,
            &mFallbackHandlers,
            &mMessageAllocator,
            &mSharedMailboxContext,
            mParams.mNodeMask,
            mParams.mProcessorMask,
            mParams.mThreadPriority,
            mParams.mYieldStrategy);
    }
}


void Framework::DestroyScheduler(Detail::IScheduler *const scheduler)
{
    IAllocator *const allocator(AllocatorManager::GetCache());

    scheduler->~IScheduler();
    allocator->Free(scheduler);
}


void Framework::RegisterActor(Actor *const actor, const char *const name)
{
    // Allocate an unused mailbox.
    const uint32_t mailboxIndex(mMailboxes.AllocateMailbox());
    Detail::Mailbox &mailbox(mMailboxes.GetMailbox(mailboxIndex));

    // Use the provided name for the actor if one was provided.
    Detail::String mailboxName(name);
    if (name == 0)
    {
        char rawName[16];
        Detail::NameGenerator::Generate(rawName, mailboxIndex);

        const char *endPointName(0);
        if (mEndPoint)
        {
            endPointName = mEndPoint->GetName();        
        }

        char scopedName[256];
        Detail::NameGenerator::Combine(
            scopedName,
            256,
            rawName,
            mName.GetValue(),
            endPointName);

        mailboxName = Detail::String(scopedName);
    }

    // Name the mailbox and register the actor.
    mailbox.Lock();
    mailbox.SetName(mailboxName);
    mailbox.RegisterActor(actor);
    mailbox.Unlock();

    // Create the unique address of the mailbox.
    // Its a pair comprising the framework index and the mailbox index within the framework.
    const Detail::Index index(mIndex, mailboxIndex);
    const Address mailboxAddress(mailboxName, index);

    // Set the actor's mailbox address.
    // The address contains the index of the framework and the index of the mailbox within the framework.
    actor->mAddress = mailboxAddress;

    if (mEndPoint)
    {
        // Check that no mailbox with this name already exists.
        Detail::Index dummy;
        if (mEndPoint->Lookup(mailboxName, dummy))
        {
            THERON_FAIL_MSG("Can't create two actors or receivers with the same name");
        }
        
        // Register the mailbox with the endPoint so it can be found using its name.
        if (!mEndPoint->Register(mailboxName, index))
        {
            THERON_FAIL_MSG("Failed to register actor with the network endpoint");
        }
    }
}


void Framework::DeregisterActor(Actor *const actor)
{
    const Address address(actor->GetAddress());
    const Detail::String &mailboxName(address.GetName());

    // Deregister the mailbox with the endPoint so it can't be found anymore.
    if (mEndPoint)
    {
        mEndPoint->Deregister(mailboxName);
    }

    // Deregister the actor, so that the worker threads will leave it alone.
    const uint32_t mailboxIndex(address.AsInteger());
    Detail::Mailbox &mailbox(mMailboxes.GetMailbox(mailboxIndex));

    // If the entry is pinned then we have to wait for it to be unpinned.
    bool deregistered(false);
    uint32_t backoff(0);

    while (!deregistered)
    {
        mailbox.Lock();

        if (!mailbox.IsPinned())
        {
            mailbox.DeregisterActor();
            deregistered = true;
        }

        mailbox.Unlock();

        Detail::Utils::Backoff(backoff);
    }

    mMailboxes.FreeMailbox(mailboxIndex);
}


bool Framework::DeliverWithinLocalProcess(Detail::IMessage *const message, const Detail::Index &index)
{
    const uint32_t targetFrameworkIndex(index.mComponents.mFramework);

    THERON_ASSERT(index.mUInt32 != 0);

    // Is the message addressed to a receiver? Receiver addresses have zero framework indices.
    if (targetFrameworkIndex == 0)
    {
        // Get the receiver registered at the addressed index.
        Receiver *const receiver(static_cast<Receiver *>(Detail::Directory::Acquire(index.mComponents.mIndex)));

        // If a receiver is registered at the mailbox then deliver the message to it.
        if (receiver)
        {
            receiver->Push(message);
        }

        // Release the receiver, allowing it to be deregistered by other threads.
        Detail::Directory::Release(index.mComponents.mIndex);

        return (receiver != 0);
    }

    bool delivered(false);

    // Get the framework registered at the addressed index.
    Framework *const framework(static_cast<Framework *>(Detail::Directory::Acquire(index.mComponents.mFramework)));

    // If a framework is registered at this index then forward the message to it.
    if (framework)
    {
        // The address is just an index with no name.
        const Address address(Detail::String(), index);
        delivered = framework->FrameworkReceive(message, address);
    }

    // Release the framework, allowing it to be deregistered by other threads.
    Detail::Directory::Release(index.mComponents.mFramework);

    return delivered;
}


} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER
