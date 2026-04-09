/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 * Author: Raphaël Gallais-Pou <raphael.gallais-pou@foss.st.com>
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/audio/codec.h>
#include <zephyr/devicetree/clocks.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wolfson_wm8994, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#include "wm8994.h"

#define DT_DRV_COMPAT wolfson_wm8994

struct wm8994_driver_config {
	struct i2c_dt_spec i2c;
	int clock_source;
	const struct device *mclk_dev;
	clock_control_subsys_t mclk_name;
};

#define DEV_CFG(dev) ((const struct wm8994_driver_config *const)dev->config)

static void wm8994_write_reg(const struct device *dev, uint16_t reg, uint16_t val);
static void wm8994_read_reg(const struct device *dev, uint16_t reg, uint16_t *val);
static void wm8994_update_reg(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t val);
static void wm8994_soft_reset(const struct device *dev);
#if DEBUG_WM8994_REGISTER
static void wm8994_read_all_reg(const struct device *dev, uint16_t endAddress);
#endif

static void wm8994_configure_output(const struct device *dev);

static void wm8994_configure_input(const struct device *dev);

static int wm8994_apply_properties(const struct device *dev);

static int wm8994_start_sequence(const struct device *dev, wm8994_sequence_id_t id)
{
	uint32_t delayUs = 93000U;
	uint16_t sequenceStat = 0U;

	switch (id) {
	case kwm8994_SequenceDACToHeadphonePowerUp:
		delayUs = 93000U;
		break;
	case kwm8994_SequenceAnalogueInputPowerUp:
		delayUs = 75000U;
		break;
	case kwm8994_SequenceChipPowerDown:
		delayUs = 32000U;
		break;
	case kwm8994_SequenceSpeakerSleep:
		delayUs = 2000U;
		break;
	case kwm8994_SequenceSpeakerWake:
		delayUs = 2000U;
		break;
	default:
		delayUs = 93000U;
		break;
	}

	wm8994_write_reg(dev, WM8994_REG_WRITE_SEQ_CTRL_1, WM8994_WSEQ_ENA);
	wm8994_write_reg(dev, WM8994_REG_WRITE_SEQ_CTRL_2, (uint16_t)id);
	while (delayUs != 0U) {
		wm8994_read_reg(dev, WM8994_REG_WRITE_SEQ_CTRL_3, &sequenceStat);
		if ((sequenceStat & 1U) == 0U) {
			break;
		}
		k_msleep(1U);
		delayUs -= 1000U;
	}

	return (sequenceStat & 1U) == 0U ? 0 : -EBUSY;
}

static int wm8994_get_clock_divider(uint32_t inputClock, uint32_t maxClock, uint16_t *divider)
{
	if ((inputClock >> 2U) > maxClock) {
		return -EINVAL;
	}

	/* fll reference clock divider */
	if (inputClock > maxClock) {
		if ((inputClock >> 1U) > maxClock) {
			*divider = 2U;
		} else {
			*divider = 1U;
		}
	} else {
		*divider = 0U;
	}

	return 0;
}

static int wm8994_protocol_config(const struct device *dev, audio_dai_type_t dai_type)
{
	wm8994_protocol_t proto;

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		proto = kwm8994_BusI2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		proto = kwm8994_BusLeftJustified;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		proto = kwm8994_BusRightJustified;
		break;
	case AUDIO_DAI_TYPE_PCMA:
		proto = kwm8994_BusPCMA - 1;
		break;
	case AUDIO_DAI_TYPE_PCMB:
		proto = kwm8994_BusPCMB | 0x10U;
		break;
	default:
		return -EINVAL;
	}

	wm8994_update_reg(dev, WM8994_REG_IFACE0, WM8994_IFACE0_FORMAT_MASK, (uint16_t)proto);

	LOG_DBG("Codec protocol: %#x", proto);
	return 0;
}

