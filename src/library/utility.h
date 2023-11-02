#pragma once

#include <zephyr/kernel.h>

static inline int64_t k_uptime_elapse(int64_t* reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;

	return delta;
}
