/*
 * max98927.c  --  MAX98927 ALSA Soc Audio driver
 *
 * Copyright (C) 2016 Maxim Integrated Products
 * Author: Ryan Lee <ryans.lee@maximintegrated.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <sound/tlv.h>
#include "max98927.h"

static struct reg_default max98927_reg[] = {
	{MAX98927_R0001_INT_RAW1,  0x00},
	{MAX98927_R0002_INT_RAW2,  0x00},
	{MAX98927_R0003_INT_RAW3,  0x00},
	{MAX98927_R0004_INT_STATE1,  0x00},
	{MAX98927_R0005_INT_STATE2,  0x00},
	{MAX98927_R0006_INT_STATE3,  0x00},
	{MAX98927_R0007_INT_FLAG1,  0x00},
	{MAX98927_R0008_INT_FLAG2,  0x00},
	{MAX98927_R0009_INT_FLAG3,  0x00},
	{MAX98927_R000A_INT_EN1,  0x00},
	{MAX98927_R000B_INT_EN2,  0x00},
	{MAX98927_R000C_INT_EN3,  0x00},
	{MAX98927_R000D_INT_FLAG_CLR1,  0x00},
	{MAX98927_R000E_INT_FLAG_CLR2,  0x00},
	{MAX98927_R000F_INT_FLAG_CLR3,  0x00},
	{MAX98927_R0010_IRQ_CTRL,  0x00},
	{MAX98927_R0011_CLK_MON,  0x00},
	{MAX98927_R0012_WDOG_CTRL,  0x00},
	{MAX98927_R0013_WDOG_RST,  0x00},
	{MAX98927_R0014_MEAS_ADC_THERM_WARN_THRESH,  0x00},
	{MAX98927_R0015_MEAS_ADC_THERM_SHDN_THRESH,  0x00},
	{MAX98927_R0016_MEAS_ADC_THERM_HYSTERESIS,  0x00},
	{MAX98927_R0017_PIN_CFG,  0x55},
	{MAX98927_R0018_PCM_RX_EN_A,  0x00},
	{MAX98927_R0019_PCM_RX_EN_B,  0x00},
	{MAX98927_R001A_PCM_TX_EN_A,  0x00},
	{MAX98927_R001B_PCM_TX_EN_B,  0x00},
	{MAX98927_R001C_PCM_TX_HIZ_CTRL_A,  0x00},
	{MAX98927_R001D_PCM_TX_HIZ_CTRL_B,  0x00},
	{MAX98927_R001E_PCM_TX_CH_SRC_A,  0x00},
	{MAX98927_R001F_PCM_TX_CH_SRC_B,  0x00},
	{MAX98927_R0020_PCM_MODE_CFG,  0x40},
	{MAX98927_R0021_PCM_MASTER_MODE,  0x00},
	{MAX98927_R0022_PCM_CLK_SETUP,  0x22},
	{MAX98927_R0023_PCM_SR_SETUP1,  0x00},
	{MAX98927_R0024_PCM_SR_SETUP2,  0x00},
	{MAX98927_R0025_PCM_TO_SPK_MONOMIX_A,  0x00},
	{MAX98927_R0026_PCM_TO_SPK_MONOMIX_B,  0x00},
	{MAX98927_R0027_ICC_RX_EN_A,  0x00},
	{MAX98927_R0028_ICC_RX_EN_B,  0x00},
	{MAX98927_R002B_ICC_TX_EN_A,  0x00},
	{MAX98927_R002C_ICC_TX_EN_B,  0x00},
	{MAX98927_R002E_ICC_HIZ_MANUAL_MODE,  0x00},
	{MAX98927_R002F_ICC_TX_HIZ_EN_A,  0x00},
	{MAX98927_R0030_ICC_TX_HIZ_EN_B,  0x00},
	{MAX98927_R0031_ICC_LNK_EN,  0x00},
	{MAX98927_R0032_PDM_TX_EN,  0x00},
	{MAX98927_R0033_PDM_TX_HIZ_CTRL,  0x00},
	{MAX98927_R0034_PDM_TX_CTRL,  0x00},
	{MAX98927_R0035_PDM_RX_CTRL,  0x00},
	{MAX98927_R0036_AMP_VOL_CTRL,  0x00},
	{MAX98927_R0037_AMP_DSP_CFG,  0x02},
	{MAX98927_R0038_TONE_GEN_DC_CFG,  0x00},
	{MAX98927_R0039_DRE_CTRL,  0x01},
	{MAX98927_R003A_AMP_EN,  0x00},
	{MAX98927_R003B_SPK_SRC_SEL,  0x00},
	{MAX98927_R003C_SPK_GAIN,  0x00},
	{MAX98927_R003D_SSM_CFG,  0x01},
	{MAX98927_R003E_MEAS_EN,  0x00},
	{MAX98927_R003F_MEAS_DSP_CFG,  0x04},
	{MAX98927_R0040_BOOST_CTRL0,  0x00},
	{MAX98927_R0041_BOOST_CTRL3,  0x00},
	{MAX98927_R0042_BOOST_CTRL1,  0x00},
	{MAX98927_R0043_MEAS_ADC_CFG,  0x00},
	{MAX98927_R0044_MEAS_ADC_BASE_MSB,  0x00},
	{MAX98927_R0045_MEAS_ADC_BASE_LSB,  0x00},
	{MAX98927_R0046_ADC_CH0_DIVIDE,  0x00},
	{MAX98927_R0047_ADC_CH1_DIVIDE,  0x00},
	{MAX98927_R0048_ADC_CH2_DIVIDE,  0x00},
	{MAX98927_R0049_ADC_CH0_FILT_CFG,  0x00},
	{MAX98927_R004A_ADC_CH1_FILT_CFG,  0x00},
	{MAX98927_R004B_ADC_CH2_FILT_CFG,  0x00},
	{MAX98927_R004C_MEAS_ADC_CH0_READ,  0x00},
	{MAX98927_R004D_MEAS_ADC_CH1_READ,  0x00},
	{MAX98927_R004E_MEAS_ADC_CH2_READ,  0x00},
	{MAX98927_R0051_BROWNOUT_STATUS,  0x00},
	{MAX98927_R0052_BROWNOUT_EN,  0x00},
	{MAX98927_R0053_BROWNOUT_INFINITE_HOLD,  0x00},
	{MAX98927_R0054_BROWNOUT_INFINITE_HOLD_CLR,  0x00},
	{MAX98927_R0055_BROWNOUT_LVL_HOLD,  0x00},
	{MAX98927_R005A_BROWNOUT_LVL1_THRESH,  0x00},
	{MAX98927_R005B_BROWNOUT_LVL2_THRESH,  0x00},
	{MAX98927_R005C_BROWNOUT_LVL3_THRESH,  0x00},
	{MAX98927_R005D_BROWNOUT_LVL4_THRESH,  0x00},
	{MAX98927_R005E_BROWNOUT_THRESH_HYSTERYSIS,  0x00},
	{MAX98927_R005F_BROWNOUT_AMP_LIMITER_ATK_REL,  0x00},
	{MAX98927_R0060_BROWNOUT_AMP_GAIN_ATK_REL,  0x00},
	{MAX98927_R0061_BROWNOUT_AMP1_CLIP_MODE,  0x00},
	{MAX98927_R0072_BROWNOUT_LVL1_CUR_LIMIT,  0x00},
	{MAX98927_R0073_BROWNOUT_LVL1_AMP1_CTRL1,  0x00},
	{MAX98927_R0074_BROWNOUT_LVL1_AMP1_CTRL2,  0x00},
	{MAX98927_R0075_BROWNOUT_LVL1_AMP1_CTRL3,  0x00},
	{MAX98927_R0076_BROWNOUT_LVL2_CUR_LIMIT,  0x00},
	{MAX98927_R0077_BROWNOUT_LVL2_AMP1_CTRL1,  0x00},
	{MAX98927_R0078_BROWNOUT_LVL2_AMP1_CTRL2,  0x00},
	{MAX98927_R0079_BROWNOUT_LVL2_AMP1_CTRL3,  0x00},
	{MAX98927_R007A_BROWNOUT_LVL3_CUR_LIMIT,  0x00},
	{MAX98927_R007B_BROWNOUT_LVL3_AMP1_CTRL1,  0x00},
	{MAX98927_R007C_BROWNOUT_LVL3_AMP1_CTRL2,  0x00},
	{MAX98927_R007D_BROWNOUT_LVL3_AMP1_CTRL3,  0x00},
	{MAX98927_R007E_BROWNOUT_LVL4_CUR_LIMIT,  0x00},
	{MAX98927_R007F_BROWNOUT_LVL4_AMP1_CTRL1,  0x00},
	{MAX98927_R0080_BROWNOUT_LVL4_AMP1_CTRL2,  0x00},
	{MAX98927_R0081_BROWNOUT_LVL4_AMP1_CTRL3,  0x00},
	{MAX98927_R0082_ENV_TRACK_VOUT_HEADROOM,  0x00},
	{MAX98927_R0083_ENV_TRACK_BOOST_VOUT_DELAY,  0x00},
	{MAX98927_R0084_ENV_TRACK_REL_RATE,  0x00},
	{MAX98927_R0085_ENV_TRACK_HOLD_RATE,  0x00},
	{MAX98927_R0086_ENV_TRACK_CTRL,  0x00},
	{MAX98927_R0087_ENV_TRACK_BOOST_VOUT_READ,  0x00},
	{MAX98927_R00FF_GLOBAL_SHDN,  0x00},
	{MAX98927_R0100_SOFT_RESET,  0x00},
	{MAX98927_R01FF_REV_ID,  0x40},
};

static int max98927_reg_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, unsigned int reg,
	unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	int data;

	regmap_read(max98927->regmap, reg, &data);
	ucontrol->value.integer.value[0] = (data & mask) >> shift;
	return 0;
}

static int max98927_reg_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, unsigned int reg,
	unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	regmap_update_bits(max98927->regmap, reg, mask, sel << shift);
	dev_dbg(codec->dev, "%s: register 0x%02X, value 0x%02X\n",
		__func__, reg, sel);
	return 0;
}

static int max98927_dai_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	unsigned int mode = 0;
	unsigned int invert = 0;

	dev_dbg(codec->dev, "%s: fmt 0x%08X\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		mode = MAX98927_PCM_MASTER_MODE_SLAVE;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		max98927->master = true;
		mode = MAX98927_PCM_MASTER_MODE_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		mode = MAX98927_PCM_MASTER_MODE_HYBRID;
	default:
		dev_err(codec->dev, "DAI clock mode unsupported");
		return -EINVAL;
	}

	regmap_update_bits(max98927->regmap,
		MAX98927_R0021_PCM_MASTER_MODE,
		MAX98927_PCM_MASTER_MODE_MASK,
		mode);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98927_PCM_MODE_CFG_PCM_BCLKEDGE;
		break;
	default:
		dev_err(codec->dev, "DAI invert mode unsupported");
		return -EINVAL;
	}

	regmap_update_bits(max98927->regmap,
		MAX98927_R0020_PCM_MODE_CFG,
		MAX98927_PCM_MODE_CFG_PCM_BCLKEDGE,
		invert);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		max98927->iface |= SND_SOC_DAIFMT_I2S;

	break;
	case SND_SOC_DAIFMT_LEFT_J:
		max98927->iface |= SND_SOC_DAIFMT_LEFT_J;
	break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(max98927->regmap,
		MAX98927_R0020_PCM_MODE_CFG,
		max98927->iface, max98927->iface);

	/* pcm channel configuration */
	if (max98927->iface & (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_LEFT_J))
		regmap_write(max98927->regmap,
			MAX98927_R0018_PCM_RX_EN_A,
			MAX98927_PCM_RX_CH0_EN|
			MAX98927_PCM_RX_CH1_EN);

	return 0;
}

