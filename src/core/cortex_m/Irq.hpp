#pragma once
#include "cstdint"
namespace core {
    class Irq
    {
    public:
        constexpr explicit Irq(const int16_t number) : interruptNumber_(number){}

        constexpr explicit operator int16_t() const
        {
            return interruptNumber_;
        }

    private:
        const int16_t interruptNumber_;
    };
}