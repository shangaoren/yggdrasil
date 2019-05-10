/*
 * ILed.hpp
 *
 *  Created on: 20 avr. 2019
 *      Author: shang
 */
#pragma once

namespace interfaces
{
class ILed
{
public:
	virtual void state(bool setOn) = 0;
	virtual void percent(float) = 0;
};
}