/* codec MCLK rate in master mode */
static const int rate_table[] = {
	5644800, 6000000, 6144000, 6500000,
	9600000, 11289600, 12000000, 12288000,
	13000000, 19200000,
};

static int max98927_set_clock(struct max98927_priv *max98927,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_codec *codec = max98927->codec;
	/* BCLK/LRCLK ratio calculation */
	int blr_clk_ratio = params_channels(params) * max98927->ch_size;
	int reg = MAX98927_R0022_PCM_CLK_SETUP;
	int mask = MAX98927_PCM_CLK_SETUP_BSEL_MASK;
	int value;

	if (max98927->master) {
		int i;
		/* match rate to closest value */
		for (i = 0; i < ARRAY_SIZE(rate_table); i++) {
			if (rate_table[i] >= max98927->sysclk)
				break;
		}
		if (i == ARRAY_SIZE(rate_table)) {
			dev_err(codec->dev, "failed to find proper clock rate.\n");
			return -EINVAL;
		}
		regmap_update_bits(max98927->regmap,
			MAX98927_R0021_PCM_MASTER_MODE,
			MAX98927_PCM_MASTER_MODE_MCLK_MASK,
			i << MAX98927_PCM_MASTER_MODE_MCLK_RATE_SHIFT);
	}

	switch (blr_clk_ratio) {
	case 32:
		value = 2;
		break;
	case 48:
		value = 3;
		break;
	case 64:
		value = 4;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(max98927->regmap, reg, mask, value);
	return 0;
}

static int max98927_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	unsigned int sampling_rate = 0;
	unsigned int chan_sz = 0;

	/* pcm mode configuration */
	switch (snd_pcm_format_width(params_format(params))) {
	case 16:
		chan_sz = MAX98927_PCM_MODE_CFG_CHANSZ_16;
		max98927->ch_size = 16;
		break;
	case 24:
		chan_sz = MAX98927_PCM_MODE_CFG_CHANSZ_24;
		max98927->ch_size = 24;
		break;
	case 32:
		chan_sz = MAX98927_PCM_MODE_CFG_CHANSZ_32;
		max98927->ch_size = 32;
		break;
	default:
		dev_err(codec->dev, "format unsupported %d",
			params_format(params));
		goto err;
	}

	regmap_update_bits(max98927->regmap,
		MAX98927_R0020_PCM_MODE_CFG,
		MAX98927_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	dev_dbg(codec->dev, "format supported %d",
		params_format(params));

	/* sampling rate configuration */
	switch (params_rate(params)) {
	case 8000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_8000;
		break;
	case 11025:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_11025;
		break;
	case 12000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_12000;
		break;
	case 16000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_16000;
		break;
	case 22050:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_22050;
		break;
	case 24000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_24000;
		break;
	case 32000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_32000;
		break;
	case 44100:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_44100;
		break;
	case 48000:
		sampling_rate |= MAX98927_PCM_SR_SET1_SR_48000;
		break;
	default:
		dev_err(codec->dev, "rate %d not supported\n",
			params_rate(params));
		goto err;
	}
	/* set DAI_SR to correct LRCLK frequency */
	regmap_update_bits(max98927->regmap,
		MAX98927_R0023_PCM_SR_SETUP1,
		MAX98927_PCM_SR_SET1_SR_MASK,
		sampling_rate);
	regmap_update_bits(max98927->regmap,
		MAX98927_R0024_PCM_SR_SETUP2,
		MAX98927_PCM_SR_SET2_SR_MASK,
		sampling_rate << MAX98927_PCM_SR_SET2_SR_SHIFT);

	/* set sampling rate of IV */
	if (max98927->interleave_mode &&
		sampling_rate > MAX98927_PCM_SR_SET1_SR_16000)
		regmap_update_bits(max98927->regmap,
			MAX98927_R0024_PCM_SR_SETUP2,
			MAX98927_PCM_SR_SET2_IVADC_SR_MASK,
			sampling_rate - 3);
	else
		regmap_update_bits(max98927->regmap,
			MAX98927_R0024_PCM_SR_SETUP2,
			MAX98927_PCM_SR_SET2_IVADC_SR_MASK,
			sampling_rate);
	return max98927_set_clock(max98927, params);
err:
	return -EINVAL;
}

#define MAX98927_RATES SNDRV_PCM_RATE_8000_48000

#define MAX98927_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static int max98927_dai_set_sysclk(struct snd_soc_dai *dai,
	int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);

	max98927->sysclk = freq;
	return 0;
}

