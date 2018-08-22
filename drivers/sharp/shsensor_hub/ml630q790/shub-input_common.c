/* drivers/sharp/shsensor_hub/ml630q790/shub-input_common.c
 *
 * Copyright (C) 2014 Sharp Corporation
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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/miscdevice.h>

#include "shub_io.h"
#include "ml630q790.h"

/* SHMDS_HUB_0110_01 add S */
#ifdef SHUB_SW_PINCTRL
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#else
#include <asm/gpio.h>
#endif
/* SHMDS_HUB_0110_01 add E */

//-------------------------------------------------------------------
// #define
//-------------------------------------------------------------------
//#define CONFIG_COM_INPUT_DEBUG
#ifdef CONFIG_COM_INPUT_DEBUG
#define COM_DBG(msg, ...) {                    \
        printk("[shub] " msg, ##__VA_ARGS__);  \
}
#else
#define COM_DBG(msg, ...)
#endif

// [COM1] Magnetic Filed sensor
#define MAG_MAX         12000
#define MAG_MIN        -12000
// [COM1] Magnetic Field sensor Uncalibrated
#define MAGUNC_MAX      12000
#define MAGUNC_MIN     -12000

// [COM2] Gyroscope
#define GYRO_MAX        20000
#define GYRO_MIN       -20000
// [COM2] Gyroscope Uncalibrated
#define GYROUNC_MAX     20000
#define GYROUNC_MIN    -20000

// [COM3] Gravity
#define GRAV_MAX        8192
#define GRAV_MIN       -8192
// [COM3] Linear Acceleration
#define LINEACC_MAX    ( 8192 + 1024)
#define LINEACC_MIN    (-8192 - 1024)

// [COM4] Game Rotation Vector
#define GAMEROT_MAX     10000
#define GAMEROT_MIN    -10000
// [COM4] Geomegnetic Rotation Vector
#define MAGROT_MAX      10000
#define MAGROT_MIN     -10000

// [COM5] Significant Motion
// [COM5] Step Counter
// [COM5] Step Detector
#define PEDOM_MAX       1000000
#define PEDOM_MIN       0

/* SHMDS_HUB_0110_01 add S */
#define SHUB_GPIO_RESET_NAME "shub_reset"
#define SHUB_GPIO_REMP_NAME  "shub_remap"
#define SHUB_GPIO_INT0_NAME  "shub_hostif_int0"
#define SHUB_GPIO_INT1_NAME  "shub_hostif_int1"

#ifndef SHUB_SW_PINCTRL
//#define SHUB_GPIO_RST     qpnp_pin_map("pm8941-gpio", 9) /* SHMDS_HUB_0104_03 mod */
//#define SHUB_GPIO_REMP    (80)                           /* SHMDS_HUB_0104_04 mod */
//#define SHUB_GPIO_INT0    (66)                           /* SHMDS_HUB_0104_02 mod */
#define SHUB_GPIO_RST     (117)
#define SHUB_GPIO_REMP    (118)
#define SHUB_GPIO_INT0    (63)
#define SHUB_GPIO_INT1    (64)
#endif
/* SHMDS_HUB_0110_01 add S */

//-------------------------------------------------------------------
// enum
//-------------------------------------------------------------------
enum{
    SHUB_COM_INPUT1,            // [COM1]
    SHUB_COM_INPUT2,            // [COM2]
    SHUB_COM_INPUT3,            // [COM3]
    SHUB_COM_INPUT4,            // [COM4]
    SHUB_COM_INPUT5,            // [COM5]
    SHUB_COM_MAXNUM             // max number 
};

static struct input_dev *shub_idev[SHUB_COM_MAXNUM] = {NULL,NULL,NULL,NULL,NULL};
static unsigned long shub_state[SHUB_COM_MAXNUM] = {0,0,0,0,0};
static int shub_create_info[SHUB_COM_MAXNUM] = {0,0,0,0,0};

static char shub_dname_com1[] = "shub_mag";
static char shub_dname_com2[] = "shub_gyro";
static char shub_dname_com3[] = "shub_linearacc";
static char shub_dname_com4[] = "shub_gamerot";
static char shub_dname_com5[] = "shub_pedo";

