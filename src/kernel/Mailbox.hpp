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

// Mailbox<T, N>: MPSC bounded message queue.
//   - Producers (any task) publish via SVC; the kernel performs the memcpy.
//   - Consumer is a single task (the owner), reads lock-free via atomic indices.
//   - Storage lives inside the Mailbox object itself, so it belongs to the
//     owner task's memory region (prepares MMU-based isolation).
//   - v1 scope: no blocking receive (poll), no postFromIsr, no multi-consumer.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "../YggdrasilConfig.hpp"
#include "ServiceCall.hpp"
#include "Task.hpp"

namespace kernel {

enum class MailboxStatus : uint16_t {
    ok   = 0,
    full = 1,
};

// Non-templated core. The kernel and the templated wrapper both operate on this.
class MailboxCore {
    friend class Scheduler;

public:
    constexpr MailboxCore(TaskController& owner,
                          std::byte* storage,
                          std::size_t slotSize,
                          std::size_t capacity,
                          const char* name) noexcept
        : owner_(&owner),
          storage_(storage),
          slotSize_(slotSize),
          capacity_(capacity),
          name_(name) {}

    MailboxCore(const MailboxCore&)            = delete;
    MailboxCore& operator=(const MailboxCore&) = delete;
    MailboxCore(MailboxCore&&)                 = delete;
    MailboxCore& operator=(MailboxCore&&)      = delete;

    // Producer-side entrypoint: issues the SVC. Called by Mailbox<T, N>::post.
    [[nodiscard]] static MailboxStatus post(MailboxCore* mailbox, const void* source) noexcept;

    // Owner-side, lock-free. Returns false if empty.
    [[nodiscard]] bool tryPop(void* destination) noexcept;

    // Owner-side, lock-free queries.
    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    void reset() noexcept;

    [[nodiscard]] const char* name() const noexcept { return name_; }

private:
    // SVC handler entrypoint. Runs in handler mode at kKernelPriority, so
    // concurrent producers are naturally serialized.
    static MailboxStatus kernelPost(MailboxCore* mailbox, const void* source) noexcept;

    void assertOwner() const noexcept;

    TaskController* const owner_;
    std::byte* const      storage_;
    const std::size_t     slotSize_;
    const std::size_t     capacity_;   // N — one slot stays sentinel, N-1 usable
    const char* const     name_;

    std::atomic<std::size_t> head_{0};  // written ONLY by kernelPost (SVC side)
    std::atomic<std::size_t> tail_{0};  // written ONLY by the owner task

    using SupervisorMailboxPost = MailboxStatus(&)(MailboxCore*, const void*);
    static SupervisorMailboxPost& serviceCallMailboxPost_;
};

// Public templated wrapper. Owns the typed storage.
template <typename T, std::size_t N>
class Mailbox {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Mailbox<T, N> requires a trivially copyable T");
    static_assert(N >= 2,
                  "Mailbox capacity must be >= 2 (one slot is sentinel)");

public:
    template <uint32_t StackSize>
    constexpr Mailbox(Task<StackSize>& owner, const char* name) noexcept
        : core_(owner.controller(), storage_, sizeof(T), N, name) {}

    Mailbox(const Mailbox&)            = delete;
    Mailbox& operator=(const Mailbox&) = delete;
    Mailbox(Mailbox&&)                 = delete;
    Mailbox& operator=(Mailbox&&)      = delete;

    // Producer-side: callable from any task. Issues an SVC.
    [[nodiscard]] MailboxStatus post(const T& msg) noexcept {
        static_assert(Config::kEnableMailbox,
                      "Mailbox is disabled: set Config::kEnableMailbox = true "
                      "in KernelConfig.hpp");
        return MailboxCore::post(&core_, &msg);
    }

    // Consumer-side: owner only, lock-free.
    [[nodiscard]] bool tryReceive(T& output) noexcept { return core_.tryPop(&output); }
    [[nodiscard]] std::size_t count() const noexcept { return core_.count(); }
    [[nodiscard]] bool empty() const noexcept { return core_.empty(); }
    void reset() noexcept { core_.reset(); }

    [[nodiscard]] const char* name() const noexcept { return core_.name(); }

private:
    alignas(T) std::byte storage_[N * sizeof(T)];
    MailboxCore core_;
};

}  // namespace kernel
