/* drivers/sharp/shtps/sy3000/shtps_rmi_devctl.c
 *
 * Copyright (c) 2013, Sharp. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

#include <sharp/shtps_dev.h>
#include "shtps_param_extern.h"
#include "shtps_rmi_devctl.h"
#include "shtps_log.h"

/* -----------------------------------------------------------------------------------
 */
#define SHTPS_LVS1_REGULATOR_NAME				"8226_lvs1"
#define SHTPS_LVS1_GPIO_TP_SPI_MOSI				16
#define SHTPS_LVS1_GPIO_TP_SPI_MISO				17
#define SHTPS_LVS1_GPIO_TP_SPI_CS_N				18
#define SHTPS_LVS1_GPIO_TP_SPI_CLK				19

//static struct regulator *shtps_lvs1 = NULL;

SHTPS_PARAM_DEF( SHTPS_LVS1_REGULATOR_RESET_SLEEPTIME, 	100  );

/* -----------------------------------------------------------------------------------
 */
static int msm_shtps_gpio_setup(int irq, int rst)
{
    int rc = 0;

#if 0
    rc = gpio_request(irq, "shtps_irq");
    if (rc) {
        SHTPS_LOG_ERR_PRINT("%s() request gpio failed (irq)\n", __func__);
        return rc;
    }
#endif

    rc = gpio_request(rst, "shtps_rst");
    if (rc) {
        SHTPS_LOG_ERR_PRINT("%s() request gpio failed (rst)\n", __func__);
        return rc;
    }

    return 0;
}

static void msm_shtps_gpio_teardown(int irq, int rst)
{
#if 0
    gpio_free(irq);
#endif
    gpio_free(rst);
}

/* -----------------------------------------------------------------------------------
 */
int shtps_device_setup(int irq, int rst)
{
	return msm_shtps_gpio_setup(irq, rst);
}
EXPORT_SYMBOL(shtps_device_setup);

void shtps_device_teardown(int irq, int rst)
{
	msm_shtps_gpio_teardown(irq, rst);
}
EXPORT_SYMBOL(shtps_device_teardown);

void shtps_device_reset(int rst)
{
	gpio_set_value(rst, 0);
	mb();
	udelay(SHTPS_HWRESET_TIME_US);

	gpio_set_value(rst, 1);
	mdelay(SHTPS_HWRESET_AFTER_TIME_MS);
}
EXPORT_SYMBOL(shtps_device_reset);

void shtps_device_sleep(struct device* dev)
{
	int ret = 0;
	int enabled = 0;
	struct regulator *reg;

	reg = regulator_get(dev, SHTPS_PWR_PMIC_PORT_NAME);
	if (IS_ERR(reg)) {
		SHTPS_LOG_ERR_PRINT("Unable to get %s regulator\n", SHTPS_PWR_PMIC_PORT_NAME);
		return;
	}

	enabled = regulator_is_enabled(reg);

	if (enabled){
		ret = regulator_set_mode(reg, REGULATOR_MODE_IDLE);
		if (ret != 0) {
			SHTPS_LOG_ERR_PRINT("regulator_set_mode fail, ret=%d, mode=%d\n", ret, REGULATOR_MODE_IDLE);
		}
	}else{
		SHTPS_LOG_ERR_PRINT("%s regulator is not enabled\n", SHTPS_PWR_PMIC_PORT_NAME);
	}

	regulator_put(reg);
}
EXPORT_SYMBOL(shtps_device_sleep);

