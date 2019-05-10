/*
 * IRgbLed.hpp
 *
 *  Created on: 21 avr. 2019
 *      Author: shang
 */

#pragma once

#include <cstdint>

namespace interfaces
{
class IRgbLed
{
public:
	virtual void red(bool setOn) = 0;
	virtual void red(float percent) = 0;

	virtual void green(bool setOn) = 0;
	virtual void green(float percent) = 0;

	virtual void blue(bool setOn) = 0;
	virtual void blue(float percent) = 0;
};
}