static char shub_dphys_com1[] = "shub_mag/input0";
static char shub_dphys_com2[] = "shub_gyro/input0";
static char shub_dphys_com3[] = "shub_linearacc/input0";
static char shub_dphys_com4[] = "shub_gamerot/input0";
static char shub_dphys_com5[] = "shub_pedo/input0";

static char *shub_com_name[] = { shub_dname_com1,
                                 shub_dname_com2,
                                 shub_dname_com3,
                                 shub_dname_com4,
                                 shub_dname_com5
                               };

static char *shub_com_phys[] = { shub_dphys_com1,
                                 shub_dphys_com2,
                                 shub_dphys_com3,
                                 shub_dphys_com4,
                                 shub_dphys_com5
                               };

static int shub_packet_size[SHUB_COM_MAXNUM] ={ 
                                 (( LOGGING_RAM_SIZE / ( DATA_SIZE_MAG       + 1)) * 2),
                                 (( LOGGING_RAM_SIZE / ( DATA_SIZE_GYRO      + 1)) * 2),
                                 (( LOGGING_RAM_SIZE / ( DATA_SIZE_LINEARACC + 1)) * 2),
                                 (( LOGGING_RAM_SIZE / ( DATA_SIZE_GAMERV    + 1)) * 2),
                                 (( LOGGING_RAM_SIZE / ( DATA_SIZE_PEDOCNT   + 1)) * 2)
                               };

/* SHMDS_HUB_0110_01 add S */
#ifdef SHUB_SW_PINCTRL
static struct spi_device *client_mcu;
static unsigned int shub_gpio_no_reset = 0;
static unsigned int shub_gpio_no_brmp = 0;
static unsigned int shub_gpio_no_int0 = 0;
static unsigned int shub_gpio_no_int1 = 0;
#endif
/* SHMDS_HUB_0110_01 add E */

static int shub_com_get_input_no(int inp_type)
{
    int inp_no = SHUB_COM_MAXNUM;
    
    switch(inp_type){
    case SHUB_INPUT_MAG:         // [COM1] Magnetic Filed sensor
    case SHUB_INPUT_MAGUNC:      // [COM1] Magnetic Field sensor Uncalibrated
        inp_no = SHUB_COM_INPUT1;
        break;
    case SHUB_INPUT_GYRO:        // [COM2] Gyroscope
    case SHUB_INPUT_GYROUNC:     // [COM2] Gyroscope Uncalibrated
        inp_no = SHUB_COM_INPUT2;
        break;
    case SHUB_INPUT_GRAVITY:     // [COM3] Gravity
    case SHUB_INPUT_LINEARACC:   // [COM3] Linear Acceleration
        inp_no = SHUB_COM_INPUT3;
        break;
    case SHUB_INPUT_GAMEROT:     // [COM4] Game Rotation Vector
    case SHUB_INPUT_GEORV:       // [COM4] Geomegnetic Rotation Vector
        inp_no = SHUB_COM_INPUT4;
        break;
    case SHUB_INPUT_SIGNIFICANT: // [COM5] Significant Motion
    case SHUB_INPUT_PEDO:        // [COM5] Step Counter
    case SHUB_INPUT_PEDODET:     // [COM5] Step Detector
        inp_no = SHUB_COM_INPUT5;
        break;
    case SHUB_INPUT_ACC:         // [----] Accelerometer
    case SHUB_INPUT_ORI:         // [----] Orientation
    case SHUB_INPUT_RVECT:       // [----] Rotation Vector
    default:
        break;
    }
    return inp_no;
}