static const struct snd_soc_dai_ops max98927_dai_ops = {
	.set_sysclk = max98927_dai_set_sysclk,
	.set_fmt = max98927_dai_set_fmt,
	.hw_params = max98927_dai_hw_params,
};

static int max98927_dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(max98927->regmap,
			MAX98927_R003A_AMP_EN,
			MAX98927_AMP_EN_MASK, 1);
		/* enable VMON and IMON */
		regmap_update_bits(max98927->regmap,
			MAX98927_R003E_MEAS_EN,
			MAX98927_MEAS_V_EN | MAX98927_MEAS_I_EN,
			MAX98927_MEAS_V_EN | MAX98927_MEAS_I_EN);
		regmap_update_bits(max98927->regmap,
			MAX98927_R00FF_GLOBAL_SHDN,
			MAX98927_GLOBAL_EN_MASK, 1);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(max98927->regmap,
			MAX98927_R00FF_GLOBAL_SHDN,
			MAX98927_GLOBAL_EN_MASK, 0);
		regmap_update_bits(max98927->regmap,
			MAX98927_R003A_AMP_EN,
			MAX98927_AMP_EN_MASK, 0);
		/* disable VMON and IMON */
		regmap_update_bits(max98927->regmap,
			MAX98927_R003E_MEAS_EN,
			MAX98927_MEAS_V_EN | MAX98927_MEAS_I_EN, 0);
		break;
	default:
		return 0;
	}
	return 0;
}

