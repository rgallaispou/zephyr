/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_ism330dlc

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ism330dlc_reg.h>
#include "stmemsc.h"

LOG_MODULE_REGISTER(ism330dlc, CONFIG_SENSOR_LOG_LEVEL);

#define ISM330DLC_RESET_TIMEOUT_MS 100
#define ISM330DLC_ACCEL_SENSITIVITY_UG 61
#define ISM330DLC_GYRO_SENSITIVITY_10UDPS 875

struct ism330dlc_config {
	struct i2c_dt_spec i2c;
	stmdev_ctx_t ctx;
};

struct ism330dlc_data {
	int16_t accel[3];
	int16_t gyro[3];
};

static int ism330dlc_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct ism330dlc_config *cfg = dev->config;
	struct ism330dlc_data *data = dev->data;
	bool fetch_accel = chan == SENSOR_CHAN_ALL ||
			   (chan >= SENSOR_CHAN_ACCEL_X &&
			    chan <= SENSOR_CHAN_ACCEL_XYZ);
	bool fetch_gyro = chan == SENSOR_CHAN_ALL ||
			  (chan >= SENSOR_CHAN_GYRO_X &&
			   chan <= SENSOR_CHAN_GYRO_XYZ);
	int16_t accel[3];
	int16_t gyro[3];

	if (!fetch_accel && !fetch_gyro) {
		return -ENOTSUP;
	}

	if (fetch_accel &&
	    ism330dlc_acceleration_raw_get(&cfg->ctx, accel) < 0) {
		return -EIO;
	}
	if (fetch_gyro &&
	    ism330dlc_angular_rate_raw_get(&cfg->ctx, gyro) < 0) {
		return -EIO;
	}

	if (fetch_accel) {
		memcpy(data->accel, accel, sizeof(accel));
	}
	if (fetch_gyro) {
		memcpy(data->gyro, gyro, sizeof(gyro));
	}
	return 0;
}

static int ism330dlc_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ism330dlc_data *data = dev->data;
	const int16_t *raw;
	bool accel;
	int axis;
	int count;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		accel = true;
		raw = data->accel;
		axis = chan == SENSOR_CHAN_ACCEL_XYZ ? 0 :
		       chan - SENSOR_CHAN_ACCEL_X;
		count = chan == SENSOR_CHAN_ACCEL_XYZ ? 3 : 1;
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		accel = false;
		raw = data->gyro;
		axis = chan == SENSOR_CHAN_GYRO_XYZ ? 0 :
		       chan - SENSOR_CHAN_GYRO_X;
		count = chan == SENSOR_CHAN_GYRO_XYZ ? 3 : 1;
		break;
	default:
		return -ENOTSUP;
	}

	for (int i = 0; i < count; i++) {
		int32_t raw_value = raw[axis + i];

		if (accel) {
			sensor_ug_to_ms2(raw_value *
					ISM330DLC_ACCEL_SENSITIVITY_UG,
					&val[i]);
		} else {
			sensor_10udegrees_to_rad(
				raw_value * ISM330DLC_GYRO_SENSITIVITY_10UDPS,
				&val[i]);
		}
	}
	return 0;
}

static int ism330dlc_init(const struct device *dev)
{
	const struct ism330dlc_config *cfg = dev->config;
	uint8_t chip_id;
	uint8_t reset;
	int64_t deadline;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		return -ENODEV;
	}

	if (ism330dlc_device_id_get(&cfg->ctx, &chip_id) < 0) {
		return -EIO;
	}
	if (chip_id != ISM330DLC_ID) {
		LOG_ERR("Unexpected chip ID: 0x%02x", chip_id);
		return -ENODEV;
	}

	if (ism330dlc_reset_set(&cfg->ctx, 1) < 0) {
		return -EIO;
	}

	deadline = k_uptime_get() + ISM330DLC_RESET_TIMEOUT_MS;
	do {
		if (ism330dlc_reset_get(&cfg->ctx, &reset) < 0) {
			return -EIO;
		}
		if (reset == 0U) {
			break;
		}
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_msleep(1);
	} while (true);

	if (ism330dlc_auto_increment_set(&cfg->ctx, 1) < 0 ||
	    ism330dlc_block_data_update_set(&cfg->ctx, 1) < 0 ||
	    ism330dlc_xl_full_scale_set(&cfg->ctx, ISM330DLC_2g) < 0 ||
	    ism330dlc_gy_full_scale_set(&cfg->ctx, ISM330DLC_250dps) < 0 ||
	    ism330dlc_xl_data_rate_set(&cfg->ctx, ISM330DLC_XL_ODR_104Hz) < 0 ||
	    ism330dlc_gy_data_rate_set(&cfg->ctx, ISM330DLC_GY_ODR_104Hz) < 0) {
		LOG_ERR("Failed to configure accelerometer/gyroscope");
		return -EIO;
	}

	/* Allow the gyroscope to settle after leaving power-down mode. */
	k_msleep(40);
	return 0;
}

static DEVICE_API(sensor, ism330dlc_api) = {
	.sample_fetch = ism330dlc_sample_fetch,
	.channel_get = ism330dlc_channel_get,
};

#define ISM330DLC_DEFINE(inst) \
	static struct ism330dlc_data ism330dlc_data_##inst; \
	static const struct ism330dlc_config ism330dlc_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		STMEMSC_CTX_I2C(&ism330dlc_config_##inst.i2c), \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ism330dlc_init, NULL, \
				    &ism330dlc_data_##inst, \
				    &ism330dlc_config_##inst, \
				    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				    &ism330dlc_api);

DT_INST_FOREACH_STATUS_OKAY(ISM330DLC_DEFINE)