static void shub_com_set_abs_params(int no)
{
    switch(no) {
    case SHUB_COM_INPUT1:
        // Magnetic Filed sensor
        input_set_abs_params(shub_idev[no], ABS_MISC, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_VOLUME, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RX, MAG_MIN, MAG_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RY, MAG_MIN, MAG_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RZ, MAG_MIN, MAG_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0X, 0, 0xFF, 0, 0);
        // Magnetic Field sensor Uncalibrated
        input_set_abs_params(shub_idev[no], ABS_HAT3Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT3X, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT1X, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT1Y, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT2X, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_X, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Y, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Z, MAGUNC_MIN, MAGUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0Y, 0, 0xFF, 0, 0);
        shub_set_param_first(shub_idev[no]); /* SHMDS_HUB_0308_01 add */
        break;
        
    case SHUB_COM_INPUT2:
        // Gyroscope
        input_set_abs_params(shub_idev[no], ABS_MISC, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_VOLUME, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RX, GYRO_MIN, GYRO_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RY, GYRO_MIN, GYRO_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RZ, GYRO_MIN, GYRO_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0X, 0, 0xFF, 0, 0); // SHMDS_HUB_0601_03 add
        // Gyroscope Uncalibrated
        input_set_abs_params(shub_idev[no], ABS_HAT3Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT3X, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT1X, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT1Y, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT2X, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0Y, 0, 0xFF, 0, 0); // SHMDS_HUB_0601_03 add
        input_set_abs_params(shub_idev[no], ABS_X, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Y, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Z, GYROUNC_MIN, GYROUNC_MAX, 0, 0);
        shub_set_param_first(shub_idev[no]); /* SHMDS_HUB_0308_01 add */
        break;
        
    case SHUB_COM_INPUT3:
        // Gravity
        input_set_abs_params(shub_idev[no], ABS_MISC, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_VOLUME, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_X, GRAV_MIN, GRAV_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Y, GRAV_MIN, GRAV_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Z, GRAV_MIN, GRAV_MAX, 0, 0);
        // Linear Acceleration
        input_set_abs_params(shub_idev[no], ABS_HAT3Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT3X, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RX, LINEACC_MIN, LINEACC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RY, LINEACC_MIN, LINEACC_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RZ, LINEACC_MIN, LINEACC_MAX, 0, 0);
        shub_set_param_first(shub_idev[no]); /* SHMDS_HUB_0308_01 add */
        break;
        
    case SHUB_COM_INPUT4:
        // Game Rotation Vector
        input_set_abs_params(shub_idev[no], ABS_HAT3Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT3X, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_X, GAMEROT_MIN, GAMEROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Y, GAMEROT_MIN, GAMEROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Z, GAMEROT_MIN, GAMEROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT1Y, GAMEROT_MIN, GAMEROT_MAX, 0, 0);
        // Geomegnetic Rotation Vector
        input_set_abs_params(shub_idev[no], ABS_MISC, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_VOLUME, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RX, MAGROT_MIN, MAGROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RY, MAGROT_MIN, MAGROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_RZ, MAGROT_MIN, MAGROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0Y, MAGROT_MIN, MAGROT_MAX, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT0X, 0, 0xFF, 0, 0);
        shub_set_param_first(shub_idev[no]); /* SHMDS_HUB_0308_01 add */
        break;
        
    case SHUB_COM_INPUT5:
        // Significant Motion
        input_set_abs_params(shub_idev[no], ABS_HAT3Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT3X, 0, 0xFFFFFFFF, 0, 0); // SHMDS_HUB_0601_04 add
        input_set_abs_params(shub_idev[no], ABS_Y, 0, 1, 0, 0);
        // Step Counter
        input_set_abs_params(shub_idev[no], ABS_MISC, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_VOLUME, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_X, PEDOM_MIN, PEDOM_MAX, 0, 0);
        // Step Detector
        input_set_abs_params(shub_idev[no], ABS_HAT2Y, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_HAT2X, 0, 0xFFFFFFFF, 0, 0);
        input_set_abs_params(shub_idev[no], ABS_Z, PEDOM_MIN, PEDOM_MAX, 0, 0);
        shub_set_param_first(shub_idev[no]); /* SHMDS_HUB_0308_01 add */
        break;
        
    default:
        printk(KERN_ERR "[shub] %s : Not Support Sensor type( No=%d )\n", __func__, no);
        break;
    }
}

