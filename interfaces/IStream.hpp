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
#include <functional>


namespace interfaces
{

	template<typename StreamType>
		class IStream
		{
		public:
			
			
			//interface to send data througth stream
			virtual bool send(const StreamType* data, uint16_t size, int64_t timeout)
			{
				return (beginSend(data, size) && endSend(timeout));
			}
	
			//interface to send data througth stream, typically starts DMA
			virtual bool beginSend(const StreamType* data, uint16_t size) = 0;
	
			
			//interface to wait for send to finish, typically wait for DMA interrupt
			virtual bool endSend(int64_t timeout) = 0;
	
			
			//function to setup callback for when data was received
			void onReceive(std::function<void(StreamType* data, uint16_t size, bool eof)> callback)
			{
				m_receiveCallback = callback;
			}

		protected:
	
			//interface to receive data 
			virtual void dataReceived(StreamType* data, uint16_t size, bool eof)
			{
				if (m_receiveCallback != nullptr)
					m_receiveCallback(data, size, eof);
			}
	
			//interface to setup receive interface, must be called after each datareceive
			virtual void receive(StreamType* data, uint16_t size, bool eof) = 0;

	
	
		private:
			std::function<void(StreamType* data, uint16_t size, bool eof)> m_receiveCallback;
	
		};
	
}