static const char * const max98927_switch_text[] = {
	"Left", "Right", "LeftRight"};

static const struct soc_enum dai_sel_enum =
	SOC_ENUM_SINGLE(MAX98927_R0025_PCM_TO_SPK_MONOMIX_A,
		MAX98927_PCM_TO_SPK_MONOMIX_CFG_SHIFT,
		3, max98927_switch_text);

static const struct snd_kcontrol_new max98927_dai_controls =
	SOC_DAPM_ENUM("DAI Sel", dai_sel_enum);

static const struct snd_soc_dapm_widget max98927_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("DAI_OUT", "HiFi Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC_E("Amp Enable", "HiFi Playback", MAX98927_R003A_AMP_EN,
		0, 0, max98927_dac_event,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX("DAI Sel Mux", SND_SOC_NOPM, 0, 0,
		&max98927_dai_controls),
	SND_SOC_DAPM_OUTPUT("BE_OUT"),
};

static DECLARE_TLV_DB_SCALE(max98927_spk_tlv, 300, 300, 0);
static DECLARE_TLV_DB_SCALE(max98927_digital_tlv, -1600, 25, 0);

static int max98927_spk_gain_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = max98927->spk_gain;
	dev_dbg(codec->dev, "%s: spk_gain setting returned %d\n", __func__,
		(int) ucontrol->value.integer.value[0]);
	return 0;
}

