/* drivers/sharp/shirda/shirda_msm_gpio.c (sharp IrDA driver)
 *
 * Copyright (C) 2011 - 2015 SHARP CORPORATION All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


static struct msm_gpio shirda_gpio_idle[] = {
	{ GPIO_CFG(SHIRDA_GPIO_TXD, 0,                   GPIO_CFG_OUTPUT,
		SHIRDA_TXD_PULL, SHIRDA_TXD_STRENGTH), "IRDA_TX"},
	{ GPIO_CFG(SHIRDA_GPIO_RXD, SHIRDA_GPIO_RX_FUNC, GPIO_CFG_INPUT,
		SHIRDA_RXD_PULL, SHIRDA_RXD_STRENGTH), "IRDA_RX"},
	{ GPIO_CFG(SHIRDA_GPIO_SD,  SHIRDA_GPIO_SD_FUNC, GPIO_CFG_OUTPUT,
		SHIRDA_SD_PULL,  SHIRDA_SD_STRENGTH),  "IRDA_SD"},
};

static struct msm_gpio shirda_gpio_active2[] = {
	{ GPIO_CFG(SHIRDA_GPIO_TXD, SHIRDA_GPIO_TX_FUNC, GPIO_CFG_OUTPUT,
		SHIRDA_TXD_PULL, SHIRDA_TXD_STRENGTH), "IRDA_TX"},
	{ GPIO_CFG(SHIRDA_GPIO_RXD, 0,                   GPIO_CFG_INPUT,
		SHIRDA_RXD_PULL, SHIRDA_RXD_STRENGTH), "IRDA_RX"},
	{ GPIO_CFG(SHIRDA_GPIO_SD,  SHIRDA_GPIO_SD_FUNC, GPIO_CFG_OUTPUT,
		SHIRDA_SD_PULL,  SHIRDA_SD_STRENGTH),  "IRDA_SD"},
};


static struct msm_gpio shirda_gpio_active[] = {
	{ GPIO_CFG(SHIRDA_GPIO_TXD, SHIRDA_GPIO_TX_FUNC, GPIO_CFG_OUTPUT,
		SHIRDA_TXD_PULL, SHIRDA_TXD_STRENGTH), "IRDA_TX"},
	{ GPIO_CFG(SHIRDA_GPIO_RXD, SHIRDA_GPIO_RX_FUNC, GPIO_CFG_INPUT,
		SHIRDA_RXD_PULL, SHIRDA_RXD_STRENGTH), "IRDA_RX"},
	{ GPIO_CFG(SHIRDA_GPIO_SD,  SHIRDA_GPIO_SD_FUNC, GPIO_CFG_OUTPUT,
		SHIRDA_SD_PULL,  SHIRDA_SD_STRENGTH),  "IRDA_SD"},
};

static int shirda_gpio_idle_init[] = {
	0,
	0,
	SHIRDA_SD_SHUTDOWN,
};


static int shirda_msm_gpios_enable(const struct msm_gpio *table, int size);
static int shirda_msm_gpios_request(const struct msm_gpio *table, int size);
static void shirda_msm_gpios_disable_free(const struct msm_gpio *table,
								int size);
static int shirda_msm_gpios_disable(const struct msm_gpio *table, int size);
static void shirda_msm_gpios_free(const struct msm_gpio *table, int size);
/*linux/arch/arm/mach-msm/gpio.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

static int shirda_msm_gpios_enable(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		rc = gpio_tlmm_config(g->gpio_cfg, GPIO_CFG_ENABLE);
		if (rc) {
			IRDALOG_ERROR("gpio_tlmm_config(%d) <%s> failed: %d\n",
			       GPIO_PIN(g->gpio_cfg), g->label ?: "?", rc);
			shirda_msm_gpios_disable(table, i);
			return rc;
		}
	}
	return 0;
}

static int shirda_msm_gpios_request(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		if ((GPIO_FUNC(g->gpio_cfg)) == 0) {
			rc = gpio_request(GPIO_PIN(g->gpio_cfg), g->label);
			if (rc) {
				IRDALOG_ERROR(
					"gpio_request(%d) <%s> failed: %d\n",
					GPIO_PIN(g->gpio_cfg),
					g->label ?: "?", rc);
				shirda_msm_gpios_free(table, i);
				return rc;
			}
		}
	}
	return 0;
}

static void shirda_msm_gpios_disable_free(const struct msm_gpio *table,
								int size)
{
	shirda_msm_gpios_disable(table, size);
	shirda_msm_gpios_free(table, size);
}

static int shirda_msm_gpios_disable(const struct msm_gpio *table, int size)
{
	int rc = 0;
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		int tmp;
		g = table + i;
		tmp = gpio_tlmm_config(g->gpio_cfg, GPIO_CFG_DISABLE);
		if (tmp) {
			IRDALOG_ERROR(
				"gpio_tlmm_config(0x%08x, GPIO_CFG_DISABLE)"
				" <%s> failed: %d\n",
				g->gpio_cfg, g->label ?: "?", rc);
			if (!rc)
				rc = tmp;
		}
	}
	return rc;
}

static void shirda_msm_gpios_free(const struct msm_gpio *table, int size)
{
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		g = table + i;
		gpio_free(GPIO_PIN(g->gpio_cfg));
	}
}

static int shirda_gpios_direction(const struct msm_gpio *table,
						const int *pol, int size);

static int shirda_gpios_direction(const struct msm_gpio *table,
						const int *pol, int size)
{
	int rc = 0;
	int i;
	const struct msm_gpio *g;
	const int *p;

	for (i = 0; i < size; i++) {
		g = table + i;
		p = pol + i;
		if ((GPIO_FUNC(g->gpio_cfg)) == 0) {

			if (GPIO_DIR(g->gpio_cfg) == GPIO_CFG_INPUT) {
				rc = gpio_direction_input(
						GPIO_PIN(g->gpio_cfg));
			} else {
				rc = gpio_direction_output(
						GPIO_PIN(g->gpio_cfg), *p);
			}
			if (rc != 0) {
				IRDALOG_ERROR(
					"Set gpio dir error(0x%08x): %s\n",
						rc, g->label ?: "?");
			}
		}
	}
	return rc;
}

static int shirda_gpio_init(void)
{
	int ret = 0;


	ret = shirda_msm_gpios_request(shirda_gpio_idle,
					ARRAY_SIZE(shirda_gpio_idle));
	if (ret != 0) {
		IRDALOG_FATAL("gpios_request() fail errno = %d\n", ret);
		return ret;
	}

	ret = shirda_gpios_direction(shirda_gpio_idle, shirda_gpio_idle_init,
					ARRAY_SIZE(shirda_gpio_idle));
	if (ret != 0) {
		IRDALOG_FATAL("gpios direction set fail errno = %d\n", ret );
		shirda_msm_gpios_free(shirda_gpio_active,
					ARRAY_SIZE(shirda_gpio_active));
		return ret;
	}

	ret = shirda_msm_gpios_enable(shirda_gpio_idle,
					ARRAY_SIZE(shirda_gpio_idle));
	if (ret != 0) {
		IRDALOG_FATAL("gpios_enable() fail errno = %d\n", ret);
		shirda_msm_gpios_free(shirda_gpio_active,
					ARRAY_SIZE(shirda_gpio_active));
		return ret;
	}


	return 0;
}

static void shirda_gpio_free(void)
{
	shirda_msm_gpios_disable_free(shirda_gpio_idle,
						ARRAY_SIZE(shirda_gpio_idle));
}

static int shirda_gpios_enable(shirda_gpio_mode mode)
{
	switch (mode) {
	case SHIRDA_GPIO_MODE_IDLE:
		return shirda_msm_gpios_enable(shirda_gpio_idle,
					ARRAY_SIZE(shirda_gpio_idle));
		break;
	case SHIRDA_GPIO_MODE_ACTIVE:
		return shirda_msm_gpios_enable(shirda_gpio_active,
					ARRAY_SIZE(shirda_gpio_active));
		break;
	case SHIRDA_GPIO_MODE_ACTIVE2:
		return shirda_msm_gpios_enable(shirda_gpio_active2,
					ARRAY_SIZE(shirda_gpio_active2));
		break;
	default:
		IRDALOG_FATAL("Invalid mode %d\n", mode);
		return -EIO;
	}
}
