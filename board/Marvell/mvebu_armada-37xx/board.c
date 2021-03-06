/*
 * Copyright (C) 2016 Stefan Roese <sr@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <phy.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/soc.h>
#include <power/regulator.h>
#include <asm-generic/gpio.h>
#include <dt-bindings/gpio/armada-3700-gpio.h>
#include <dm/device.h>
#include <dm/lists.h>
#include <dm/pinctrl.h>
#include <dm/uclass.h>
#ifdef CONFIG_BOARD_CONFIG_EEPROM
#include <mvebu_cfg_eeprom.h>
#endif
#include <nm_common.h>

DECLARE_GLOBAL_DATA_PTR;

/* on Armada3700 rev2 devel-board, IO expander (with I2C address 0x22) bit
 * 14 is used as Serdes Lane 2 muxing, which could be used as SATA PHY or
 * USB3 PHY.
 */
enum COMPHY_LANE2_MUXING {
	COMPHY_LANE2_MUX_USB3,
	COMPHY_LANE2_MUX_SATA
};

/* IO expander I2C device */
#define I2C_IO_EXP_ADDR		0x22
#define I2C_IO_CFG_REG_0	0x6
#define I2C_IO_DATA_OUT_REG_0	0x2
#define I2C_IO_REG_0_SATA_OFF	2
#define I2C_IO_REG_0_USB_H_OFF	1
#define I2C_IO_COMPHY_SATA3_USB_MUX_BIT	14

/*
* For Armada3700 A0 chip, comphy serdes lane 2 could be used as PHY for SATA
* or USB3.
* For Armada3700 rev2 devel-board, pin 14 of IO expander PCA9555 with I2C
* address 0x22 is used as Serdes Lane 2 muxing; the pin needs to be set in
* output mode: high level is for SATA while low level is for USB3;
*/
static int board_comphy_usb3_sata_mux(enum COMPHY_LANE2_MUXING comphy_mux)
{
	int ret;
	u8 buf[8];
	struct udevice *i2c_dev;
	int i2c_byte, i2c_bit_in_byte;

	if (!of_machine_is_compatible("marvell,armada-3720-db-v2") &&
	    !of_machine_is_compatible("marvell,armada-3720-db-v3"))
		return 0;

	ret = i2c_get_chip_for_busnum(0, I2C_IO_EXP_ADDR, 1, &i2c_dev);
	if (ret) {
		printf("Cannot find PCA9555: %d\n", ret);
		return 0;
	}

	ret = dm_i2c_read(i2c_dev, I2C_IO_CFG_REG_0, buf, 2);
	if (ret) {
		printf("Failed to read IO expander value via I2C\n");
		return ret;
	}

	i2c_byte = I2C_IO_COMPHY_SATA3_USB_MUX_BIT / 8;
	i2c_bit_in_byte = I2C_IO_COMPHY_SATA3_USB_MUX_BIT % 8;

	/* Configure IO exander bit 14 of address 0x22 in output mode */
	buf[i2c_byte] &= ~(1 << i2c_bit_in_byte);
	ret = dm_i2c_write(i2c_dev, I2C_IO_CFG_REG_0, buf, 2);
	if (ret) {
		printf("Failed to set IO expander via I2C\n");
		return ret;
	}

	ret = dm_i2c_read(i2c_dev, I2C_IO_DATA_OUT_REG_0, buf, 2);
	if (ret) {
		printf("Failed to read IO expander value via I2C\n");
		return ret;
	}

	/* Configure output level for IO exander bit 14 of address 0x22 */
	if (comphy_mux == COMPHY_LANE2_MUX_SATA)
		buf[i2c_byte] |= (1 << i2c_bit_in_byte);
	else
		buf[i2c_byte] &= ~(1 << i2c_bit_in_byte);

	ret = dm_i2c_write(i2c_dev, I2C_IO_DATA_OUT_REG_0, buf, 2);
	if (ret) {
		printf("Failed to set IO expander via I2C\n");
		return ret;
	}

	return 0;
}

int board_early_init_f(void)
{
#ifdef CONFIG_BOARD_CONFIG_EEPROM
	cfg_eeprom_init();
#endif

#ifdef CONFIG_MVEBU_SYS_INFO
	/*
	 * Call this function to transfer data from address 0x4000000
	 * into a global struct, before code relocation.
	 */
	sys_info_init();
#endif
	return 0;
}

int board_usb3_vbus_init(void)
{
#if defined(CONFIG_DM_REGULATOR)
	struct udevice *regulator;
	int ret;

	/* lower usb vbus  */
	ret = regulator_get_by_platname("usb3-vbus", &regulator);
	if (ret) {
		debug("Cannot get usb3-vbus regulator\n");
		return 0;
	}

	ret = regulator_set_enable(regulator, false);
	if (ret) {
		error("Failed to turn OFF the VBUS regulator\n");
		return ret;
	}
#endif
	return 0;
}

static int  log_out(int enable){
#ifdef CONFIG_SILENT_CONSOLE
	if (enable)
		gd->flags &= ~GD_FLG_SILENT;
	else
		gd->flags |= GD_FLG_SILENT;
#endif
	return 0;
}

