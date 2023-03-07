#pragma once
#include <cpp_rutils/date.h>

namespace cpp_cached {

	using time_point = std__chrono::utc_clock::time_point;
	// today 23:59:59
	time_point last_point_of_today();

}