static int32_t shub_com_create(int inp_type, struct device *dev, int com_no)
{
    int32_t ret = 0;
    
    if (shub_state[com_no] == 0) {
        
        shub_idev[com_no] = input_allocate_device();
        if (!shub_idev[com_no]) {
            printk(KERN_ERR "[shub] %s : allocate Error!!( type=%d, No=%d )\n", __func__, inp_type, com_no);
            return -ENOMEM;
        }
        shub_idev[com_no]->name       = shub_com_name[com_no];
        shub_idev[com_no]->phys       = shub_com_phys[com_no];
        shub_idev[com_no]->id.bustype = BUS_HOST;
        shub_idev[com_no]->dev.parent = dev;
        shub_idev[com_no]->evbit[0]   = BIT_MASK(EV_ABS);
        
        shub_com_set_abs_params(com_no);
        input_set_events_per_packet(shub_idev[com_no], shub_packet_size[com_no]);
        ret = input_register_device(shub_idev[com_no]);
        if (ret) {
            printk(KERN_ERR "[shub] %s : input_register_device Error!!( type=%d, No=%d )\n", __func__, inp_type, com_no);
            input_free_device(shub_idev[com_no]);
            return 1;
        }
        shub_create_info[com_no] = inp_type;
        COM_DBG("%s : register_device( type=%d, No=%d )\n", __func__, inp_type, com_no);
    }
    shub_state[com_no] |= (1<<inp_type);
    COM_DBG("input Enable state = 0x%04lx,0x%04lx,0x%04lx,0x%04lx,0x%04lx\n",
            shub_state[0],shub_state[1],shub_state[2],shub_state[3],shub_state[4] );
    return ret;
}

struct input_dev *shub_com_allocate_device(int inp_type, struct device *dev)
{
    int32_t ret = 0;
    int inp_no;
    struct input_dev *inp_dev = NULL;
    
    inp_no = shub_com_get_input_no(inp_type);
    if (inp_no >= SHUB_COM_MAXNUM) {
        printk(KERN_ERR "[shub] %s : Not Support Sensor type Error!!( type=%d )\n", __func__, inp_type);
        return inp_dev;
    }
    COM_DBG("%s : No=%d, type=%d\n", __func__, inp_no, inp_type);
    
    ret = shub_com_create(inp_type, dev, inp_no);
    if (ret == 0) {
        inp_dev = shub_idev[inp_no];
    }
    return inp_dev;
}

void shub_com_unregister_device(int inp_type)
{
    int inp_no;
    COM_DBG("%s : type=%d\n", __func__, inp_type);
    
    inp_no = shub_com_get_input_no(inp_type);
    if (inp_no >= SHUB_COM_MAXNUM) {
        printk(KERN_ERR "[shub] %s : Not Support Sensor type Error!!( type=%d )\n", __func__, inp_type);
        return;
    }
    
    shub_state[inp_no] &= ~(1<<inp_type);
    COM_DBG("input Disable state = 0x%04lx,0x%04lx,0x%04lx,0x%04lx,0x%04lx\n",
            shub_state[0],shub_state[1],shub_state[2],shub_state[3],shub_state[4] );
    
    if (inp_type == shub_create_info[inp_no]) {
        if (shub_state[inp_no] != 0) {
            printk(KERN_ERR "[shub] %s : state Error!!( type=%d, state=0x%04lx )\n", __func__, inp_type, shub_state[inp_no]);
        }
    }
    
    if (shub_state[inp_no] == 0) {
        input_unregister_device(shub_idev[inp_no]);
        input_free_device(shub_idev[inp_no]);
    }
    return;
}

/* SHMDS_HUB_0110_01 add S */
int shub_set_gpio_no( struct spi_device *client )
{
#ifdef SHUB_SW_PINCTRL
    struct device_node *np = client->dev.of_node;
    
    client_mcu = client;
    shub_gpio_no_reset = of_get_named_gpio(np, "qcom,shub-gpio-reset", 0);
    shub_gpio_no_brmp  = of_get_named_gpio(np, "qcom,shub-gpio-brmp",  0);
    shub_gpio_no_int0  = of_get_named_gpio(np, "qcom,shub-gpio-int0",  0);
    shub_gpio_no_int1  = of_get_named_gpio(np, "qcom,shub-gpio-int1",  0);
    printk("[shub]shub_set_gpio_no: %ld, %ld, %ld, %ld\n", (long)shub_gpio_no_reset, (long)shub_gpio_no_brmp, (long)shub_gpio_no_int0, (long)shub_gpio_no_int1);
#endif
    return 0;
}

int shub_get_gpio_no(int gpio)
{
    int gpio_no = -1;
#ifdef SHUB_SW_PINCTRL
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        gpio_no = shub_gpio_no_reset;
        break;
    case SHUB_GPIO_PIN_BRMP:
        gpio_no = shub_gpio_no_brmp;
        break;
    case SHUB_GPIO_PIN_INT0:
        gpio_no = shub_gpio_no_int0;
        break;
    case SHUB_GPIO_PIN_INT1:
        gpio_no = shub_gpio_no_int1;
        break;
    default:
        break;
    }
