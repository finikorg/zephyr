/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include <drivers/edac.h>
#include <ibecc.h>

#include <app_memory/app_memdomain.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define STACK_SIZE 1024
#define PRIORITY 7
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
struct k_thread thread_id;

K_APPMEM_PARTITION_DEFINE(default_part);
K_APP_BMEM(default_part) static atomic_t handled;
K_APP_BMEM(default_part) static volatile uint32_t error_type;
K_APP_BMEM(default_part) static volatile uint64_t error_address;
K_APP_BMEM(default_part) static volatile uint16_t error_syndrome;

#if 0
/* Initialize statically message queue */
#define MAX_MSGS 4
K_MSGQ_DEFINE(mqueue, sizeof(uint8_t), MAX_MSGS, 4);
#endif

/**
 * Callback called from ISR. Runs in supervisor mode
 */
static void notification_callback(const struct device *dev, void *data)
{
	struct ibecc_error *error_data = data;

	/* Special care need to be taken for NMI callback:
	 * delayed_work, mutex and semaphores are not working stable
	 * here, using integer increment for now
	 */

	error_type = error_data->type;
	error_address = error_data->address;
	error_syndrome = error_data->syndrome;

	atomic_set(&handled, true);
#if 0
	int ret;
	ret = k_msgq_put(&mqueue, &test, K_NO_WAIT);
	if (ret) {
		LOG_ERR("k_msgq_put failed with %d", ret);
	}
#endif
}

#define DEVICE_NAME DT_LABEL(DT_NODELABEL(ibecc))

void thread_function(void *p1, void *p2, void *p3)
{
	LOG_INF("Thread started");
#if 1
	while (true) {
		if (atomic_cas(&handled, true, false)) {
			/* make nice printing */
			k_sleep(K_MSEC(300));
			printk("\nGot notification about IBECC event\n");
			printk("Parameters: %d %llx %x\n",
			       error_type, error_address, error_syndrome);
		}

		k_sleep(K_MSEC(300));
	}
#else
	while (true) {
		k_msgq_get(&mqueue, payload, K_FOREVER);
		LOG_INF("Got notification");
	}
#endif
}

void main(void)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		LOG_ERR("Cannot open EDAC device: %s", DEVICE_NAME);
		return;
	}

	if (edac_notify_callback_set(dev, notification_callback)) {
		LOG_ERR("Cannot set notification callback");
		return;
	}

	LOG_INF("EDAC shell application initialized");

	k_mem_domain_add_partition(&k_mem_domain_default, &default_part);

	k_thread_create(&thread_id, thread_stack, STACK_SIZE, thread_function,
			NULL, NULL, NULL, PRIORITY, K_USER, K_FOREVER);
#if 0
	//k_thread_access_grant(&thread_id, &mqueue);
#endif
	k_thread_start(&thread_id);
}
