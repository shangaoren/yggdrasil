#pragma once
#include <cstdint>
#include "yggdrasil/interfaces/IVectorsManager.hpp"

namespace core
{
	namespace interfaces
	{
		class ISystemTimer
		{
		public:
			virtual void initSystemTimer(uint32_t coreFrequency, uint32_t ticksFrequency) = 0;
			virtual void startSystemTimer() = 0;
			virtual IVectorManager::Irq getIrq() =0;
		};
	}	//End namespace interfaces
}//End namespace core