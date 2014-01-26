// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXES_MAILBOX_H
#define THERON_DETAIL_MAILBOXES_MAILBOX_H


#include <Theron/Align.h>
#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/Queue.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Mailboxes/MessageQueue.h>
#include <Theron/Detail/Threading/SpinLock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{


class Actor;


namespace Detail
{


/**
An addressable message queue that can receive messages.
*/
class THERON_PREALIGN(THERON_CACHELINE_ALIGNMENT) Mailbox : public Queue<Mailbox>::Node
{
public:

    /**
    Default constructor.
    */
    inline Mailbox();

    /**
    Returns the index of this mailbox.
    */
    inline uint32_t GetIndex() const;

    /**
    Sets the index of this mailbox.
    */
    inline void SetIndex(const uint32_t index);

    /**
    Returns a reference to the mailbox's message queue.
    */
    inline MessageQueue &Queue();

    /**
    Returns a const reference to the mailbox's message queue.
    */
    inline const MessageQueue &Queue() const;

    /**
    Gets a reference to the timestamp value stored in the mailbox.
    */
    inline uint64_t &Timestamp();

    /**
    Gets a const-reference to the timestamp value stored in the mailbox.
    */
    inline const uint64_t &Timestamp() const;

private:

    uint32_t mIndex;                            ///< Index of this mailbox within the owning framework.
    MessageQueue mMessageQueue;                 ///< Stores messages queued in the mailbox.
    uint64_t mTimestamp;                        ///< Used for measuring mailbox scheduling latencies.

} THERON_POSTALIGN(THERON_CACHELINE_ALIGNMENT);


inline Mailbox::Mailbox() :
  mIndex(0),
  mMessageQueue(),
  mTimestamp(0)
{
}


THERON_FORCEINLINE uint32_t Mailbox::GetIndex() const
{
    return mIndex;
}


THERON_FORCEINLINE void Mailbox::SetIndex(const uint32_t index)
{
    mIndex = index;
}


THERON_FORCEINLINE MessageQueue &Mailbox::Queue()
{
    return mMessageQueue;
}


THERON_FORCEINLINE const MessageQueue &Mailbox::Queue() const
{
    return mMessageQueue;
}


THERON_FORCEINLINE uint64_t &Mailbox::Timestamp()
{
    return mTimestamp;
}


THERON_FORCEINLINE const uint64_t &Mailbox::Timestamp() const
{
    return mTimestamp;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXES_MAILBOX_H