static int max98927_spk_gain_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	/* 0x7 is reserved */
	if (sel > 6)
		return -EINVAL;

	regmap_update_bits(max98927->regmap,
		MAX98927_R003C_SPK_GAIN,
		MAX98927_SPK_PCM_GAIN_MASK, sel);
	max98927->spk_gain = sel;
	return 0;
}

static int max98927_digital_gain_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = max98927->digital_gain;
	dev_dbg(codec->dev, "%s: digital_gain setting returned %d\n", __func__,
		(int) ucontrol->value.integer.value[0]);
	return 0;
}

static int max98927_digital_gain_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	if (sel < ((1 << MAX98927_AMP_VOL_WIDTH) - 1)) {
		regmap_update_bits(max98927->regmap,
			MAX98927_R0036_AMP_VOL_CTRL,
			MAX98927_AMP_VOL_MASK, sel);
		max98927->digital_gain = sel;
	}
	return 0;
}

static int max98927_boost_voltage_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0040_BOOST_CTRL0,
		MAX98927_BOOST_CTRL0_VOUT_MASK, 0);
}

static int max98927_boost_voltage_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0040_BOOST_CTRL0,
		MAX98927_BOOST_CTRL0_VOUT_MASK, 0);
}

static int max98927_amp_vol_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0036_AMP_VOL_CTRL,
		MAX98927_AMP_VOL_SEL,
		MAX98927_AMP_VOL_SHIFT);
}

static int max98927_amp_vol_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0036_AMP_VOL_CTRL,
		MAX98927_AMP_VOL_SEL,
		MAX98927_AMP_VOL_SHIFT);
}

static int max98927_amp_dsp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0052_BROWNOUT_EN,
		MAX98927_BROWNOUT_DSP_EN,
		MAX98927_BROWNOUT_DSP_SHIFT);
}

static int max98927_amp_dsp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0052_BROWNOUT_EN,
		MAX98927_BROWNOUT_DSP_EN,
		MAX98927_BROWNOUT_DSP_SHIFT);
}

static int max98927_ramp_switch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0037_AMP_DSP_CFG,
		MAX98927_AMP_DSP_CFG_RMP_BYPASS,
		MAX98927_AMP_DSP_CFG_RMP_SHIFT);
}
static int max98927_ramp_switch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0037_AMP_DSP_CFG,
		MAX98927_AMP_DSP_CFG_RMP_BYPASS,
		MAX98927_AMP_DSP_CFG_RMP_SHIFT);
}

static int max98927_dre_en_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0039_DRE_CTRL,
		MAX98927_DRE_CTRL_DRE_EN, 0);
}
static int max98927_dre_en_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0039_DRE_CTRL,
		MAX98927_DRE_CTRL_DRE_EN, 0);
}
static int max98927_spk_src_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R003B_SPK_SRC_SEL,
		MAX98927_SPK_SRC_MASK, 0);
}

static int max98927_spk_src_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R003B_SPK_SRC_SEL,
		MAX98927_SPK_SRC_MASK, 0);
}

static int max98927_mono_out_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_get(kcontrol, ucontrol,
		MAX98927_R0025_PCM_TO_SPK_MONOMIX_A,
		MAX98927_PCM_TO_SPK_MONOMIX_CFG_MASK,
		MAX98927_PCM_TO_SPK_MONOMIX_CFG_SHIFT);
}

static int max98927_mono_out_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return max98927_reg_put(kcontrol, ucontrol,
		MAX98927_R0025_PCM_TO_SPK_MONOMIX_A,
		MAX98927_PCM_TO_SPK_MONOMIX_CFG_MASK,
		MAX98927_PCM_TO_SPK_MONOMIX_CFG_SHIFT);
}