static int wm8994_audio_fmt_config(const struct device *dev, audio_dai_cfg_t *cfg, uint32_t mclk)
{
	uint32_t val;
	uint16_t word_size = cfg->i2s.word_size;
	uint32_t ratio = mclk / cfg->i2s.frame_clk_freq;

	switch (word_size) {
	case 16:
		val = WM8994_IFACE0_WL_16BITS;
		break;
	case 20:
		val = WM8994_IFACE0_WL_20BITS;
		break;
	case 24:
		val = WM8994_IFACE0_WL_24BITS;
		break;
	case 32:
		val = WM8994_IFACE0_WL_32BITS;
		break;
	default:
		LOG_WRN("Invalid codec bit width: %d", cfg->i2s.word_size);
		return -EINVAL;
	}

	wm8994_update_reg(dev, WM8994_REG_IFACE0, WM8994_IFACE0_WL_MASK, WM8994_IFACE0_WL(val));

	switch (cfg->i2s.frame_clk_freq) {
	case kwm8994_AudioSampleRate8kHz:
		val = 0x15U;
		break;
	case kwm8994_AudioSampleRate11025Hz:
		val = 0x04U;
		break;
	case kwm8994_AudioSampleRate12kHz:
		val = 0x14U;
		break;
	case kwm8994_AudioSampleRate16kHz:
		val = 0x13U;
		break;
	case kwm8994_AudioSampleRate22050Hz:
		val = 0x02U;
		break;
	case kwm8994_AudioSampleRate24kHz:
		val = 0x12U;
		break;
	case kwm8994_AudioSampleRate32kHz:
		val = 0x11U;
		break;
	case kwm8994_AudioSampleRate44100Hz:
		val = 0x00U;
		break;
	case kwm8994_AudioSampleRate48kHz:
		val = 0x10U;
		break;
	case kwm8994_AudioSampleRate88200Hz:
		val = 0x06U;
		break;
	case kwm8994_AudioSampleRate96kHz:
		val = 0x16U;
		break;
	default:
		LOG_WRN("Invalid codec sample rate: %d", cfg->i2s.frame_clk_freq);
		return -EINVAL;
	}

	wm8994_write_reg(dev, WM8994_REG_ADDCTL3, val);

	switch (ratio) {
	case 64:
		val = 0x00U;
		break;
	case 128:
		val = 0x02U;
		break;
	case 192:
		val = 0x04U;
		break;
	case 256:
		val = 0x06U;
		break;
	case 384:
		val = 0x08U;
		break;
	case 512:
		val = 0x0AU;
		break;
	case 768:
		val = 0x0CU;
		break;
	case 1024:
		val = 0x0EU;
		break;
	case 1536:
		val = 0x12U;
		break;
	case 3072:
		val = 0x14U;
		break;
	case 6144:
		val = 0x16U;
		break;
	default:
		LOG_WRN("Invalid codec ratio: %d", ratio);
		return -EINVAL;
	}

	wm8994_write_reg(dev, WM8994_REG_CLK4, val);

	return 0;
}

static int wm8994_out_update(const struct device *dev, audio_channel_t channel, uint16_t val,
			     uint16_t mask)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		wm8994_update_reg(dev, WM8994_REG_LOUT2, mask, val);
		return 0;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		wm8994_update_reg(dev, WM8994_REG_ROUT2, mask, val);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		wm8994_update_reg(dev, WM8994_REG_LOUT1, mask, val);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		wm8994_update_reg(dev, WM8994_REG_ROUT1, mask, val);
		return 0;

	case AUDIO_CHANNEL_ALL:
		wm8994_update_reg(dev, WM8994_REG_LOUT1, mask, val);
		wm8994_update_reg(dev, WM8994_REG_ROUT1, mask, val);
		wm8994_update_reg(dev, WM8994_REG_LOUT2, mask, val);
		wm8994_update_reg(dev, WM8994_REG_ROUT2, mask, val);
		return 0;

	default:
		return -EINVAL;
	}
}

