#include "CriticalSection.hpp"
#include "../YggdrasilConfig.hpp"

namespace kernel {
        CriticalSection::CriticalSection() {
            core::Core::SupervisorCallHelper<ServiceCall::SvcNumber::enterCriticalSection, void()>();
        }

        CriticalSection::~CriticalSection() {
            core::Core::SupervisorCallHelper<ServiceCall::SvcNumber::exitCriticalSection, void()>();
        }
}