void shtps_device_wakeup(struct device* dev)
{
	int ret = 0;
	int enabled = 0;
	struct regulator *reg;

	reg = regulator_get(dev, SHTPS_PWR_PMIC_PORT_NAME);
	if (IS_ERR(reg)) {
		SHTPS_LOG_ERR_PRINT("Unable to get %s regulator\n", SHTPS_PWR_PMIC_PORT_NAME);
		return;
	}

	enabled = regulator_is_enabled(reg);

	if (enabled){
		ret = regulator_set_mode(reg, REGULATOR_MODE_NORMAL);
		if (ret != 0) {
			SHTPS_LOG_ERR_PRINT("regulator_set_mode fail, ret=%d, mode=%d\n", ret, REGULATOR_MODE_NORMAL);
		}
	}else{
		SHTPS_LOG_ERR_PRINT("%s regulator is not enabled\n", SHTPS_PWR_PMIC_PORT_NAME);
	}

	regulator_put(reg);
}
EXPORT_SYMBOL(shtps_device_wakeup);
void shtps_lvs1_regulator_reset(int rst)
{
#if 0
	int ret;

	if(shtps_lvs1==NULL){
		printk(KERN_ERR "[shtps]regulator reset Err");
		return;
	}

	gpio_set_value(rst, 0);

	gpio_request(SHTPS_LVS1_GPIO_TP_SPI_MOSI, "TP_SPI_MOSI");
	gpio_request(SHTPS_LVS1_GPIO_TP_SPI_MISO, "TP_SPI_MISO");
	gpio_request(SHTPS_LVS1_GPIO_TP_SPI_CS_N, "TP_SPI_CS_N");
	gpio_request(SHTPS_LVS1_GPIO_TP_SPI_CLK, "TP_SPI_CLK");
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_MOSI, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_MISO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_CS_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_CLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);

	ret = regulator_is_enabled(shtps_lvs1);
	if(ret == 0){
		ret = regulator_enable(shtps_lvs1);
			if (ret){
				printk(KERN_ERR "[shtps]regulator enable check Err[%d]",ret);
			}
	}

	ret = regulator_disable(shtps_lvs1);
	if (ret){
		printk(KERN_ERR "[shtps]regulator disable Err[%d]",ret);
	}

	msleep(SHTPS_LVS1_REGULATOR_RESET_SLEEPTIME);

	ret = regulator_enable(shtps_lvs1);
	if (ret){
		printk(KERN_ERR "[shtps]regulator enable Err[%d]",ret);
	}

	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_MOSI, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_MISO, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_CS_N, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHTPS_LVS1_GPIO_TP_SPI_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_free(SHTPS_LVS1_GPIO_TP_SPI_MOSI);
	gpio_free(SHTPS_LVS1_GPIO_TP_SPI_MISO);
	gpio_free(SHTPS_LVS1_GPIO_TP_SPI_CS_N);
	gpio_free(SHTPS_LVS1_GPIO_TP_SPI_CLK);
#endif
}
EXPORT_SYMBOL(shtps_lvs1_regulator_reset);

void shtps_lvs1_regulator_get(struct device *dev)
{
#if 0
	if(shtps_lvs1)
	{
		return ;
	}

	shtps_lvs1 = regulator_get(dev,SHTPS_LVS1_REGULATOR_NAME);

	if (IS_ERR(shtps_lvs1)) {
		printk(KERN_ERR "[shtps]regulator_get Err");
		shtps_lvs1 = NULL;
		return;
	}
#endif
}
EXPORT_SYMBOL(shtps_lvs1_regulator_get);

void shtps_lvs1_regulator_put(void)
{
#if 0
	if(shtps_lvs1==NULL){
		printk(KERN_ERR "[shtps]regulator put Err");
		return;
	}

	regulator_put(shtps_lvs1);
	shtps_lvs1 = NULL;
#endif
}
EXPORT_SYMBOL(shtps_lvs1_regulator_put);

void shtps_lvs1_regulator_init(struct device *dev)
{
#if 0
	int ret;

	if(shtps_lvs1)
	{
		return ;
	}

	shtps_lvs1 = regulator_get(dev,SHTPS_LVS1_REGULATOR_NAME);

	if (IS_ERR(shtps_lvs1)) {
		printk(KERN_ERR "[shtps]regulator_init Err");
		shtps_lvs1 = NULL;
		return;
	}

	ret = regulator_enable(shtps_lvs1);
	if (ret){
		printk(KERN_ERR "[shtps]regulator_enable Err[%d]",ret);
	}

	regulator_put(shtps_lvs1);
	shtps_lvs1 = NULL;

#endif
}
EXPORT_SYMBOL(shtps_lvs1_regulator_init);

void shtps_lvs1_regulator_remove(void)
{
#if 0
	int ret;

	if(shtps_lvs1==NULL){
		return;
	}

	ret = regulator_disable(shtps_lvs1);
	if (ret){
		printk(KERN_ERR "[shtps]regulator_disable Err[%d]",ret);
	}

	regulator_put(shtps_lvs1);
	shtps_lvs1 = NULL;
#endif
}
EXPORT_SYMBOL(shtps_lvs1_regulator_remove);

MODULE_DESCRIPTION("SHARP TOUCHPANEL DRIVER MODULE");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SHARP CORPORATION");
MODULE_VERSION("1.00");