static int wm8994_out_volume_config(const struct device *dev, audio_channel_t channel, int volume)
{
	/* Set volume values with VU = 0 */
	const uint16_t val = WM8994_REGVAL_OUT_VOL(1, 0, volume);
	const uint16_t mask =
		wm8994_REGMASK_OUT_VU | wm8994_REGMASK_OUT_ZC | wm8994_REGMASK_OUT_VOL;

	return wm8994_out_update(dev, channel, val, mask);
}

static int wm8994_out_mute_config(const struct device *dev, audio_channel_t channel, bool mute)
{
	uint8_t val = 0U;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		val = mute ? 2U : 0U;
		wm8994_update_reg(dev, WM8994_REG_CLASSD1, WM8994_L_CH_MUTE_MASK, val);
		return 0;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		val = mute ? 1U : 0U;
		wm8994_update_reg(dev, WM8994_REG_CLASSD1, WM8994_R_CH_MUTE_MASK, val);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		val = mute ? 2U : 0U;
		wm8994_update_reg(dev, WM8994_REG_POWER2, WM8994_L_CH_MUTE_MASK, val);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		val = mute ? 1U : 0U;
		wm8994_update_reg(dev, WM8994_REG_POWER2, WM8994_R_CH_MUTE_MASK, val);
		return 0;

	case AUDIO_CHANNEL_ALL:
		val = mute ? 3U : 0U;
		wm8994_update_reg(dev, wm8994_REG_CLASSD1,
				  (wm8994_L_CH_MUTE_MASK | wm8994_R_CH_MUTE_MASK), val);
		wm8994_update_reg(dev, wm8994_REG_POWER2,
				  (wm8994_L_CH_MUTE_MASK | wm8994_R_CH_MUTE_MASK), val);
		return 0;

	default:
		return -EINVAL;
	}
}

static int wm8994_in_update(const struct device *dev, audio_channel_t channel, uint16_t mask,
			    uint16_t val)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		wm8994_update_reg(dev, wm8994_REG_LINVOL, mask, val);
		return 0;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		wm8994_update_reg(dev, wm8994_REG_RINVOL, mask, val);
		return 0;

	case AUDIO_CHANNEL_ALL:
		wm8994_update_reg(dev, wm8994_REG_LINVOL, mask, val);
		wm8994_update_reg(dev, wm8994_REG_RINVOL, mask, val);
		return 0;

	default:
		return -EINVAL;
	}
}

static int wm8994_in_volume_config(const struct device *dev, audio_channel_t channel, int volume)
{
	const uint16_t val = WM8994_REGVAL_IN_VOL(1, 0, 0, volume);
	const uint16_t mask = WM8994_REGMASK_IN_MUTE;

	return wm8994_in_update(dev, channel, mask, val);
}

static int wm8994_in_mute_config(const struct device *dev, audio_channel_t channel, bool mute)
{
	const uint16_t val = WM8994_REGVAL_IN_VOL(1, mute, 0, 0);
	const uint16_t mask = wm8994_REGMASK_IN_MUTE;

	return wm8994_in_update(dev, channel, mask, val);
}

static int wm8994_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	uint8_t reg;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		reg = WM8994_REG_LEFT_INPUT_PGA;
		break;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		reg = WM8994_REG_RIGHT_INPUT_PGA;
		break;

	default:
		return -EINVAL;
	}

	/* Input PGA source */
	wm8994_write_reg(dev, reg, input);
	return 0;
}

static int wm8994_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	/* Output MIXER */
	switch (channel) {
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		wm8994_write_reg(dev, wm8994_REG_LEFT_HEADPHONE_MIXER, output);
		break;
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		wm8994_write_reg(dev, wm8994_REG_RIGHT_HEADPHONE_MIXER, output);
		break;
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_REAR_LEFT:
	case AUDIO_CHANNEL_SIDE_LEFT:
		wm8994_write_reg(dev, wm8994_REG_LEFT_SPEAKER_MIXER, output);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_REAR_RIGHT:
	case AUDIO_CHANNEL_SIDE_RIGHT:
		wm8994_write_reg(dev, wm8994_REG_RIGHT_SPEAKER_MIXER, output);
		break;
	default:
		break;
	}

	return 0;
}