static bool max98927_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98927_R0001_INT_RAW1 ... MAX98927_R0028_ICC_RX_EN_B:
	case MAX98927_R002B_ICC_TX_EN_A ... MAX98927_R002C_ICC_TX_EN_B:
	case MAX98927_R002E_ICC_HIZ_MANUAL_MODE
		... MAX98927_R004E_MEAS_ADC_CH2_READ:
	case MAX98927_R0051_BROWNOUT_STATUS
		... MAX98927_R0055_BROWNOUT_LVL_HOLD:
	case MAX98927_R005A_BROWNOUT_LVL1_THRESH
		... MAX98927_R0061_BROWNOUT_AMP1_CLIP_MODE:
	case MAX98927_R0072_BROWNOUT_LVL1_CUR_LIMIT
		... MAX98927_R0087_ENV_TRACK_BOOST_VOUT_READ:
	case MAX98927_R00FF_GLOBAL_SHDN:
	case MAX98927_R0100_SOFT_RESET:
	case MAX98927_R01FF_REV_ID:
		return true;
	}
	return false;
};

static bool max98927_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98927_R0001_INT_RAW1 ... MAX98927_R0009_INT_FLAG3:
		return true;
	default:
		return false;
	}
}

static const char * const max98927_boost_voltage_text[] = {
	"6.5V", "6.625V", "6.75V", "6.875V", "7V", "7.125V", "7.25V", "7.375V",
	"7.5V", "7.625V", "7.75V", "7.875V", "8V", "8.125V", "8.25V", "8.375V",
	"8.5V", "8.625V", "8.75V", "8.875V", "9V", "9.125V", "9.25V", "9.375V",
	"9.5V", "9.625V", "9.75V", "9.875V", "10V"
};

static const char * const max98927_speaker_source_text[] = {
	"i2s", "reserved", "tone", "pdm"
};

static const char * const max98927_monomix_output_text[] = {
	"ch_0", "ch_1", "ch_1_2_div"
};

static const struct soc_enum max98927_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max98927_monomix_output_text),
		max98927_monomix_output_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max98927_speaker_source_text),
		max98927_speaker_source_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max98927_boost_voltage_text),
		max98927_boost_voltage_text),
};

static const struct snd_kcontrol_new max98927_snd_controls[] = {
	SOC_SINGLE_EXT_TLV("Speaker Volume",
		MAX98927_R003C_SPK_GAIN,
		0, 6, 0,
		max98927_spk_gain_get, max98927_spk_gain_put,
		max98927_spk_tlv),
	SOC_SINGLE_EXT_TLV("Digital Gain",
		MAX98927_R0036_AMP_VOL_CTRL,
		0, (1<<MAX98927_AMP_VOL_WIDTH)-1, 0,
		max98927_digital_gain_get, max98927_digital_gain_put,
		max98927_digital_tlv),
	SOC_SINGLE_EXT("Amp DSP Enable",
		MAX98927_R0052_BROWNOUT_EN,
		MAX98927_BROWNOUT_DSP_SHIFT, 1, 0,
		max98927_amp_dsp_get, max98927_amp_dsp_put),
	SOC_SINGLE_EXT("Ramp Switch",
		MAX98927_R0037_AMP_DSP_CFG,
		MAX98927_AMP_DSP_CFG_RMP_SHIFT, 1, 1,
		max98927_ramp_switch_get, max98927_ramp_switch_put),
	SOC_SINGLE_EXT("DRE EN",
		MAX98927_R0039_DRE_CTRL,
		MAX98927_DRE_EN_SHIFT, 1, 0,
		max98927_dre_en_get, max98927_dre_en_put),
	SOC_SINGLE_EXT("Amp Volume Location",
		MAX98927_R0036_AMP_VOL_CTRL,
		MAX98927_AMP_VOL_SEL_SHIFT, 1, 0,
		max98927_amp_vol_get, max98927_amp_vol_put),
	SOC_ENUM_EXT("Boost Output Voltage", max98927_enum[2],
		max98927_boost_voltage_get, max98927_boost_voltage_put),
	SOC_ENUM_EXT("Speaker Source", max98927_enum[1],
		max98927_spk_src_get, max98927_spk_src_put),
	SOC_ENUM_EXT("Monomix Output", max98927_enum[0],
		max98927_mono_out_get, max98927_mono_out_put),
};

static const struct snd_soc_dapm_route max98927_audio_map[] = {
	{"Amp Enable", NULL, "DAI_OUT"},
	{"DAI Sel Mux", "Left", "Amp Enable"},
	{"DAI Sel Mux", "Right", "Amp Enable"},
	{"DAI Sel Mux", "LeftRight", "Amp Enable"},
	{"BE_OUT", NULL, "DAI Sel Mux"},
};

static struct snd_soc_dai_driver max98927_dai[] = {
	{
		.name = "max98927-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98927_RATES,
			.formats = MAX98927_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98927_RATES,
			.formats = MAX98927_FORMATS,
		},
		.ops = &max98927_dai_ops,
	}
};

