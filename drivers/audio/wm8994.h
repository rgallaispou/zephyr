/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 * Author: Raphaël Gallais-Pou <raphael.gallais-pou@foss.st.com>
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_WM8994_H_
#define ZEPHYR_DRIVERS_AUDIO_WM8994_H_

#ifdef __cplusplus
extern "C" {
#endif

#define WM8994_SWAP_UINT16_BYTE_SEQUENCE(x) ((((x) & 0x00ffU) << 8U) | (((x) & 0xff00U) >> 8U))
/*! @brief wm8994 max clock */
#define WM8994_MAX_DSP_CLOCK                (24576000U)
#define WM8994_MAX_SYS_CLOCK                (12288000U)
/*! @brief wm8994 f2 better performance range */
#define WM8994_FLL_VCO_MIN_FREQ             90000000U
#define WM8994_FLL_VCO_MAX_FREQ             100000000U
#define WM8994_FLL_LOCK_TIMEOUT             10000000U
/*! @brief WM8994 FLLN range */
#define WM8994_FLL_N_MIN_VALUE              6U
#define WM8994_FLL_N_MAX_VALUE              12U
#define WM8994_FLL_MAX_REFERENCE_CLOCK      (13500000U)

/*! @brief Define the register address of wm8994. */
#define WM8994_REG_RESET        0x0000	/* R0     - Reset (0) */
#define WM8994_REG_POWER_MGMT_1 0x0001	/* R1     - Power Management (1) */
#define WM8994_REG_POWER_MGMT_2 0x0002	/* R2     - Power Management (2) */
#define WM8994_REG_POWER_MGMT_3 0x0003	/* R3     - Power Management (3) */
#define WM8994_REG_POWER_MGMT_4 0x0004	/* R4     - Power Management (4) */
#define WM8994_REG_POWER_MGMT_5 0x0005	/* R5     - Power Management (5) */
#define WM8994_REG_POWER_MGMT_6 0x0006	/* R6     - Power Management (6) */
#define WM8994_REG_INPUT_MIXER_1 0x0015	/* R21    - Input Mixer (1) */
#define WM8994_REG_L_LINEIN_1_2 0x0018	/* R24    - Left Line Input 1&2 Volume */
#define WM8994_REG_L_LINEIN_3_4 0x0019	/* R25    - Left Line Input 3&4 Volume */
#define WM8994_REG_R_LINEIN_1_2 0x001A	/* R26    - Right Line Input 1&2 Volume */
#define WM8994_REG_R_LINEIN_3_4 0x001B	/* R27    - Right Line Input 3&4 Volume */
#define WM8994_REG_L_OUT_VOL 0x001C	/* R28    - Left Output Volume */
#define WM8994_REG_R_OUT_VOL 0x001D	/* R29    - Right Output Volume */
#define WM8994_REG_LINE_OUTS_VOL 0x001E	/* R30    - Line Outputs Volume */
#define WM8994_REG_HPOUT2_VOL 0x001F	/* R31    - HPOUT2 Volume */
#define WM8994_REG_L_OPGA_VOL 0x0020	/* R32    - Left OPGA Volume */
#define WM8994_REG_R_OPGA_VOL 0x0021	/* R33    - Right OPGA Volume */
#define WM8994_REG_SPKMIXL_ATT 0x0022	/* R34    - SPKMIXL Attenuation */
#define WM8994_REG_SPKMIXR_ATT 0x0023	/* R35    - SPKMIXR Attenuation */
#define WM8994_REG_SPKOUT_MIXERS 0x0024	/* R36    - SPKOUT Mixers */
#define WM8994_REG_CLASSD 0x0025	/* R37    - ClassD */
#define WM8994_REG_L_SPK_VOL 0x0026	/* R38    - Speaker Volume Left */
#define WM8994_REG_R_SPK_VOL 0x0027	/* R39    - Speaker Volume Right */
#define WM8994_REG_INPUT_MIXER_2 0x0028	/* R40    - Input Mixer (2) */
#define WM8994_REG_INPUT_MIXER_3 0x0029	/* R41    - Input Mixer (3) */
#define WM8994_REG_INPUT_MIXER_4 0x002A	/* R42    - Input Mixer (4) */
#define WM8994_REG_INPUT_MIXER_5 0x002B	/* R43    - Input Mixer (5) */
#define WM8994_REG_INPUT_MIXER_6 0x002C	/* R44    - Input Mixer (6) */
#define WM8994_REG_OUTPUT_MIXER_1 0x002D	/* R45    - Output Mixer (1) */
#define WM8994_REG_OUTPUT_MIXER_2 0x002E	/* R46    - Output Mixer (2) */
#define WM8994_REG_OUTPUT_MIXER_3 0x002F	/* R47    - Output Mixer (3) */
#define WM8994_REG_OUTPUT_MIXER_4 0x0030	/* R48    - Output Mixer (4) */
#define WM8994_REG_OUTPUT_MIXER_5 0x0031	/* R49    - Output Mixer (5) */
#define WM8994_REG_OUTPUT_MIXER_6 0x0032	/* R50    - Output Mixer (6) */
#define WM8994_REG_HPOUT2_MIXER 0x0033	/* R51    - HPOUT2 Mixer */
#define WM8994_REG_LINE_MIXER_1 0x0034	/* R52    - Line Mixer (1) */
#define WM8994_REG_LINE_MIXER_2 0x0035	/* R53    - Line Mixer (2) */
#define WM8994_REG_SPK_MIXER 0x0036	/* R54    - Speaker Mixer */
#define WM8994_REG_MORE_CONTROL 0x0037	/* R55    - Additional Control */
#define WM8994_REG_ANTIPOP_1 0x0038	/* R56    - AntiPOP (1) */
#define WM8994_REG_ANTIPOP_2 0x0039	/* R57    - AntiPOP (2) */
#define WM8994_REG_MICBIAS 0x003A	/* R58    - MICBIAS */
#define WM8994_REG_LDO_1 0x003B	/* R59    - LDO 1 */
#define WM8994_REG_LDO_2 0x003C	/* R60    - LDO 2 */
#define WM8994_REG_CHARGE_PUMP 0x004C	/* R76    - Charge Pump (1) */
#define WM8994_REG_CLASSW 0x0051	/* R81    - Class W (1) */
#define WM8994_REG_DC_SERVO_1 0x0055	/* R85    - DC Servo (2) */
#define WM8994_REG_DC_SERVO_2 0x0057	/* R87    - DC Servo (4) */
#define WM8994_REG_ANALOG_HP 0x0060	/* R96    - Analogue HP (1) */
#define WM8994_REG_CONTROL_INT 0x0101	/* R257   - Control Interface */
#define WM8994_REG_WSEQ_CTRL_1 0x0110	/* R272   - Write Sequencer Ctrl (1) */
#define WM8994_REG_WSEQ_CTRL_2 0x0111	/* R273   - Write Sequencer Ctrl (2) */
#define WM8994_REG_AIF1_CLK_1 0x0200	/* R512   - AIF1 Clocking (1) */
#define WM8994_REG_AIF1_CLK_2 0x0201	/* R513   - AIF1 Clocking (2) */
#define WM8994_REG_AIF2_CLK_1 0x0204	/* R516   - AIF2 Clocking (1) */
#define WM8994_REG_AIF2_CLK_2 0x0205	/* R517   - AIF2 Clocking (2) */
#define WM8994_REG_CLK_1 0x0208	/* R520   - Clocking (1) */
#define WM8994_REG_CLK_2 0x0209	/* R521   - Clocking (2) */
#define WM8994_REG_AIF1_RATE 0x0210	/* R528   - AIF1 Rate */
#define WM8994_REG_AIF2_RATE 0x0211	/* R529   - AIF2 Rate */
#define WM8994_REG_FLL1_CONTROL_1 0x0220	/* R544   - FLL1 Control (1) */
#define WM8994_REG_FLL1_CONTROL_2 0x0221	/* R545   - FLL1 Control (2) */
#define WM8994_REG_FLL1_CONTROL_3 0x0222	/* R546   - FLL1 Control (3) */
#define WM8994_REG_FLL1_CONTROL_4 0x0223	/* R547   - FLL1 Control (4) */
#define WM8994_REG_FLL1_CONTROL_5 0x0224	/* R548   - FLL1 Control (5) */
#define WM8994_REG_FLL2_CONTROL_1 0x0240	/* R576   - FLL2 Control (1) */
#define WM8994_REG_FLL2_CONTROL_2 0x0241	/* R577   - FLL2 Control (2) */
#define WM8994_REG_FLL2_CONTROL_3 0x0242	/* R578   - FLL2 Control (3) */
#define WM8994_REG_FLL2_CONTROL_4 0x0243	/* R579   - FLL2 Control (4) */
#define WM8994_REG_FLL2_CONTROL_5 0x0244	/* R580   - FLL2 Control (5) */
#define WM8994_REG_AIF1_CONTROL_1 0x0300	/* R768   - AIF1 Control (1) */
#define WM8994_REG_AIF1_CONTROL_2 0x0301	/* R769   - AIF1 Control (2) */
#define WM8994_REG_AIF1_MST_SLV 0x0302	/* R770   - AIF1 Master/Slave */
#define WM8994_REG_AIF1_BCLK 0x0303	/* R771   - AIF1 BCLK */
/* TODO to be filled */
#define WM8994_REG_AIF2_CONTROL_1 0x0310	/* R784   - AIF2 Control (1) */
#define WM8994_REG_AIF2_CONTROL_2 0x0311	/* R785   - AIF2 Control (2) */
#define WM8994_REG_AIF2_MST_SLV 0x0312	/* R786   - AIF2 Master/Slave */
#define WM8994_REG_AIF2_BCLK 0x0313	/* R787   - AIF2 BCLK */
/* TODO to be filled */
#define WM8994_REG_DAC1_L_VOL 0x0610	/* R1552  - DAC1 Left Volume */
#define WM8994_REG_DAC1_R_VOL 0x0611	/* R1553  - DAC1 Right Volume */
#define WM8994_REG_DAC2_L_VOL 0x0612	/* R1554  - DAC2 Left Volume */
#define WM8994_REG_DAC2_R_VOL 0x0613	/* R1555  - DAC2 Right Volume */
#define WM8994_REG_DAC_SOFTMUTE 0x0614	/* R1556  - DAC Softmute */
#define WM8994_REG_OVERSAMPLING 0x0620	/* R1568  - Oversampling */
#define WM8994_REG_SIDETONE 0x0621	/* R1569  - Sidetone */
#define WM8994_REG_GPIO_1 0x0700	/* R1792  - GPIO 1 */
#define WM8994_REG_GPIO_2 0x0701	/* R1793  - GPIO 2 */
#define WM8994_REG_GPIO_3 0x0702	/* R1794  - GPIO 3 */
#define WM8994_REG_GPIO_4 0x0703	/* R1795  - GPIO 4 */
#define WM8994_REG_GPIO_5 0x0704	/* R1796  - GPIO 5 */
#define WM8994_REG_GPIO_6 0x0705	/* R1797  - GPIO 6 */
#define WM8994_REG_GPIO_7 0x0706	/* R1798  - GPIO 7 */
#define WM8994_REG_GPIO_8 0x0707	/* R1799  - GPIO 8 */
#define WM8994_REG_GPIO_9 0x0708	/* R1800  - GPIO 9 */
#define WM8994_REG_GPIO_10 0x0709	/* R1801  - GPIO 10 */
#define WM8994_REG_GPIO_11 0x070A	/* R1802  - GPIO 11 */
#define WM8994_REG_PULL_CONTROL_1 0x0720	/* R1824  - Pull Control (1) */
#define WM8994_REG_PULL_CONTROL_2 0x0721	/* R1825  - Pull Control (2) */
#define WM8994_REG_INT_STATUS_1 0x0738	/* R1848  - Interrupt Status 1 Mask */
#define WM8994_REG_INT_STATUS_1 0x0739	/* R1849  - Interrupt Status 2 Mask */
#define WM8994_REG_INT_CONTROL 0x0740	/* R1856  - Interrupt Control */
#define WM8994_REG_INT_DEBOUNCE 0x0748	/* R1864  - IRQ Debounce */

#define WM8962_L_CH_MUTE_MASK 2U
#define WM8962_R_CH_MUTE_MASK 1U

/*! @brief WM8962 CLOCK2 bits */
#define WM8962_CLOCK2_BCLK_DIV_MASK 0xFU

/*! @brief WM8962_IFACE0 FORMAT bits */
#define WM8962_IFACE0_FORMAT_MASK  0x13U
#define WM8962_IFACE0_FORMAT_SHIFT 0x00U
#define WM8962_IFACE0_FORMAT_RJ    0x00U
#define WM8962_IFACE0_FORMAT_LJ    0x01U
#define WM8962_IFACE0_FORMAT_I2S   0x02U
#define WM8962_IFACE0_FORMAT_DSP   0x03U
#define WM8962_IFACE0_FORMAT(x)    (((x) << WM8962_IFACE1_FORMAT_SHIFT) & WM8962_IFACE1_FORMAT_MASK)

/*! @brief WM8962_IFACE0 WL bits */
#define WM8962_IFACE0_WL_MASK   0x0CU
#define WM8962_IFACE0_WL_SHIFT  0x02U
#define WM8962_IFACE0_WL_16BITS 0x00U
#define WM8962_IFACE0_WL_20BITS 0x01U
#define WM8962_IFACE0_WL_24BITS 0x02U
#define WM8962_IFACE0_WL_32BITS 0x03U
#define WM8962_IFACE0_WL(x)     (((x) << WM8962_IFACE0_WL_SHIFT) & WM8962_IFACE0_WL_MASK)

/*! @brief WM8962_IFACE1 LRP bit */
#define WM8962_IFACE1_LRP_MASK         0x10U
#define WM8962_IFACE1_LRP_SHIFT        0x04U
#define WM8962_IFACE1_LRCLK_NORMAL_POL 0x00U
#define WM8962_IFACE1_LRCLK_INVERT_POL 0x01U
#define WM8962_IFACE1_DSP_MODEA        0x00U
#define WM8962_IFACE1_DSP_MODEB        0x01U
#define WM8962_IFACE1_LRP(x)           (((x) << WM8962_IFACE1_LRP_SHIFT) & WM8962_IFACE1_LRP_MASK)

/*! @brief WM8962_IFACE1 DLRSWAP bit */
#define WM8962_IFACE1_DLRSWAP_MASK  0x20U
#define WM8962_IFACE1_DLRSWAP_SHIFT 0x05U
#define WM8962_IFACE1_DACCH_NORMAL  0x00U
#define WM8962_IFACE1_DACCH_SWAP    0x01U

#define WM8962_IFACE1_DLRSWAP(x) (((x) << WM8962_IFACE1_DLRSWAP_SHIFT) & WM8962_IFACE1_DLRSWAP_MASK)

/*! @brief WM8962_IFACE1 MS bit */
#define WM8962_IFACE1_MS_MASK  0x40U
#define WM8962_IFACE1_MS_SHIFT 0x06U
#define WM8962_IFACE1_SLAVE    0x00U
#define WM8962_IFACE1_MASTER   0x01U
#define WM8962_IFACE1_MS(x)    (((x) << WM8962_IFACE1_MS_SHIFT) & WM8962_IFACE1_MS_MASK)

/*! @brief WM8962_IFACE1 BCLKINV bit */
#define WM8962_IFACE1_BCLKINV_MASK   0x80U
#define WM8962_IFACE1_BCLKINV_SHIFT  0x07U
#define WM8962_IFACE1_BCLK_NONINVERT 0x00U
#define WM8962_IFACE1_BCLK_INVERT    0x01U

#define WM8962_IFACE1_BCLKINV(x) (((x) << WM8962_IFACE1_BCLKINV_SHIFT) & WM8962_IFACE1_BCLKINV_MASK)

/*! @brief WM8962_IFACE1 ALRSWAP bit */
#define WM8962_IFACE1_ALRSWAP_MASK  0x100U
#define WM8962_IFACE1_ALRSWAP_SHIFT 0x08U
#define WM8962_IFACE1_ADCCH_NORMAL  0x00U
#define WM8962_IFACE1_ADCCH_SWAP    0x01U

#define WM8962_IFACE1_ALRSWAP(x) (((x) << WM8962_IFACE1_ALRSWAP_SHIFT) & WM8962_IFACE1_ALRSWAP_MASK)

/*! @brief WM8962_POWER1 */
#define WM8962_POWER1_VREF_MASK  0x40U
#define WM8962_POWER1_VREF_SHIFT 0x06U

#define WM8962_POWER1_AINL_MASK  0x20U
#define WM8962_POWER1_AINL_SHIFT 0x05U

#define WM8962_POWER1_AINR_MASK  0x10U
#define WM8962_POWER1_AINR_SHIFT 0x04U

#define WM8962_POWER1_ADCL_MASK  0x08U
#define WM8962_POWER1_ADCL_SHIFT 0x03U

#define WM8962_POWER1_ADCR_MASK  0x4U
#define WM8962_POWER1_ADCR_SHIFT 0x02U

#define WM8962_POWER1_MICB_MASK  0x02U
#define WM8962_POWER1_MICB_SHIFT 0x01U

#define WM8962_POWER1_DIGENB_MASK  0x01U
#define WM8962_POWER1_DIGENB_SHIFT 0x00U

/*! @brief WM8962_POWER2 */
#define WM8962_POWER2_DACL_MASK  0x100U
#define WM8962_POWER2_DACL_SHIFT 0x08U

#define WM8962_POWER2_DACR_MASK  0x80U
#define WM8962_POWER2_DACR_SHIFT 0x07U

#define WM8962_POWER2_LOUT1_MASK  0x40U
#define WM8962_POWER2_LOUT1_SHIFT 0x06U

#define WM8962_POWER2_ROUT1_MASK  0x20U
#define WM8962_POWER2_ROUT1_SHIFT 0x05U

#define WM8962_POWER2_SPKL_MASK  0x10U
#define WM8962_POWER2_SPKL_SHIFT 0x04U

#define WM8962_POWER2_SPKR_MASK  0x08U
#define WM8962_POWER2_SPKR_SHIFT 0x03U

#define WM8962_POWER3_LMIC_MASK           0x20U
#define WM8962_POWER3_LMIC_SHIFT          0x05U
#define WM8962_POWER3_RMIC_MASK           0x10U
#define WM8962_POWER3_RMIC_SHIFT          0x04U
#define WM8962_POWER3_LOMIX_MASK          0x08U
#define WM8962_POWER3_LOMIX_SHIFT         0x03U
#define WM8962_POWER3_ROMIX_MASK          0x04U
#define WM8962_POWER3_ROMIX_SHIFT         0x02U
/*! @brief WM8962 I2C address. */
#define WM8962_I2C_ADDR                   (0x34 >> 1U)
/*! @brief WM8962 I2C baudrate */
#define WM8962_I2C_BAUDRATE               (100000U)
/*! @brief WM8962 maximum volume value */
#define WM8962_ADC_MAX_VOLUME_VALUE       0xFFU
#define WM8962_DAC_MAX_VOLUME_VALUE       0xFFU
#define WM8962_HEADPHONE_MAX_VOLUME_VALUE 0x7FU
#define WM8962_HEADPHONE_MIN_VOLUME_VALUE 0x2FU
#define WM8962_LINEIN_MAX_VOLUME_VALUE    0x3FU
#define WM8962_SPEAKER_MAX_VOLUME_VALUE   0x7FU
#define WM8962_SPEAKER_MIN_VOLUME_VALUE   0x2FU

#define WM8962_ADC_DEFAULT_VOLUME_VALUE       0x1C0U
#define WM8962_DAC_DEFAULT_VOLUME_VALUE       0x1C0U
#define WM8962_HEADPHONE_DEFAULT_VOLUME_VALUE 0x179U
#define WM8962_LINEIN_DEFAULT_VOLUME_VALUE    0x12DU
#define WM8962_SPEAKER_DEFAULT_VOLUME_VALUE   0x179U

/**
 * WM8994_REG_L_OUT_VOL, WM8994_REG_R_OUT_VOL (headphone outs),
 * WM8994_REG_L_OPGA_VOL, WM8994_REG_R_OPGA_VOL (mixer outs),
 * WM8994_REG_L_SPK_VOL, WM8994_REG_L_SPK_VOL (speaker outs):
 * [8]   - VU: Volume update, works for entire channel pair
 * [7]   - ZC: Zero-crossing enable
 * [6]   - MUTE: Mute mask
 * [5:0] - VOL: 6-bit volume value
 */
#define WM8994_REGVAL_OUT_VOL(vu, zc, mute, vol)                                                  \
	(((vu & 0b1) << 8) | (zc & 0b1) << 7 | (mute & 0b1) << 6 | (vol & 0b000111111))
#define WM8994_REGMASK_OUT_VU   0b100000000
#define WM8994_REGMASK_OUT_ZC   0b010000000
#define WM8994_REGMASK_OUT_MUTE 0b001000000
#define WM8994_REGMASK_OUT_VOL  0b000111111

#define WM8994_OUT_MUTE(x) ((x << 8U) & (1U << 8U))

/**
 * WM8994_REG_LINVOL, WM8994_REG_RINVOL:
 * [8]   - VU: Volume update, works for entire channel pair
 * [7]   - MUTE: Input mute
 * [6]   - ZC: Zero-crossing enable
 * [5:0] - VOL: 6-bit volume value
 */
#define WM8994_REGVAL_IN_VOL(vu, mute, zc, vol)                                                    \
	((vu & 0b1) << 8 | (mute & 0b1) << 7 | (zc & 0b1) << 6 | (vol & 0b000111111))
#define WM8994_REGMASK_IN_VU     0b100000000
#define WM8994_REGMASK_IN_MUTE   0b010000000
#define WM8994_REGMASK_IN_ZC     0b001000000
#define WM8994_REGMASK_IN_VOLUME 0b000111111

/*! @brief wm8994 input mixer source.
 * @anchor wm8994_input_mixer_source_t
 */
typedef enum _wm8994_input_mixer_source {
	kwm8994_InputMixerSourceInput2 = 4U,   /*!< input mixer source input 2 */
	kwm8994_InputMixerSourceInput3 = 2U,   /*!< input mixer source input 3 */
	kwm8994_InputMixerSourceInputPGA = 1U, /*!< input mixer source input PGA */
} wm8994_input_mixer_source_t;

/*! @brief wm8994 output mixer source.
 * @anchor wm8994_output_mixer_source_t
 */
typedef enum _wm8994_output_mixer_source {
	kwm8994_OutputMixerDisabled = 0U,              /*!< output mixer disabled */
	kwm8994_OutputMixerSourceInput4Right = 1U,     /*!< output mixer source input 4 left */
	kwm8994_OutputMixerSourceInput4Left = 2U,      /*!< output mixer source input 4 right */
	kwm8994_OutputMixerSourceRightInputMixer = 4U, /*!< output mixer source left input mixer */
	kwm8994_OutputMixerSourceLeftInputMixer = 8U,  /*!< output mixer source right input mixer*/
	kwm8994_OutputMixerSourceRightDAC = 0x10U,     /*!< output mixer source left DAC */
	kwm8994_OutputMixerSourceLeftDAC = 0x20U,      /*!< output mixer source Right DAC */
} wm8994_output_mixer_source_t;

/*! @brief Modules in wm8994 board. */
typedef enum _wm8994_module {
	kwm8994_ModuleADC = 0,           /*!< ADC module in wm8994 */
	kwm8994_ModuleDAC = 1,           /*!< DAC module in wm8994 */
	kwm8994_ModuleMICB = 4,          /*!< Mic bias */
	kwm8994_ModuleMIC = 5,           /*!< Input Mic */
	kwm8994_ModuleLineIn = 6,        /*!< Analog in PGA  */
	kwm8994_ModuleHeadphone = 7,     /*!< Line out module */
	kwm8994_ModuleSpeaker = 8,       /*!< Speaker module */
	kwm8994_ModuleHeaphoneMixer = 9, /*!< Output mixer */
	kwm8994_ModuleSpeakerMixer = 10, /*!< Output mixer */
} wm8994_module_t;

/*! @brief wm8994 play channel
 * @anchor _wm8994_play_channel
 */
typedef enum _wm8994_play_channel {
	kwm8994_HeadphoneLeft = 1,  /*!< wm8994 headphone left channel */
	kwm8994_HeadphoneRight = 2, /*!< wm8994 headphone right channel */
	kwm8994_SpeakerLeft = 4,    /*!< wm8994 speaker left channel */
	kwm8994_SpeakerRight = 8,   /*!< wm8994 speaker right channel */
} wm8994_play_channel_t;

/*!
 * @brief The audio data transfer protocol choice.
 * wm8994 only supports I2S format and PCM format.
 */
typedef enum _wm8994_protocol {
	kwm8994_BusPCMA = 4,           /*!< PCMA mode */
	kwm8994_BusPCMB = 3,           /*!< PCMB mode */
	kwm8994_BusI2S = 2,            /*!< I2S type */
	kwm8994_BusLeftJustified = 1,  /*!< Left justified mode */
	kwm8994_BusRightJustified = 0, /*!< Right justified mode */
} wm8994_protocol_t;

/*! @brief wm8994 input source */
typedef enum _wm8994_input_pga_source {
	kwm8994_InputPGASourceInput1 = 8, /*!< Input PGA source input1 */
	kwm8994_InputPGASourceInput2 = 4, /*!< Input PGA source input2 */
	kwm8994_InputPGASourceInput3 = 2, /*!< Input PGA source input3 */
	kwm8994_InputPGASourceInput4 = 1, /*!< Input PGA source input4 */
} wm8994_input_pga_source_t;

/*! @brief wm8994 input source */
typedef enum _wm8994_output_pga_source {
	kwm8994_OutputPGASourceMixer = 0, /*!< Output PGA source mixer */
	kwm8994_OutputPGASourceDAC = 1,   /*!< Output PGA source DAC */
} wm8994_output_pga_source_t;

/*! @brief audio sample rate definition
 * @anchor _wm8994_sample_rate
 */
typedef enum _wm8994_sample_rate {
	kwm8994_AudioSampleRate8kHz = 8000U,     /*!< Sample rate 8000 Hz */
	kwm8994_AudioSampleRate11025Hz = 11025U, /*!< Sample rate 11025 Hz */
	kwm8994_AudioSampleRate12kHz = 12000U,   /*!< Sample rate 12000 Hz */
	kwm8994_AudioSampleRate16kHz = 16000U,   /*!< Sample rate 16000 Hz */
	kwm8994_AudioSampleRate22050Hz = 22050U, /*!< Sample rate 22050 Hz */
	kwm8994_AudioSampleRate24kHz = 24000U,   /*!< Sample rate 24000 Hz */
	kwm8994_AudioSampleRate32kHz = 32000U,   /*!< Sample rate 32000 Hz */
	kwm8994_AudioSampleRate44100Hz = 44100U, /*!< Sample rate 44100 Hz */
	kwm8994_AudioSampleRate48kHz = 48000U,   /*!< Sample rate 48000 Hz */
	kwm8994_AudioSampleRate88200Hz = 88200U, /*!< Sample rate 88200 Hz */
	kwm8994_AudioSampleRate96kHz = 96000U,   /*!< Sample rate 96000 Hz */
} wm8994_sample_rate_t;

/*! @brief audio bit width
 * @anchor _wm8994_audio_bit_width
 */
typedef enum _wm8994_audio_bit_width {
	kwm8994_AudioBitWidth16bit = 16U, /*!< audio bit width 16 */
	kwm8994_AudioBitWidth20bit = 20U, /*!< audio bit width 20 */
	kwm8994_AudioBitWidth24bit = 24U, /*!< audio bit width 24 */
	kwm8994_AudioBitWidth32bit = 32U, /*!< audio bit width 32 */
} wm8994_audio_bit_width_t;

/*! @brief wm8994 fll clock source */
typedef enum _wm8994_fllclk_source {
	kwm8994_FLLClkSourceMCLK = 0U, /*!< FLL clock source from MCLK */
	kwm8994_FLLClkSourceBCLK = 1U, /*!< FLL clock source from BCLK */
} wm8994_fllclk_source_t;

/*! @brief wm8994 sysclk source */
typedef enum _wm8994_sysclk_source {
	kwm8994_SysClkSourceMclk = 0U, /*!< sysclk source from external MCLK */
	kwm8994_SysClkSourceFLL = 1U,  /*!< sysclk source from internal FLL */
} wm8994_sysclk_source_t;

/*! @brief wm8994 default sequence */
typedef enum _wm8994_sequence_id {
	kwm8994_SequenceDACToHeadphonePowerUp = 0x80U, /*!< dac to headphone power up sequence */
	kwm8994_SequenceAnalogueInputPowerUp = 0x92U,  /*!< Analogue input power up sequence */
	kwm8994_SequenceChipPowerDown = 0x9BU,         /*!< Chip power down sequence */
	kwm8994_SequenceSpeakerSleep = 0xE4U,          /*!< Speaker sleep sequence */
	kwm8994_SequenceSpeakerWake = 0xE8U,           /*!< speaker wake sequence */
} wm8994_sequence_id_t;

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_AUDIO_WM8994_H_ */
