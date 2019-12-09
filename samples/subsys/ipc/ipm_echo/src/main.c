/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app);


static void ipm_callback(void *context, u32_t id, volatile void *data)
{
	struct device *ipm = context;

	LOG_DBG("id 0x%x", id);

	LOG_HEXDUMP_DBG((void *)data, 4, "data");

	ipm_send(ipm, 0, 0, (void *)data, 4);
}

void main(void)
{
	struct device *ipm;

	LOG_DBG("Starting app");

	ipm = device_get_binding("ipm0");
	if (!ipm) {
		LOG_ERR("Cannot get IPM device");
		return;
	}

	ipm_register_callback(ipm, ipm_callback, ipm);
	ipm_set_enabled(ipm, 1);

	LOG_DBG("IPM initialized");

	u8_t data[] = { 0xde, 0xad };

	ipm_send(ipm, 0, 0, data, sizeof(data));

	while (1) {
	}
}
