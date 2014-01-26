// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_MAILBOXES_MESSAGEQUEUE_H
#define THERON_DETAIL_MAILBOXES_MESSAGEQUEUE_H


#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>

#include <Theron/Detail/Containers/Queue.h>
#include <Theron/Detail/Messages/IMessage.h>
#include <Theron/Detail/Threading/SpinLock.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable:4324)  // structure was padded due to __declspec(align())
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
A lockable queue of messages.
*/
class MessageQueue
{
public:

    inline MessageQueue();

    /**
    Lock the queue, acquiring exclusive access.
    */
    inline void Lock() const;

    /**
    Unlock the queue, relinquishing exclusive access.
    */
    inline void Unlock() const;

    /**
    Returns true if the queue contains no messages.
    */
    inline bool Empty() const;

    /**
    Pushes a message into the queue.
    */
    inline void Push(IMessage *const message);

    /**
    Peeks at the first message in the queue.
    The message is inspected without actually being removed.
    \note It's illegal to call this method when the queue is empty.
    */
    inline IMessage *Front() const;

    /**
    Pops the first message from the queue.
    \note It's illegal to call this method when the queue is empty.
    */
    inline IMessage *Pop();

    /**
    Returns the number of messages currently queued in the queue.
    */
    inline uint32_t Count() const;

private:

    mutable SpinLock mLock;         ///< Thread synchronization object protecting the queue.
    uint32_t mCount;                ///< Number of messages in the queue.
    Queue<IMessage> mQueue;         ///< Queue of messages.

};


inline MessageQueue::MessageQueue() :
  mLock(),
  mCount(0),
  mQueue()
{
}


THERON_FORCEINLINE void MessageQueue::Lock() const
{
    mLock.Lock();
}


THERON_FORCEINLINE void MessageQueue::Unlock() const
{
    mLock.Unlock();
}


THERON_FORCEINLINE bool MessageQueue::Empty() const
{
    return mQueue.Empty();
}


THERON_FORCEINLINE void MessageQueue::Push(IMessage *const message)
{
    mQueue.Push(message);
    ++mCount;
}


THERON_FORCEINLINE IMessage *MessageQueue::Front() const
{
    return mQueue.Front();
}


THERON_FORCEINLINE IMessage *MessageQueue::Pop()
{
    --mCount;
    return mQueue.Pop();
}


THERON_FORCEINLINE uint32_t MessageQueue::Count() const
{
    return mCount;
}


} // namespace Detail
} // namespace Theron


#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


#endif // THERON_DETAIL_MAILBOXES_MESSAGEQUEUE_H

