/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(wolfson_wm8994, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT wolfson_wm8994

#define WM8994_SOFTWARE_RESET              0x0000U
#define WM8994_POWER_MANAGEMENT_1          0x0001U
#define WM8994_POWER_MANAGEMENT_2          0x0002U
#define WM8994_POWER_MANAGEMENT_3          0x0003U
#define WM8994_POWER_MANAGEMENT_4          0x0004U
#define WM8994_POWER_MANAGEMENT_5          0x0005U
#define WM8994_POWER_MANAGEMENT_6          0x0006U
#define WM8994_LEFT_OUTPUT_VOLUME          0x001CU
#define WM8994_RIGHT_OUTPUT_VOLUME         0x001DU
#define WM8994_AIF1_CLOCKING_1             0x0200U
#define WM8994_AIF1_RATE                   0x0210U
#define WM8994_AIF1_CONTROL_1              0x0300U
#define WM8994_AIF1_MASTER_SLAVE           0x0302U
#define WM8994_DAC1_LEFT_VOLUME            0x0610U
#define WM8994_DAC1_RIGHT_VOLUME           0x0611U
#define WM8994_DAC1_FILTERS_1              0x0420U

#define WM8994_SW_RESET_VALUE              0x8994U

#define WM8994_PM1_BIAS_ENA                BIT(0)
#define WM8994_PM1_VMID_SEL_MASK           (BIT(1) | BIT(2))
#define WM8994_PM1_VMID_NORMAL             BIT(2)
#define WM8994_PM2_MIXOUTL_ENA             BIT(5)
#define WM8994_PM2_MIXOUTR_ENA             BIT(4)
#define WM8994_PM3_DAC1L_ENA               BIT(0)
#define WM8994_PM3_DAC1R_ENA               BIT(1)
#define WM8994_PM4_ADCL_ENA                BIT(0)
#define WM8994_PM4_ADCR_ENA                BIT(1)
#define WM8994_PM5_AIF1DAC1L_ENA           BIT(0)
#define WM8994_PM5_AIF1DAC1R_ENA           BIT(1)
#define WM8994_PM5_AIF1ADC1L_ENA           BIT(4)
#define WM8994_PM5_AIF1ADC1R_ENA           BIT(5)
#define WM8994_PM6_DAC1L_TO_HPOUT1L        BIT(0)
#define WM8994_PM6_DAC1R_TO_HPOUT1R        BIT(1)

#define WM8994_AIF1CLK_ENA                 BIT(0)
#define WM8994_AIF1CLK_SRC_SHIFT           1U
#define WM8994_AIF1CLK_SRC_MASK            (BIT(1) | BIT(2))
#define WM8994_AIF1_SR_MASK                0x000FU
#define WM8994_AIF1CLK_RATE_MASK           0x00F0U
#define WM8994_AIF1CLK_RATE_SHIFT          4U
#define WM8994_AIF1_FMT_MASK               0x0003U
#define WM8994_AIF1_WL_MASK                0x0018U
#define WM8994_AIF1_WL_SHIFT               3U
#define WM8994_AIF1_MASTER                 BIT(0)
#define WM8994_DAC1_MUTE_MASK              (BIT(8) | BIT(9))
#define WM8994_DAC1_VOLUME_UPDATE          BIT(8)
#define WM8994_DAC1_VOLUME_MASK            0x00FFU
#define WM8994_OUTPUT_VOLUME_UPDATE        BIT(8)
#define WM8994_OUTPUT_VOLUME_MASK          0x003FU

#define WM8994_DEFAULT_DAC_VOLUME          0x00C0U
#define WM8994_DEFAULT_OUTPUT_VOLUME       0x0039U
#define WM8994_REGISTER_CACHE_SIZE         0x700U
#define WM8994_RESET_DELAY_US              1000U
#define WM8994_VMID_DELAY_US               50000U

enum wm8994_clock_source {
	WM8994_CLK_SRC_MCLK1,
	WM8994_CLK_SRC_MCLK2,
	WM8994_CLK_SRC_FLL1,
	WM8994_CLK_SRC_FLL2,
};

struct wm8994_config {
	struct i2c_dt_spec i2c;
	uint32_t mclk_freq;
	uint32_t clock_source;
};

struct wm8994_data {
	bool initialized;
	bool output_muted;
	uint16_t reg_cache[WM8994_REGISTER_CACHE_SIZE];
	uint8_t dac_volume_left;
	uint8_t dac_volume_right;
	uint8_t output_volume_left;
	uint8_t output_volume_right;
	audio_route_t route;
};

static int wm8994_reg_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct wm8994_config *config = dev->config;
	struct wm8994_data *data = dev->data;
	uint8_t buf[4];
	int ret;

	sys_put_be16(reg, &buf[0]);
	sys_put_be16(val, &buf[2]);

	ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%04x: %d", reg, ret);
		return ret;
	}

	if (reg < ARRAY_SIZE(data->reg_cache)) {
		data->reg_cache[reg] = val;
	}

	return 0;
}