static void wm8994_set_master_clock(const struct device *dev, audio_dai_cfg_t *cfg, uint32_t sysclk)
{
	uint32_t sampleRate = cfg->i2s.frame_clk_freq;
	uint32_t bitWidth = cfg->i2s.word_size;
	uint32_t bclkDiv = 0U;
	uint16_t regClkDiv = 0U, sysClkDiv = 0U;
	int ret = 0;

	wm8994_get_clock_divider(sysclk, wm8994_MAX_DSP_CLOCK, &sysClkDiv);
	sysclk /= 1 << sysClkDiv;

	bclkDiv = sysclk / (sampleRate * bitWidth * 2U);

	switch (bclkDiv) {
	case 1:
		regClkDiv = 0U;
		break;
	case 2:
		regClkDiv = 2U;
		break;
	case 3:
		regClkDiv = 3U;
		break;
	case 4:
		regClkDiv = 4U;
		break;
	case 6:
		regClkDiv = 6U;
		break;
	case 8:
		regClkDiv = 7U;
		break;
	case 12:
		regClkDiv = 9U;
		break;
	case 16:
		regClkDiv = 10U;
		break;
	case 24:
		regClkDiv = 11U;
		break;
	case 32:
		regClkDiv = 13U;
		break;

	default:
		ret = -1;
		break;
	}
	if (ret == 0) {
		wm8994_update_reg(dev, wm8994_REG_CLOCK2, wm8994_CLOCK2_BCLK_DIV_MASK,
				  (uint16_t)regClkDiv);
		wm8994_write_reg(dev, wm8994_REG_IFACE2, (uint16_t)(bitWidth * 2U));
	} else {
		LOG_ERR("Unsupported divider.");
	}
}

