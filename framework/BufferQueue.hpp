/*MIT License

Copyright (c) 2018 Florian GERARD

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


#pragma once

#include <cstdint>
#include "yggdrasil/kernel/Event.hpp"

namespace framework
{
	template<typename BufferType, uint32_t bufferSize, uint32_t bufferNumber>
		class BufferQueue
		{
		public:
			
			struct Data
			{
				BufferType buffer[bufferSize];
				uint32_t usedSize;
				Data* previous;
				Data* next;
			};
			
			
			//Create a bufferQueue and init it
			constexpr BufferQueue() : 
				m_inRead(nullptr),
				m_inWrite(nullptr),
				m_filled(nullptr),
				m_newDataAvailable(kernel::Event()),
				m_newBufferAvailable(kernel::Event(true))
			{
				m_available = m_source[0];
				m_available->previous = nullptr;
				Data* iterator = m_available;
				for (uint32_t i = 1; i < bufferNumber; i++)
				{
					iterator->next = m_source[i];
					m_source[i].previous = iterator;
					iterator = iterator->next;
				}
				iterator->next = nullptr;
			}
			
			
			
			~BufferQueue();
			
			//return the number of bufferType data in a buffer
			uint32_t size()
			{
				return bufferSize;
			}
			
			kernel::Event& newDataAvailable()
			{
				return m_newDataAvailable;
			}
			
			kernel::Event& newBufferAvailable()
			{
				return m_newBufferAvailable;
			}
			
			
			
			//return the next buffer to be written, nullptr if none available
			Data* nextData()
			{
				if (m_filled == nullptr)
					return nullptr;
				if (m_inWrite != nullptr)
					return m_inWrite;
				m_inWrite = m_filled;
				m_filled = m_filled->next;
				m_filled->previous = nullptr;
				m_inWrite->next = nullptr;
				m_inWrite->previous = nullptr;
				return m_inWrite;
			}
			
			
			//return the next buffer to be read, nullptr if no data available
			Data* nextAvailable()
			{
				if (m_available == nullptr)
					return nullptr;
				if (m_inRead != nullptr)
					return m_inRead;	
				m_inRead = m_available;
				m_available = m_available->next;
				m_available->previous = nullptr;
				m_inRead->next = nullptr;
				m_inRead->previous = nullptr;
				return m_inRead;
				
			}
			
			
			//release a buffer after reading
			bool setAvailable()
			{
				if (m_inRead == nullptr)
					return false;
				if (m_available == nullptr)
				{
					m_available = m_inRead;	
					m_inRead->usedSize = 0;
					m_inRead = nullptr;
				}
				else
				{
					Data* iterator = m_available;
					while (iterator->next != nullptr)
						iterator = iterator->next;
					iterator->next = m_inRead;
					m_inRead->previous = iterator;
					m_inRead->usedSize = 0;
					m_inRead = nullptr;
				}
				
				if (m_newBufferAvailable.someoneWaiting())
					m_newBufferAvailable.signal();
				return true;
			}
			
			
			//Write is finished, put in filled list
			bool setFilled()
			{
				if (m_inWrite == nullptr)
					return false;
				if (m_filled == nullptr)
				{
					m_filled = m_inWrite;
					m_inWrite = nullptr;
				}
				else
				{
					Data* iterator = m_filled;
					while (iterator->next != nullptr)
						iterator = iterator->next;
					iterator->next = m_inWrite;
					m_inWrite->previous = iterator;
				}
				m_inWrite = nullptr;
				if(m_newDataAvailable.someoneWaiting())
					m_newDataAvailable.signal();
				return true;
			}
	

	
	private:
			kernel::Event m_newDataAvailable;
			kernel::Event m_newBufferAvailable;
			
			Data m_source[bufferNumber];
			Data* m_available;
			Data* m_filled;
			Data* m_inRead;
			Data* m_inWrite;
		};
	
}

