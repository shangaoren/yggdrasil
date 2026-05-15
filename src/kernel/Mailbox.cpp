/*MIT License

Copyright (c) 2026 Florian GERARD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Florian GERARD shall not be used
in advertising or otherwise to promote the sale, use or other dealings in this
Software without prior written authorization from Florian GERARD

*/

#include "Mailbox.hpp"

#include "../framework/assert.hpp"
#include "Scheduler.hpp"

namespace kernel {

namespace {

inline std::size_t nextIndex(std::size_t index, std::size_t capacity) noexcept {
    return (index + 1 == capacity) ? 0 : index + 1;
}

}  // namespace

MailboxStatus MailboxCore::post(MailboxCore* mailbox, const void* source) noexcept {
    return serviceCallMailboxPost_(mailbox, source);
}

MailboxStatus MailboxCore::kernelPost(MailboxCore* mailbox, const void* source) noexcept {
    y_assert(mailbox != nullptr);
    y_assert(source != nullptr);

    const std::size_t head = mailbox->head_.load(std::memory_order_relaxed);
    const std::size_t tail = mailbox->tail_.load(std::memory_order_acquire);
    const std::size_t next = nextIndex(head, mailbox->capacity_);
    if (next == tail) {
        return MailboxStatus::full;
    }
    std::memcpy(mailbox->storage_ + head * mailbox->slotSize_, source, mailbox->slotSize_);
    mailbox->head_.store(next, std::memory_order_release);
    return MailboxStatus::ok;
}

bool MailboxCore::tryPop(void* destination) noexcept {
    y_assert(destination != nullptr);
    assertOwner();

    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t head = head_.load(std::memory_order_acquire);
    if (head == tail) {
        return false;
    }
    std::memcpy(destination, storage_ + tail * slotSize_, slotSize_);
    tail_.store(nextIndex(tail, capacity_), std::memory_order_release);
    return true;
}

std::size_t MailboxCore::count() const noexcept {
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    return (head >= tail) ? (head - tail) : (capacity_ - tail + head);
}

bool MailboxCore::empty() const noexcept {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_relaxed);
}

void MailboxCore::reset() noexcept {
    assertOwner();
    tail_.store(head_.load(std::memory_order_acquire),
                std::memory_order_release);
}

void MailboxCore::assertOwner() const noexcept {
    y_assert(Scheduler::activeTask.load(std::memory_order_relaxed) == owner_);
}

MailboxCore::SupervisorMailboxPost MailboxCore::serviceCallMailboxPost_ =
    Core::SupervisorCallHelper<ServiceCall::SvcNumber::mailboxPost,
                               MailboxStatus(MailboxCore*, const void*)>::call;

}  // namespace kernel