static int wm8994_reg_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct wm8994_config *config = dev->config;
	struct wm8994_data *data = dev->data;
	uint8_t reg_buf[2];
	uint8_t val_buf[2];
	int ret;

	sys_put_be16(reg, reg_buf);

	ret = i2c_write_read_dt(&config->i2c, reg_buf, sizeof(reg_buf), val_buf, sizeof(val_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%04x: %d", reg, ret);
		return ret;
	}

	*val = sys_get_be16(val_buf);

	if (reg < ARRAY_SIZE(data->reg_cache)) {
		data->reg_cache[reg] = *val;
	}

	return 0;
}

static uint16_t wm8994_cached_reg_get(const struct device *dev, uint16_t reg)
{
	struct wm8994_data *data = dev->data;

	if (reg < ARRAY_SIZE(data->reg_cache)) {
		return data->reg_cache[reg];
	}

	return 0U;
}

static int wm8994_reg_update(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t val)
{
	uint16_t old_val = wm8994_cached_reg_get(dev, reg);
	uint16_t new_val = (old_val & ~mask) | (val & mask);

	if (new_val == old_val) {
		return 0;
	}

	return wm8994_reg_write(dev, reg, new_val);
}

static int wm8994_soft_reset(const struct device *dev)
{
	struct wm8994_data *data = dev->data;
	int ret;

	ret = wm8994_reg_write(dev, WM8994_SOFTWARE_RESET, WM8994_SW_RESET_VALUE);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(WM8994_RESET_DELAY_US);
	memset(data->reg_cache, 0, sizeof(data->reg_cache));
	data->reg_cache[WM8994_SOFTWARE_RESET] = WM8994_SW_RESET_VALUE;

	return 0;
}

