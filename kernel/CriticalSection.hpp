#pragma once

namespace kernel {
    class CriticalSection {
    public:
        CriticalSection(CriticalSection&) = delete;
        CriticalSection(CriticalSection&&) = delete;
        CriticalSection();
        ~CriticalSection();
    };
}
