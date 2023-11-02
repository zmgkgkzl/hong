#pragma once

#include <zephyr/kernel.h>
#include "bsp_config.h"

int  bsp_init(void);
void bsp_indication_set(bsp_indication_t indication);