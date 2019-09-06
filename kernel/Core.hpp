#pragma once

#include "yggdrasil/kernel/Processor.hpp"
#include "yggdrasil/interfaces/IVectorsManager.hpp"
#include "yggdrasil/interfaces/IClocks.hpp"
#include "yggdrasil/interfaces/ISystemTimer.hpp"

namespace kernel
{

	using namespace core::interfaces;
	class Core
	{
	public:
		constexpr Core(IVectorManager& vectorManager, ISystemTimer& SystemTimer, IClocks& coreClocks)
			: kVectorManager(vectorManager), kSystemTimer(SystemTimer), kCoreClocks(coreClocks)
		{
			
		}
		
		// Get the instance of vector manager
		IVectorManager& vectorManager()
		{
			return kVectorManager;
		}
		
		// Get the instance of system timer
		ISystemTimer& systemTimer()
		{
			return kSystemTimer;
		}
		
		// Get the system clocks tree controler
		IClocks& coreClocks()
		{
			return kCoreClocks;
		}
	private:
		IVectorManager& kVectorManager;
		ISystemTimer& kSystemTimer;
		IClocks& kCoreClocks;
		
	};
	}