static int wm8994_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint32_t sysClk = 0;
	uint16_t clockDiv = 0U;

	const struct wm8994_driver_config *const dev_cfg = DEV_CFG(dev);

	if (cfg->dai_type >= AUDIO_DAI_TYPE_INVALID) {
		LOG_ERR("dai_type not supported");
		return -EINVAL;
	}

	if (dev_cfg->clock_source == 0) {
		int err = clock_control_on(dev_cfg->mclk_dev, dev_cfg->mclk_name);

		if (err < 0) {
			LOG_ERR("MCLK clock source enable fail: %d", err);
		}

		err = clock_control_get_rate(dev_cfg->mclk_dev, dev_cfg->mclk_name,
					     &cfg->mclk_freq);
		if (err < 0) {
			LOG_ERR("MCLK clock source freq acquire fail: %d", err);
		}
	}

	wm8994_soft_reset(dev);
	if (cfg->dai_route == AUDIO_ROUTE_BYPASS) {
		return 0;
	}

	/* disable internal osc/FLL2/FLL3/FLL */
	wm8994_write_reg(dev, WM8994_REG_PLL2, 0);
	wm8994_update_reg(dev, WM8994_REG_FLL_CTRL_1, 1U, 0U);
	wm8994_write_reg(dev, WM8994_REG_CLOCK2, 0x9E4);
	wm8994_write_reg(dev, WM8994_REG_POWER1, 0x1FE);
	wm8994_write_reg(dev, WM8994_REG_POWER2, 0x1E0);

	if ((cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_TARGET) == 0) {
		wm8994_set_master_clock(dev, &cfg->dai_cfg, cfg->mclk_freq);
		wm8994_update_reg(dev, wm8994_REG_IFACE0, 1U << 6U, 1U << 6U);
	}

	wm8994_start_sequence(dev, kwm8994_SequenceDACToHeadphonePowerUp);
	wm8994_start_sequence(dev, kwm8994_SequenceAnalogueInputPowerUp);
	wm8994_start_sequence(dev, kwm8994_SequenceSpeakerWake);

	/* enable system clock */
	wm8994_update_reg(dev, wm8994_REG_CLOCK2, 0x20U, 0x20U);

	/* sysclk clock divider, maximum 12.288MHZ */
	wm8994_read_reg(dev, wm8994_REG_CLOCK1, &clockDiv);
	sysClk = cfg->mclk_freq / (1UL << (clockDiv & 3U));

	/* set data protocol */
	wm8994_protocol_config(dev, cfg->dai_type);
	/*
	 * ADC volume, 0dB
	 */
	wm8994_write_reg(dev, wm8994_REG_LADC, WM8994_ADC_DEFAULT_VOLUME_VALUE);
	wm8994_write_reg(dev, wm8994_REG_RADC, WM8994_ADC_DEFAULT_VOLUME_VALUE);
	/*
	 * Digital DAC volume, -15.5dB
	 */
	wm8994_write_reg(dev, wm8994_REG_LDAC, WM8994_DAC_DEFAULT_VOLUME_VALUE);
	wm8994_write_reg(dev, wm8994_REG_RDAC, WM8994_DAC_DEFAULT_VOLUME_VALUE);
	/* speaker volume 6dB */
	wm8994_write_reg(dev, wm8994_REG_LOUT2, WM8994_SPEAKER_DEFAULT_VOLUME_VALUE);
	wm8994_write_reg(dev, wm8994_REG_ROUT2, WM8994_SPEAKER_DEFAULT_VOLUME_VALUE);
	/* input PGA volume */
	wm8994_write_reg(dev, wm8994_REG_LINVOL, WM8994_LINEIN_DEFAULT_VOLUME_VALUE);
	wm8994_write_reg(dev, wm8994_REG_RINVOL, WM8994_LINEIN_DEFAULT_VOLUME_VALUE);
	/* Headphone volume */
	wm8994_write_reg(dev, wm8994_REG_LOUT1, WM8994_HEADPHONE_DEFAULT_VOLUME_VALUE);
	wm8994_write_reg(dev, wm8994_REG_ROUT1, WM8994_HEADPHONE_DEFAULT_VOLUME_VALUE);
	wm8994_audio_fmt_config(dev, &cfg->dai_cfg, sysClk);

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_BYPASS:

		break;
	case AUDIO_ROUTE_PLAYBACK:
		wm8994_configure_output(dev);
		break;

	case AUDIO_ROUTE_CAPTURE:
		wm8994_configure_input(dev);
		break;

	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		wm8994_configure_output(dev);
		wm8994_configure_input(dev);
		break;

	default:
		break;
	}

	return 0;
}

static void wm8994_start_output(const struct device *dev)
{
	/* Not supported */
}

static void wm8994_stop_output(const struct device *dev)
{
	/* Not supported */
}

static int wm8994_set_property(const struct device *dev, audio_property_t property,
			       audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return wm8994_out_volume_config(dev, channel, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return wm8994_out_mute_config(dev, channel, val.mute);

	case AUDIO_PROPERTY_INPUT_VOLUME:
		return wm8994_in_volume_config(dev, channel, val.vol);

	case AUDIO_PROPERTY_INPUT_MUTE:
		return wm8994_in_mute_config(dev, channel, val.mute);
	default:
		break;
	}

	return -EINVAL;
}

static int wm8994_apply_properties(const struct device *dev)
{
	/**
	 * Set VU = 1 for all input and output channels, VU takes effect for the whole
	 * channel pair.
	 */
	wm8994_update_reg(dev, wm8994_REG_LOUT1, WM8994_REGVAL_OUT_VOL(1, 0, 0),
			  wm8994_REGMASK_OUT_VU);
	wm8994_update_reg(dev, wm8994_REG_LINVOL, WM8994_REGVAL_IN_VOL(1, 0, 0, 0),
			  wm8994_REGMASK_IN_VU);

	return 0;
}

static void wm8994_write_reg(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct wm8994_driver_config *const dev_cfg = DEV_CFG(dev);
	uint8_t data[4];
	int ret;

	/* data is reversed */
	data[0] = (reg >> 8) & 0xff;
	data[1] = reg & 0xff;
	data[2] = (val >> 8) & 0xff;
	data[3] = val & 0xff;

	ret = i2c_write(dev_cfg->i2c.bus, data, 4, dev_cfg->i2c.addr);

	if (ret != 0) {
		LOG_ERR("i2c write to codec error %d", ret);
	}

	LOG_DBG("REG:%#02x VAL:%#02x", reg, val);
}