#else
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        gpio_no = SHUB_GPIO_RST;
        break;
    case SHUB_GPIO_PIN_BRMP:
        gpio_no = SHUB_GPIO_REMP;
        break;
    case SHUB_GPIO_PIN_INT0:
        gpio_no = SHUB_GPIO_INT0;
        break;
    case SHUB_GPIO_PIN_INT1:
        gpio_no = SHUB_GPIO_INT1;
        break;
    default:
        break;
    }
#endif
    return gpio_no;
}

int shub_gpio_request(int gpio)
{
    int ret = 0;  // SHMDS_HUB_0110_03 mod
#ifdef SHUB_SW_PINCTRL
    struct pinctrl_state *pin_state = NULL;
    struct pinctrl *pin;
    int gpio_no;  // SHMDS_HUB_0110_03 add
    
    gpio_no = shub_get_gpio_no(gpio);  // SHMDS_HUB_0110_03 add
    pin = devm_pinctrl_get(&client_mcu->dev);
    
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        ret = gpio_request(gpio_no, SHUB_GPIO_RESET_NAME); /* SHMDS_HUB_0110_03 add */
        pin_state = pinctrl_lookup_state(pin, "shub_reset_active");
        break;
    case SHUB_GPIO_PIN_BRMP:
        ret = gpio_request(gpio_no, SHUB_GPIO_REMP_NAME);  /* SHMDS_HUB_0110_03 add */
        pin_state = pinctrl_lookup_state(pin, "shub_brmp_active");
        break;
    case SHUB_GPIO_PIN_INT0:
        ret = gpio_request(gpio_no, SHUB_GPIO_INT0_NAME);  /* SHMDS_HUB_0110_05 add */
        pin_state = pinctrl_lookup_state(pin, "shub_int0_active");
        break;
    case SHUB_GPIO_PIN_INT1:
        ret = gpio_request(gpio_no, SHUB_GPIO_INT1_NAME);  /* SHMDS_HUB_0110_05 add */
        pin_state = pinctrl_lookup_state(pin, "shub_int1_active");
        break;
    default:
        break;
    }
    
    // SHMDS_HUB_0110_03 add S
    if (ret < 0){
        printk("[shub]shub_gpio_request: gpio_request error(gpio %d, ret=%d)", gpio, ret);
        return -1;
    }
    // SHMDS_HUB_0110_03 add E
    
    if(pin_state == NULL){
        printk("[shub]shub_gpio_request: Null error(gpio %d)", gpio);
        return -1;
    }
    
    ret = pinctrl_select_state(pin, pin_state);
    if(ret){
        printk("[shub]shub_gpio_request: error(gpio %d, ret=%d)", gpio, ret);
        return ret;
    }
#else
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        ret = gpio_request(SHUB_GPIO_RST, SHUB_GPIO_RESET_NAME);
        break;
    case SHUB_GPIO_PIN_BRMP:
        ret = gpio_request(SHUB_GPIO_REMP, SHUB_GPIO_REMP_NAME);
        break;
    case SHUB_GPIO_PIN_INT0:
        ret = gpio_request(SHUB_GPIO_INT0, SHUB_GPIO_INT0_NAME);
        break;
    case SHUB_GPIO_PIN_INT1:
        ret = gpio_request(SHUB_GPIO_INT1, SHUB_GPIO_INT1_NAME);
        break;
    default:
        break;
    }
#endif
    return 0;
}

