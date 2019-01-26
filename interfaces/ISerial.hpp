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

class ISerial
{
public :

	virtual bool sendData(const uint8_t* data, uint16_t size) = 0;
	virtual bool receiveData(uint8_t* buffer, uint16_t size, bool eof) = 0;
	virtual bool circularReceive(uint8_t* buffer, uint16_t size, bool eof) = 0;

	void onDataReceived(std::function<void(uint8_t* data, uint16_t size, bool eof)> callback)
	{
		m_dataReceived = callback;
	}

	void onDataSend(std::function<void(void)> callback)
	{
		m_dataSend = callback;
	}

protected:
	void dataReceived(uint8_t* data, uint16_t size, bool eof)
	{
		if(m_dataReceived != nullptr)
			m_dataReceived(data,size,eof);
	}

	void dataSend()
	{
		if(m_dataSend != nullptr)
			m_dataSend();
	}

private:
	std::function<void(uint8_t* data, uint16_t size, bool eof)> m_dataReceived = nullptr;
	std::function<void(void)> m_dataSend = nullptr;


};
}