static void wm8994_read_reg(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct wm8994_driver_config *const dev_cfg = DEV_CFG(dev);
	uint16_t value;
	int ret;

	reg = WM8994_SWAP_UINT16_BYTE_SEQUENCE(reg);

	ret = i2c_write_read(dev_cfg->i2c.bus, dev_cfg->i2c.addr, &reg, sizeof(reg), &value,
			     sizeof(value));
	if (ret == 0) {
		*val = (value >> 8) & 0xff;
		*val += ((value & 0xff) << 8);
		/* update cache*/
		LOG_DBG("REG:%#02x VAL:%#02x", WM8994_SWAP_UINT16_BYTE_SEQUENCE(reg), *val);
	}
}

static void wm8994_update_reg(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t val)
{
	uint16_t reg_val = 0;
	uint16_t new_value = 0;

	wm8994_read_reg(dev, reg, &reg_val);
	LOG_DBG("read %#x = %x", reg, reg_val);
	new_value = (reg_val & ~mask) | (val & mask);
	LOG_DBG("write %#x = %x", reg, new_value);
	wm8994_write_reg(dev, reg, new_value);
}

static void wm8994_soft_reset(const struct device *dev)
{
	wm8994_write_reg(dev, WM8994_REG_RESET, 0x8994U);
}

static void wm8994_configure_output(const struct device *dev)
{
	wm8994_out_volume_config(dev, AUDIO_CHANNEL_ALL, WM8994_HEADPHONE_DEFAULT_VOLUME_VALUE);
	wm8994_out_mute_config(dev, AUDIO_CHANNEL_ALL, false);

	wm8994_apply_properties(dev);
}

static void wm8994_configure_input(const struct device *dev)
{
	wm8994_route_input(dev, AUDIO_CHANNEL_FRONT_LEFT, kwm8994_InputPGASourceInput1);
	wm8994_route_input(dev, AUDIO_CHANNEL_FRONT_RIGHT, kwm8994_InputPGASourceInput3);

	/* Input MIXER source */
	wm8994_write_reg(dev, wm8994_REG_INPUTMIX,
			 (((kwm8994_InputMixerSourceInputPGA & 7U) << 3U) |
			  (kwm8994_InputMixerSourceInputPGA & 7U)));
	/* Input MIXER enable */
	wm8994_write_reg(dev, wm8994_REG_INPUT_MIXER_1, 3U);

	wm8994_in_volume_config(dev, AUDIO_CHANNEL_ALL, wm8994_LINEIN_DEFAULT_VOLUME_VALUE);
	wm8994_in_mute_config(dev, AUDIO_CHANNEL_ALL, false);
}

#if DEBUG_wm8994_REGISTER
static void wm8994_read_all_reg(const struct device *dev, uint16_t endAddress)
{
	uint16_t readValue = 0U, i = 0U;

	for (i = 0U; i < endAddress; i++) {
		wm8994_read_reg(dev, i, &readValue);
	}
}
#endif

static const struct audio_codec_api wm8994_driver_api = {
	.configure = wm8994_configure,
	.start_output = wm8994_start_output,
	.stop_output = wm8994_stop_output,
	.set_property = wm8994_set_property,
	.apply_properties = wm8994_apply_properties,
	.route_input = wm8994_route_input,
	.route_output = wm8994_route_output
};

#define wm8994_INIT(n)                                                                             \
	static const struct wm8994_driver_config wm8994_device_config_##n = {                      \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.clock_source = DT_INST_ENUM_IDX(n, clock_source),                                 \
		.mclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, mclk)),                   \
		.mclk_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, name)};  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &wm8994_device_config_##n, POST_KERNEL,         \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &wm8994_driver_api);

DT_INST_FOREACH_STATUS_OKAY(wm8994_INIT)