static int max98927_probe(struct snd_soc_codec *codec)
{
	struct max98927_priv *max98927 = snd_soc_codec_get_drvdata(codec);
	int ret = 0, reg = 0;

	max98927->codec = codec;
	codec->control_data = max98927->regmap;
	codec->cache_bypass = 1;

	/* Software Reset */
	regmap_write(max98927->regmap,
		MAX98927_R0100_SOFT_RESET, MAX98927_SOFT_RESET);

	/* Check Revision ID */
	ret = regmap_read(max98927->regmap,
		MAX98927_R01FF_REV_ID, &reg);
	if (ret < 0)
		dev_err(codec->dev,
			"Failed to read: 0x%02X\n", MAX98927_R01FF_REV_ID);
	else
		dev_info(codec->dev,
			"MAX98927 revisionID: 0x%02X\n", reg);

	/* IV default slot configuration */
	regmap_write(max98927->regmap,
		MAX98927_R001C_PCM_TX_HIZ_CTRL_A,
		0xFF);
	regmap_write(max98927->regmap,
		MAX98927_R001D_PCM_TX_HIZ_CTRL_B,
		0xFF);
	regmap_write(max98927->regmap,
		MAX98927_R0025_PCM_TO_SPK_MONOMIX_A,
		0x80);
	regmap_write(max98927->regmap,
		MAX98927_R0026_PCM_TO_SPK_MONOMIX_B,
		0x1);
	/* Set inital volume (+13dB) */
	regmap_write(max98927->regmap,
		MAX98927_R0036_AMP_VOL_CTRL,
		0x38);
	regmap_write(max98927->regmap,
		MAX98927_R003C_SPK_GAIN,
		0x05);
	/* Enable DC blocker */
	regmap_write(max98927->regmap,
		MAX98927_R0037_AMP_DSP_CFG,
		0x03);
	/* Enable IMON VMON DC blocker */
	regmap_write(max98927->regmap,
		MAX98927_R003F_MEAS_DSP_CFG,
		0xF7);
	/* Boost Output Voltage & Current limit */
	regmap_write(max98927->regmap,
		MAX98927_R0040_BOOST_CTRL0,
		0x1C);
	regmap_write(max98927->regmap,
		MAX98927_R0042_BOOST_CTRL1,
		0x3E);
	/* Measurement ADC config */
	regmap_write(max98927->regmap,
		MAX98927_R0043_MEAS_ADC_CFG,
		0x04);
	regmap_write(max98927->regmap,
		MAX98927_R0044_MEAS_ADC_BASE_MSB,
		0x00);
	regmap_write(max98927->regmap,
		MAX98927_R0045_MEAS_ADC_BASE_LSB,
		0x24);
	/* Brownout Level */
	regmap_write(max98927->regmap,
		MAX98927_R007F_BROWNOUT_LVL4_AMP1_CTRL1,
		0x06);
	/* Envelope Tracking configuration */
	regmap_write(max98927->regmap,
		MAX98927_R0082_ENV_TRACK_VOUT_HEADROOM,
		0x08);
	regmap_write(max98927->regmap,
		MAX98927_R0086_ENV_TRACK_CTRL,
		0x01);
	regmap_write(max98927->regmap,
		MAX98927_R0087_ENV_TRACK_BOOST_VOUT_READ,
		0x10);

	/* voltage, current slot configuration */
	regmap_write(max98927->regmap,
		MAX98927_R001E_PCM_TX_CH_SRC_A,
		(max98927->i_l_slot<<MAX98927_PCM_TX_CH_SRC_A_I_SHIFT|
		max98927->v_l_slot)&0xFF);

	if (max98927->v_l_slot < 8) {
		regmap_update_bits(max98927->regmap,
			MAX98927_R001C_PCM_TX_HIZ_CTRL_A,
			1 << max98927->v_l_slot, 0);
		regmap_update_bits(max98927->regmap,
			MAX98927_R001A_PCM_TX_EN_A,
			1 << max98927->v_l_slot,
			1 << max98927->v_l_slot);
	} else {
		regmap_update_bits(max98927->regmap,
			MAX98927_R001D_PCM_TX_HIZ_CTRL_B,
			1 << (max98927->v_l_slot - 8), 0);
		regmap_update_bits(max98927->regmap,
			MAX98927_R001B_PCM_TX_EN_B,
			1 << (max98927->v_l_slot - 8),
			1 << (max98927->v_l_slot - 8));
	}

	if (max98927->i_l_slot < 8) {
		regmap_update_bits(max98927->regmap,
			MAX98927_R001C_PCM_TX_HIZ_CTRL_A,
			1 << max98927->i_l_slot, 0);
		regmap_update_bits(max98927->regmap,
			MAX98927_R001A_PCM_TX_EN_A,
			1 << max98927->i_l_slot,
			1 << max98927->i_l_slot);
	} else {
		regmap_update_bits(max98927->regmap,
			MAX98927_R001D_PCM_TX_HIZ_CTRL_B,
			1 << (max98927->i_l_slot - 8), 0);
		regmap_update_bits(max98927->regmap,
			MAX98927_R001B_PCM_TX_EN_B,
			1 << (max98927->i_l_slot - 8),
			1 << (max98927->i_l_slot - 8));
	}

	/* Set interleave mode */
	if (max98927->interleave_mode)
		regmap_update_bits(max98927->regmap,
			MAX98927_R001F_PCM_TX_CH_SRC_B,
			MAX98927_PCM_TX_CH_INTERLEAVE_MASK,
			MAX98927_PCM_TX_CH_INTERLEAVE_MASK);
	return ret;
}

