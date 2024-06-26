/********************************************************************
* libavio/include/Clock.h
*
* Copyright (c) 2022  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>

namespace avio
{

class Clock
{
public:
	int64_t milliseconds();
	int64_t update(int64_t rts);
	int64_t stream_time();
	void pause(bool paused);

	int64_t correction = 0;

private:
	std::chrono::steady_clock clock;
	bool started = false;
	std::chrono::steady_clock::time_point play_start;
	std::chrono::steady_clock::time_point pause_start;

};

}

#endif // CLOCK_H