extern int do_i2c(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[]);
static void aw2013_init(void)
{
	cmd_tbl_t *cmdtp = NULL;
	int flag = 0;
	int argc = 3;
	char *argv[4];

	log_out(0);
	argv[1] = "dev";
	argv[2] = "0";
	do_i2c(cmdtp, flag, argc, argv);

	argc = 2;
	argv[1] = "probe";
	do_i2c(cmdtp, flag, argc, argv);
	log_out(1);

	return ;
}

static int get_board_type(void)
{
	int value = 0, ret;
	unsigned int selector = 0, gpio[3];;
	struct udevice *dev;
	const struct pinctrl_ops *ops;

	/* set jtag function as gpio mode */
	ret = uclass_get_device(UCLASS_PINCTRL, 0, &dev);
	if (ret){
		printf("Cannot get armada-37xx-pinctrl udevice\n");
		return 0;
	}
	ops = pinctrl_get_ops(dev);
	if (!ops) {
		dev_dbg(dev, "ops is not set.  Do not bind.\n");
		return 0;
	}
	ret = ops->pinmux_group_set(dev, selector, 1);
	if(ret)
		printf("pinmux_group_set error\n");

	/* set pcie reset function to gpio */
	ret = uclass_get_device(UCLASS_PINCTRL, 1, &dev);
	if (ret){
		printf("Cannot get armada-37xx-pinctrl udevice\n");
		return 0;
	}
	ops = pinctrl_get_ops(dev);
	if (!ops) {
		dev_dbg(dev, "ops is not set.  Do not bind.\n");
		return 0;
	}
	selector = 5;
	ret = ops->pinmux_group_set(dev, selector, 1);
	if(ret)
		printf("pinmux_group_set error\n");

	ret = gpio_lookup_name(VERCTL_0, NULL, NULL, &gpio[0]);
	if (ret)
		printf("GPIO: '%s' not found\n", VERCTL_0);
	
	ret = gpio_lookup_name(VERCTL_1, NULL, NULL, &gpio[1]);
	if (ret)
		printf("GPIO: '%s' not found\n", VERCTL_1);

	ret = gpio_lookup_name(VERCTL_2, NULL, NULL, &gpio[2]);
	if (ret)
		printf("GPIO: '%s' not found\n", VERCTL_2);

	gpio_free(gpio[0]);
	gpio_free(gpio[1]);
	gpio_free(gpio[2]);
	
	gpio_request(gpio[0], "verctl_0");
	gpio_request(gpio[1], "verctl_1");
	gpio_request(gpio[2], "verctl_2");
	
	gpio_direction_input(gpio[0]);
	gpio_direction_input(gpio[1]);
	gpio_direction_input(gpio[2]);
	
	mdelay(2);
	
	value += 1 * gpio_get_value(gpio[0]);
	value += 2 * gpio_get_value(gpio[1]);
	value += 4 * gpio_get_value(gpio[2]);
	
	gd->board_type = value;

	return 0;
}

static int hwware_reset(void)
{
	unsigned int gpio;
	int ret;

	ret = gpio_lookup_name(LED_INT, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", LED_INT);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "led_int");
		gpio_direction_input(gpio);
	}

	ret = gpio_lookup_name(FIQ_INT, NULL, NULL, &gpio);
	if (ret) 
		printf("GPIO: '%s' not found\n", FIQ_INT);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "fiq_int");
		gpio_direction_input(gpio);
	}

	/*
	ret = gpio_lookup_name(EN_5V, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", EN_5V);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "en_5v");
		gpio_direction_output(gpio, 0);
	}
	*/

	ret = gpio_lookup_name(KEY_RESET, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", KEY_RESET);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "key_reset");
		gpio_direction_input(gpio);
	}

	ret = gpio_lookup_name(FAN_CTL, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", FAN_CTL);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "fan_ctl");
		gpio_direction_output(gpio, 1);
	}

	ret = gpio_lookup_name(PHY_INT, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", PHY_INT);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "phy_int");
		gpio_direction_input(gpio);
	}

	ret = gpio_lookup_name(PHY_RESET, NULL, NULL, &gpio);
	if (ret)
		printf("GPIO: '%s' not found\n", PHY_RESET);
	else{
		gpio_free(gpio);
		gpio_request(gpio, "phy_reset");
		gpio_direction_output(gpio, 1);
		mdelay(20);
		gpio_set_value(gpio, 0);
		mdelay(20);
		gpio_set_value(gpio, 1);
	}

	return 0;
}

int board_init(void)
{
	/* board_usb3_vbus_init(); */

	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
#ifdef CONFIG_OF_CONTROL
	printf("U-Boot DT blob at : %p\n", gd->fdt_blob);
#endif
	get_board_type();
	if (NM05 == gd->board_type) {
		hwware_reset();
		aw2013_init();
	}
	
	/* enable serdes lane 2 mux for sata phy */
	/* board_comphy_usb3_sata_mux(COMPHY_LANE2_MUX_SATA); */

	return 0;
}

/* Board specific AHCI / SATA enable code */
int board_ahci_enable(struct udevice *dev)
{
#if defined(CONFIG_DM_REGULATOR)
	int ret;
	struct udevice *regulator;

	ret = device_get_supply_regulator(dev, "power-supply",
					  &regulator);
	if (ret) {
		debug("%s: No sata power supply\n", dev->name);
		return 0;
	}

	ret = regulator_set_enable(regulator, true);
	if (ret) {
		error("Error enabling sata power supply\n");
		return ret;
	}
#endif
	return 0;
}
