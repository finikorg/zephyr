/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <device.h>
#include <shell/shell.h>
#include <drivers/ipm.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(shell_ipm, CONFIG_SHELL_IPM_LOG_LEVEL);

struct shell_ipm_data {
	bool initialized;
	/* IPM device */
	struct device *dev;
};

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_ipm_data *ipm_data =
		(struct shell_ipm_data *)transport->ctx;

	LOG_DBG("");

	if (ipm_data->initialized) {
		return -EINVAL;
	}

	ipm_data->initialized = true;
	ipm_data->dev = (struct device *)config;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_ipm_data *ipm_data =
		(struct shell_ipm_data *)transport->ctx;

	LOG_DBG("");

	if (!ipm_data->initialized) {
		return -ENODEV;
	}

	ipm_data->initialized = false;

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	struct shell_ipm_data *ipm_data =
		(struct shell_ipm_data *)transport->ctx;

	LOG_DBG("transport %p blocking %d", transport, blocking);

	if (!ipm_data->initialized) {
		return -ENODEV;
	}

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_ipm_data *ipm_data =
		(struct shell_ipm_data *)transport->ctx;
	const u8_t *data8 = (const u8_t *)data;

	LOG_DBG("length %d", length);

	if (!ipm_data->initialized) {
		*cnt = 0;
		return -ENODEV;
	}

	LOG_DBG("dev %p", ipm_data->dev);

	//ipm_send(ipm_data->dev, 1, data8[0], data8, 1);

	*cnt = length;
	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_ipm_data *ipm_data =
		(struct shell_ipm_data *)transport->ctx;

	if (!ipm_data->initialized) {
		return -ENODEV;
	}

	LOG_DBG("");

	*cnt = 0;

	return 0;
}

static const struct shell_transport_api transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static struct shell_ipm_data shell_transport_ipm_data;
struct shell_transport shell_transport_ipm = {
	.api = &transport_api,
	.ctx = &shell_transport_ipm_data,
};

SHELL_DEFINE(shell_ipm, CONFIG_SHELL_PROMPT_IPM, &shell_transport_ipm, 1, 0,
	     SHELL_FLAG_OLF_CRLF);

static int enable_shell_ipm(struct device *arg)
{
	ARG_UNUSED(arg);

	bool log_backend = CONFIG_SHELL_IPM_INIT_LOG_LEVEL > 0;
	u32_t level = (CONFIG_SHELL_IPM_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_IPM_INIT_LOG_LEVEL;
	struct device *dev;

	dev = device_get_binding("IPM_0");
	if (!dev) {
		LOG_ERR("Cannot get IPM device");
		return -ENODEV;
	}

	return shell_init(&shell_ipm, dev, true, log_backend, level);
}

SYS_INIT(enable_shell_ipm, APPLICATION, 0);

const struct shell *shell_backend_ipm_get_ptr(void)
{
	return &shell_ipm;
}