static const struct snd_soc_codec_driver soc_codec_dev_max98927 = {
	.probe = max98927_probe,
	.component_driver = {
		.controls = max98927_snd_controls,
		.num_controls = ARRAY_SIZE(max98927_snd_controls),
		.dapm_widgets = max98927_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(max98927_dapm_widgets),
		.dapm_routes = max98927_audio_map,
		.num_dapm_routes = ARRAY_SIZE(max98927_audio_map),
	},
};

static const struct regmap_config max98927_regmap = {
	.reg_bits         = 16,
	.val_bits         = 8,
	.max_register     = MAX98927_R01FF_REV_ID,
	.reg_defaults     = max98927_reg,
	.num_reg_defaults = ARRAY_SIZE(max98927_reg),
	.readable_reg	  = max98927_readable_register,
	.volatile_reg	  = max98927_volatile_reg,
	.cache_type       = REGCACHE_RBTREE,
};

static void max98927_slot_config(struct i2c_client *i2c,
	struct max98927_priv *max98927)
{
	int value;

	if (!of_property_read_u32(i2c->dev.of_node,
		"maxim,vmon-slot-no", &value))
		max98927->v_l_slot = value & 0xF;
	else
		max98927->v_l_slot = 0;
	if (!of_property_read_u32(i2c->dev.of_node,
		"maxim,imon-slot-no", &value))
		max98927->i_l_slot = value & 0xF;
	else
		max98927->i_l_slot = 1;
}

static int max98927_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{

	int ret = 0, value;
	struct max98927_priv *max98927 = NULL;

	max98927 = devm_kzalloc(&i2c->dev,
		sizeof(*max98927), GFP_KERNEL);

	if (!max98927) {
		ret = -ENOMEM;
		goto err;
	}
	i2c_set_clientdata(i2c, max98927);

	/* update interleave mode info */
	if (!of_property_read_u32(i2c->dev.of_node,
		"maxim,interleave_mode", &value)) {
		if (value > 0)
			max98927->interleave_mode = 1;
		else
			max98927->interleave_mode = 0;
	} else
		max98927->interleave_mode = 0;

	/* regmap initialization */
	max98927->regmap
		= devm_regmap_init_i2c(i2c, &max98927_regmap);
	if (IS_ERR(max98927->regmap)) {
		ret = PTR_ERR(max98927->regmap);
		dev_err(&i2c->dev,
			"Failed to allocate regmap: %d\n", ret);
		goto err;
	}

	/* voltage/current slot configuration */
	max98927_slot_config(i2c, max98927);

	/* codec registeration */
	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_max98927,
		max98927_dai, ARRAY_SIZE(max98927_dai));
	if (ret < 0)
		dev_err(&i2c->dev, "Failed to register codec: %d\n", ret);

	return ret;

err:
	if (max98927)
		devm_kfree(&i2c->dev, max98927);
	return ret;
}

static int max98927_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id max98927_i2c_id[] = {
	{ "max98927", 0},
	{ },
};

MODULE_DEVICE_TABLE(i2c, max98927_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id max98927_of_match[] = {
	{ .compatible = "maxim,max98927", },
	{ }
};
MODULE_DEVICE_TABLE(of, max98927_of_match);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id max98927_acpi_match[] = {
	{ "MX98927", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, max98927_acpi_match);
#endif

static struct i2c_driver max98927_i2c_driver = {
	.driver = {
		.name = "max98927",
		.of_match_table = of_match_ptr(max98927_of_match),
		.acpi_match_table = ACPI_PTR(max98927_acpi_match),
		.pm = NULL,
	},
	.probe  = max98927_i2c_probe,
	.remove = max98927_i2c_remove,
	.id_table = max98927_i2c_id,
};

module_i2c_driver(max98927_i2c_driver)

MODULE_DESCRIPTION("ALSA SoC MAX98927 driver");
MODULE_AUTHOR("Ryan Lee <ryans.lee@maximintegrated.com>");
MODULE_LICENSE("GPL");