static int wm8994_sample_rate_val(uint32_t rate, uint16_t *val)
{
	switch (rate) {
	case AUDIO_PCM_RATE_8K:
		*val = 0x0U;
		return 0;
	case AUDIO_PCM_RATE_11P025K:
	case 12000:
		*val = 0x1U;
		return 0;
	case AUDIO_PCM_RATE_16K:
		*val = 0x2U;
		return 0;
	case AUDIO_PCM_RATE_22P05K:
	case AUDIO_PCM_RATE_24K:
		*val = 0x3U;
		return 0;
	case AUDIO_PCM_RATE_32K:
		*val = 0x4U;
		return 0;
	case AUDIO_PCM_RATE_44P1K:
	case AUDIO_PCM_RATE_48K:
		*val = 0x5U;
		return 0;
	case AUDIO_PCM_RATE_96K:
		*val = 0x6U;
		return 0;
	case AUDIO_PCM_RATE_192K:
		*val = 0x7U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int wm8994_word_length_val(uint8_t word_size, uint16_t *val)
{
	switch (word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		*val = 0x0U;
		return 0;
	case AUDIO_PCM_WIDTH_20_BITS:
		*val = 0x1U;
		return 0;
	case AUDIO_PCM_WIDTH_24_BITS:
		*val = 0x2U;
		return 0;
	case AUDIO_PCM_WIDTH_32_BITS:
		*val = 0x3U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int wm8994_dai_format_val(audio_dai_type_t dai_type, uint16_t *val)
{
	switch (dai_type) {
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		*val = 0x0U;
		return 0;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		*val = 0x1U;
		return 0;
	case AUDIO_DAI_TYPE_I2S:
		*val = 0x2U;
		return 0;
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCMB:
		*val = 0x3U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int wm8994_validate_dai_cfg(struct audio_codec_cfg *cfg)
{
	uint16_t val;

	if (cfg->dai_cfg.i2s.channels == 0U || cfg->dai_cfg.i2s.channels > 2U) {
		LOG_ERR("Unsupported channel count: %u", cfg->dai_cfg.i2s.channels);
		return -ENOTSUP;
	}

	if (wm8994_sample_rate_val(cfg->dai_cfg.i2s.frame_clk_freq, &val) < 0) {
		LOG_ERR("Unsupported sample rate: %u", cfg->dai_cfg.i2s.frame_clk_freq);
		return -ENOTSUP;
	}

	if (wm8994_word_length_val(cfg->dai_cfg.i2s.word_size, &val) < 0) {
		LOG_ERR("Unsupported word size: %u", cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	if (wm8994_dai_format_val(cfg->dai_type, &val) < 0) {
		LOG_ERR("Unsupported DAI type: %d", cfg->dai_type);
		return -ENOTSUP;
	}

	return 0;
}

static int wm8994_configure_clocking(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct wm8994_config *config = dev->config;
	uint16_t sample_rate;
	uint32_t ratio;
	uint16_t aifclk_rate = 0U;
	int ret;

	ret = wm8994_sample_rate_val(cfg->dai_cfg.i2s.frame_clk_freq, &sample_rate);
	if (ret < 0) {
		return ret;
	}

	if (cfg->dai_cfg.i2s.frame_clk_freq == 0U) {
		return -EINVAL;
	}

	ratio = config->mclk_freq / cfg->dai_cfg.i2s.frame_clk_freq;
	switch (ratio) {
	case 64U:
		aifclk_rate = 0x0U;
		break;
	case 128U:
		aifclk_rate = 0x1U;
		break;
	case 192U:
		aifclk_rate = 0x2U;
		break;
	case 256U:
		aifclk_rate = 0x3U;
		break;
	case 384U:
		aifclk_rate = 0x4U;
		break;
	case 512U:
		aifclk_rate = 0x5U;
		break;
	case 768U:
		aifclk_rate = 0x6U;
		break;
	case 1024U:
		aifclk_rate = 0x7U;
		break;
	default:
		LOG_ERR("Unsupported MCLK/fs ratio: %u", ratio);
		return -ENOTSUP;
	}

	ret = wm8994_reg_update(dev, WM8994_AIF1_CLOCKING_1,
				  WM8994_AIF1CLK_SRC_MASK | WM8994_AIF1CLK_ENA,
				  (config->clock_source << WM8994_AIF1CLK_SRC_SHIFT) |
				  WM8994_AIF1CLK_ENA);
	if (ret < 0) {
		return ret;
	}

	return wm8994_reg_update(dev, WM8994_AIF1_RATE,
				  WM8994_AIF1CLK_RATE_MASK | WM8994_AIF1_SR_MASK,
				  (aifclk_rate << WM8994_AIF1CLK_RATE_SHIFT) | sample_rate);
}

static int wm8994_configure_audio_interface(const struct device *dev,
					    struct audio_codec_cfg *cfg)
{
	uint16_t format;
	uint16_t word_length;
	uint16_t master = 0U;
	int ret;

	ret = wm8994_dai_format_val(cfg->dai_type, &format);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_word_length_val(cfg->dai_cfg.i2s.word_size, &word_length);
	if (ret < 0) {
		return ret;
	}

	if ((cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_TARGET) == I2S_OPT_FRAME_CLK_CONTROLLER) {
		master = WM8994_AIF1_MASTER;
	}

	ret = wm8994_reg_update(dev, WM8994_AIF1_CONTROL_1,
				  WM8994_AIF1_FMT_MASK | WM8994_AIF1_WL_MASK,
				  format | (word_length << WM8994_AIF1_WL_SHIFT));
	if (ret < 0) {
		return ret;
	}

	return wm8994_reg_update(dev, WM8994_AIF1_MASTER_SLAVE, WM8994_AIF1_MASTER, master);
}

static int wm8994_power_bias(const struct device *dev, bool enable)
{
	uint16_t val = enable ? (WM8994_PM1_BIAS_ENA | WM8994_PM1_VMID_NORMAL) : 0U;
	int ret;

	ret = wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_1,
				  WM8994_PM1_BIAS_ENA | WM8994_PM1_VMID_SEL_MASK, val);
	if (ret < 0) {
		return ret;
	}

	if (enable) {
		k_busy_wait(WM8994_VMID_DELAY_US);
	}

	return 0;
}

static int wm8994_route_playback(const struct device *dev, bool enable)
{
	uint16_t pm2 = enable ? (WM8994_PM2_MIXOUTL_ENA | WM8994_PM2_MIXOUTR_ENA) : 0U;
	uint16_t pm3 = enable ? (WM8994_PM3_DAC1L_ENA | WM8994_PM3_DAC1R_ENA) : 0U;
	uint16_t pm5 = enable ? (WM8994_PM5_AIF1DAC1L_ENA | WM8994_PM5_AIF1DAC1R_ENA) : 0U;
	uint16_t pm6 = enable ? (WM8994_PM6_DAC1L_TO_HPOUT1L | WM8994_PM6_DAC1R_TO_HPOUT1R) : 0U;
	int ret;

	ret = wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_2,
				  WM8994_PM2_MIXOUTL_ENA | WM8994_PM2_MIXOUTR_ENA, pm2);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_3,
				  WM8994_PM3_DAC1L_ENA | WM8994_PM3_DAC1R_ENA, pm3);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_5,
				  WM8994_PM5_AIF1DAC1L_ENA | WM8994_PM5_AIF1DAC1R_ENA, pm5);
	if (ret < 0) {
		return ret;
	}

	return wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_6,
				  WM8994_PM6_DAC1L_TO_HPOUT1L | WM8994_PM6_DAC1R_TO_HPOUT1R, pm6);
}

static int wm8994_route_capture(const struct device *dev, bool enable)
{
	uint16_t pm4 = enable ? (WM8994_PM4_ADCL_ENA | WM8994_PM4_ADCR_ENA) : 0U;
	uint16_t pm5 = enable ? (WM8994_PM5_AIF1ADC1L_ENA | WM8994_PM5_AIF1ADC1R_ENA) : 0U;
	int ret;

	ret = wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_4,
				  WM8994_PM4_ADCL_ENA | WM8994_PM4_ADCR_ENA, pm4);
	if (ret < 0) {
		return ret;
	}

	return wm8994_reg_update(dev, WM8994_POWER_MANAGEMENT_5,
				  WM8994_PM5_AIF1ADC1L_ENA | WM8994_PM5_AIF1ADC1R_ENA, pm5);
}

static int wm8994_set_route(const struct device *dev, audio_route_t route)
{
	int ret;

	ret = wm8994_power_bias(dev, route != AUDIO_ROUTE_BYPASS);
	if (ret < 0) {
		return ret;
	}

	switch (route) {
	case AUDIO_ROUTE_BYPASS:
		ret = wm8994_route_playback(dev, false);
		if (ret < 0) {
			return ret;
		}

		return wm8994_route_capture(dev, false);
	case AUDIO_ROUTE_PLAYBACK:
		ret = wm8994_route_capture(dev, false);
		if (ret < 0) {
			return ret;
		}

		return wm8994_route_playback(dev, true);
	case AUDIO_ROUTE_CAPTURE:
		ret = wm8994_route_playback(dev, false);
		if (ret < 0) {
			return ret;
		}

		return wm8994_route_capture(dev, true);
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		ret = wm8994_route_playback(dev, true);
		if (ret < 0) {
			return ret;
		}

		return wm8994_route_capture(dev, true);
	default:
		return -EINVAL;
	}
}

static int wm8994_set_dac_mute(const struct device *dev, bool mute)
{
	struct wm8994_data *data = dev->data;
	uint16_t val = mute ? WM8994_DAC1_MUTE_MASK : 0U;
	int ret;

	ret = wm8994_reg_update(dev, WM8994_DAC1_FILTERS_1, WM8994_DAC1_MUTE_MASK, val);
	if (ret < 0) {
		return ret;
	}

	data->output_muted = mute;
	return 0;
}

static int wm8994_set_dac_volume(const struct device *dev, audio_channel_t channel, uint8_t volume,
					 bool store)
{
	struct wm8994_data *data = dev->data;
	uint16_t reg_val = WM8994_DAC1_VOLUME_UPDATE | volume;
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		ret = wm8994_reg_update(dev, WM8994_DAC1_LEFT_VOLUME,
					  WM8994_DAC1_VOLUME_UPDATE | WM8994_DAC1_VOLUME_MASK,
					  reg_val);
		if (ret < 0) {
			return ret;
		}

		if (store) {
			data->dac_volume_left = volume;
		}
		return 0;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		ret = wm8994_reg_update(dev, WM8994_DAC1_RIGHT_VOLUME,
					  WM8994_DAC1_VOLUME_UPDATE | WM8994_DAC1_VOLUME_MASK,
					  reg_val);
		if (ret < 0) {
			return ret;
		}

		if (store) {
			data->dac_volume_right = volume;
		}
		return 0;
	case AUDIO_CHANNEL_ALL:
		ret = wm8994_set_dac_volume(dev, AUDIO_CHANNEL_FRONT_LEFT, volume, store);
		if (ret < 0) {
			return ret;
		}

		return wm8994_set_dac_volume(dev, AUDIO_CHANNEL_FRONT_RIGHT, volume, store);
	default:
		return -EINVAL;
	}
}

static int wm8994_set_output_volume(const struct device *dev, audio_channel_t channel,
					    uint8_t volume, bool store)
{
	struct wm8994_data *data = dev->data;
	uint16_t reg_val = WM8994_OUTPUT_VOLUME_UPDATE | (volume & WM8994_OUTPUT_VOLUME_MASK);
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		ret = wm8994_reg_update(dev, WM8994_LEFT_OUTPUT_VOLUME,
					  WM8994_OUTPUT_VOLUME_UPDATE | WM8994_OUTPUT_VOLUME_MASK,
					  reg_val);
		if (ret < 0) {
			return ret;
		}

		if (store) {
			data->output_volume_left = volume;
		}
		return 0;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		ret = wm8994_reg_update(dev, WM8994_RIGHT_OUTPUT_VOLUME,
					  WM8994_OUTPUT_VOLUME_UPDATE | WM8994_OUTPUT_VOLUME_MASK,
					  reg_val);
		if (ret < 0) {
			return ret;
		}

		if (store) {
			data->output_volume_right = volume;
		}
		return 0;
	case AUDIO_CHANNEL_ALL:
		ret = wm8994_set_output_volume(dev, AUDIO_CHANNEL_FRONT_LEFT, volume, store);
		if (ret < 0) {
			return ret;
		}

		return wm8994_set_output_volume(dev, AUDIO_CHANNEL_FRONT_RIGHT, volume, store);
	default:
		return -EINVAL;
	}
}

