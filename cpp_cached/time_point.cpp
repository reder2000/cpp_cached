#include "time_point.h"

namespace cpp_cached {

	time_point last_point_of_today()
	{
		auto now = std__chrono::utc_clock::now();
		auto nowsys = std__chrono::utc_clock::to_sys(now);
		auto nowsystmt = std::chrono::system_clock::to_time_t(nowsys);
		auto nowsysstructtm = to_<struct tm>::_(nowsystmt);
		nowsysstructtm.tm_hour = 23;
		nowsysstructtm.tm_min = 59;
		nowsysstructtm.tm_sec = 59;
		nowsystmt = to_<time_t>::_(nowsysstructtm);
		nowsys = std::chrono::system_clock::from_time_t(nowsystmt);
		auto now_midnight = std__chrono::utc_clock::from_sys(nowsys);
		return now_midnight;
	}

}