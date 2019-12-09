/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <posix/unistd.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#if 1
static struct device *ipm_console_device;

static int console_out(int c)
{
	u32_t id = c & ~BIT(31);

	LOG_DBG("");

	/* Send character with busy wait */
	ipm_send(ipm_console_device, 1, id, &id, sizeof(id));

	return c;
}
#endif

void main(void)
{
	int i = 0;

	LOG_DBG("");

	ipm_console_device = device_get_binding("IPM_0");

	__stdout_hook_install(console_out);
	__printk_hook_install(console_out);

	while (1) {
		printk("Hello World! %s\n", CONFIG_BOARD);
		//ret = ipm_send(dev, 1, 0xde, NULL, 0);

		LOG_DBG("i %d", i);
		sleep(5);
		i++;
	}
}