static int wm8994_apply_properties(const struct device *dev)
{
	struct wm8994_data *data = dev->data;
	int ret;

	ret = wm8994_set_dac_volume(dev, AUDIO_CHANNEL_FRONT_LEFT, data->dac_volume_left, false);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_set_dac_volume(dev, AUDIO_CHANNEL_FRONT_RIGHT, data->dac_volume_right, false);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_set_output_volume(dev, AUDIO_CHANNEL_FRONT_LEFT,
					 data->output_volume_left, false);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_set_output_volume(dev, AUDIO_CHANNEL_FRONT_RIGHT,
					 data->output_volume_right, false);
	if (ret < 0) {
		return ret;
	}

	return wm8994_set_dac_mute(dev, data->output_muted);
}

static int wm8994_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct wm8994_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		return -ENODEV;
	}

	ret = wm8994_validate_dai_cfg(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	data->route = cfg->dai_route;

	ret = wm8994_configure_clocking(dev, cfg);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_configure_audio_interface(dev, cfg);
	if (ret < 0) {
		return ret;
	}

	ret = wm8994_set_route(dev, cfg->dai_route);
	if (ret < 0) {
		return ret;
	}

	return wm8994_apply_properties(dev);
}

static void wm8994_start_output(const struct device *dev)
{
	(void)wm8994_set_dac_mute(dev, false);
}