int shub_gpio_free(int gpio)
{
#ifdef SHUB_SW_PINCTRL
    struct pinctrl_state *pin_state = NULL;
    struct pinctrl *pin;
    int ret;
    int gpio_no;  // SHMDS_HUB_0110_03 add
    
    gpio_no = shub_get_gpio_no(gpio);   // SHMDS_HUB_0110_03 add
    pin = devm_pinctrl_get(&client_mcu->dev);
    
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        pin_state = pinctrl_lookup_state(pin, "shub_reset_suspend");
        gpio_free(gpio_no);  /* SHMDS_HUB_0110_03 add */
        break;
    case SHUB_GPIO_PIN_BRMP:
        pin_state = pinctrl_lookup_state(pin, "shub_brmp_suspend");
        gpio_free(gpio_no);  /* SHMDS_HUB_0110_03 add */
        break;
    case SHUB_GPIO_PIN_INT0:
        pin_state = pinctrl_lookup_state(pin, "shub_int0_suspend");
        gpio_free(gpio_no);  /* SHMDS_HUB_0110_05 add */
        break;
    case SHUB_GPIO_PIN_INT1:
        pin_state = pinctrl_lookup_state(pin, "shub_int1_suspend");
        gpio_free(gpio_no);  /* SHMDS_HUB_0110_05 add */
        break;
    default:
        break;
    }
    
    if(pin_state == NULL){
        printk("[shub]shub_gpio_free: Null error(gpio %d)", gpio);
        return -1;
    }
    
    ret = pinctrl_select_state(pin, pin_state);
    if(ret){
        printk("[shub]shub_gpio_free: error(gpio %d, ret=%d)", gpio, ret);
        return ret;
    }
#else
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        gpio_free(SHUB_GPIO_RST);
        break;
    case SHUB_GPIO_PIN_BRMP:
        gpio_free(SHUB_GPIO_REMP);
        break;
    case SHUB_GPIO_PIN_INT0:
        gpio_free(SHUB_GPIO_INT0);
        break;
    case SHUB_GPIO_PIN_INT1:
        gpio_free(SHUB_GPIO_INT1);
        break;
    default:
        break;
    }
#endif
    return 0;
}

int shub_gpio_direction_output(int gpio, int data)
{
    int gpio_no;
    
    gpio_no = shub_get_gpio_no(gpio);
    gpio_direction_output(gpio_no, data);
    return 0;
}

int shub_gpio_direction_input(int gpio)
{
    int gpio_no;
    
    gpio_no = shub_get_gpio_no(gpio);
    gpio_direction_input(gpio_no); /* SHMDS_HUB_0110_04 mod */
    return 0;
}

int shub_gpio_set_value(int gpio, int data)
{
    int gpio_no;
    
    gpio_no = shub_get_gpio_no(gpio);
    gpio_set_value(gpio_no, data);
    return 0;
}

int shub_gpio_get_value(int gpio)
{
    int ret;
    int gpio_no;
    
    gpio_no = shub_get_gpio_no(gpio);
    ret = gpio_get_value(gpio_no);
    return ret;
}

int shub_gpio_tlmm_config(int gpio, int data)
{
#ifdef SHUB_SW_PINCTRL
    unsigned long config = PIN_CONFIG_BIAS_PULL_DOWN;
    
    if(data == SHUB_GPIO_PULL_UP){
        config = PIN_CONFIG_BIAS_PULL_UP;
    }
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        pin_config_set("fd510000.pinctrl", "gp-93", config);
        break;
    case SHUB_GPIO_PIN_BRMP:
        pin_config_set("fd510000.pinctrl", "gp-94", config);
        break;
    case SHUB_GPIO_PIN_INT0:
        pin_config_set("fd510000.pinctrl", "gp-75", config);
        break;
    case SHUB_GPIO_PIN_INT1:
        pin_config_set("fd510000.pinctrl", "gp-76", config);
        break;
    default:
        break;
    }
#else
    switch(gpio){
    case SHUB_GPIO_PIN_RESET:
        gpio_tlmm_config(GPIO_CFG(SHUB_GPIO_RST, 0, GPIO_CFG_INPUT, data, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        break;
    case SHUB_GPIO_PIN_BRMP:
        gpio_tlmm_config(GPIO_CFG(SHUB_GPIO_REMP, 0, GPIO_CFG_INPUT, data, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        break;
    case SHUB_GPIO_PIN_INT0:
        gpio_tlmm_config(GPIO_CFG(SHUB_GPIO_INT0, 0, GPIO_CFG_INPUT, data, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        break;
    case SHUB_GPIO_PIN_INT1:
        gpio_tlmm_config(GPIO_CFG(SHUB_GPIO_INT1, 0, GPIO_CFG_INPUT, data, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        break;
    default:
        break;
    }
#endif
    return 0;
}

int shub_gpio_to_irq(int gpio)
{
    int gpio_no = 0;
    int ret  = 0;
    
    gpio_no = shub_get_gpio_no(gpio);
    ret = gpio_to_irq(gpio_no);
    return ret;
}
/* SHMDS_HUB_0110_01 add E */