static void wm8994_stop_output(const struct device *dev)
{
	(void)wm8994_set_dac_mute(dev, true);
}

static int wm8994_set_property(const struct device *dev, audio_property_t property,
				       audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return wm8994_set_output_volume(dev, channel,
						CLAMP(val.vol, 0, WM8994_OUTPUT_VOLUME_MASK), true);
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (channel != AUDIO_CHANNEL_ALL &&
		    channel != AUDIO_CHANNEL_FRONT_LEFT &&
		    channel != AUDIO_CHANNEL_FRONT_RIGHT &&
		    channel != AUDIO_CHANNEL_HEADPHONE_LEFT &&
		    channel != AUDIO_CHANNEL_HEADPHONE_RIGHT) {
			return -EINVAL;
		}

		return wm8994_set_dac_mute(dev, val.mute);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return -ENOTSUP;
	case AUDIO_PROPERTY_INPUT_MUTE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}
}

static int wm8994_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(input);

	if (channel != AUDIO_CHANNEL_FRONT_LEFT && channel != AUDIO_CHANNEL_FRONT_RIGHT &&
	    channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return -ENOTSUP;
}

static int wm8994_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(output);

	if (channel != AUDIO_CHANNEL_FRONT_LEFT &&
	    channel != AUDIO_CHANNEL_FRONT_RIGHT &&
	    channel != AUDIO_CHANNEL_HEADPHONE_LEFT &&
	    channel != AUDIO_CHANNEL_HEADPHONE_RIGHT &&
	    channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return -ENOTSUP;
}

static DEVICE_API(audio_codec, wm8994_driver_api) = {
	.configure = wm8994_configure,
	.start_output = wm8994_start_output,
	.stop_output = wm8994_stop_output,
	.set_property = wm8994_set_property,
	.apply_properties = wm8994_apply_properties,
	.route_input = wm8994_route_input,
	.route_output = wm8994_route_output,
};

static int wm8994_init(const struct device *dev)
{
	const struct wm8994_config *config = dev->config;
	struct wm8994_data *data = dev->data;
	uint16_t device_id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = wm8994_reg_read(dev, WM8994_SOFTWARE_RESET, &device_id);
	if (ret < 0) {
		return ret;
	}

	if (device_id != WM8994_SW_RESET_VALUE) {
		LOG_ERR("Unexpected device ID: 0x%04x", device_id);
		return -EINVAL;
	}

	data->dac_volume_left = WM8994_DEFAULT_DAC_VOLUME;
	data->dac_volume_right = WM8994_DEFAULT_DAC_VOLUME;
	data->output_volume_left = WM8994_DEFAULT_OUTPUT_VOLUME;
	data->output_volume_right = WM8994_DEFAULT_OUTPUT_VOLUME;
	data->output_muted = true;
	data->route = AUDIO_ROUTE_BYPASS;
	data->initialized = true;

	LOG_INF("%s initialized", dev->name);
	return 0;
}

#define WM8994_INIT(inst)                                                                       \
	static struct wm8994_data wm8994_data_##inst;                                             \
	static const struct wm8994_config wm8994_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                   \
		.mclk_freq = DT_INST_PROP_OR(inst, mclk_frequency, 12288000U),                       \
		.clock_source = DT_INST_ENUM_IDX(inst, clock_source),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, wm8994_init, NULL, &wm8994_data_##inst,                       \
			      &wm8994_config_##inst, POST_KERNEL,                                      \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &wm8994_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WM8994_INIT)