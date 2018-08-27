#include <common.h>
#include <dm.h>
#include <errno.h>
#include <led.h>
#include <asm/gpio.h>
#include <dm/lists.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

struct aw2013_info {
	int brightness;
	int max_brightness;
	int max_current;
	int rise_time_ms;
	int hold_time_ms;
	int fall_time_ms;
	int off_time_ms;
	int id;
	struct udevice *dev;
};

/* register address */
#define AW_REG_RESET                    0x00
#define AW_REG_GLOBAL_CONTROL           0x01
#define AW_REG_LED_STATUS               0x02
#define AW_REG_LED_ENABLE               0x30
#define AW_REG_LED_CONFIG_BASE          0x31
#define AW_REG_LED_BRIGHTNESS_BASE      0x34
#define AW_REG_TIMESET0_BASE            0x37
#define AW_REG_TIMESET1_BASE            0x38

/* register bits */
#define AW2013_CHIPID                   0x33
#define AW_LED_MOUDLE_ENABLE_MASK       0x01
#define AW_LED_FADE_OFF_MASK            0x40
#define AW_LED_FADE_ON_MASK             0x20
#define AW_LED_BREATHE_MODE_MASK        0x10
#define AW_LED_RESET_MASK               0x55

#define AW_LED_RESET_DELAY              8
#define AW_LED_POWER_ON_DELAY   1
#define AW_LED_POWER_OFF_DELAY  10
#define AW2013_VDD_MIN_UV               2600000
#define AW2013_VDD_MAX_UV               3300000
#define AW2013_VI2C_MIN_UV              1800000
#define AW2013_VI2C_MAX_UV              1800000

#define MAX_RISE_TIME_MS                7
#define MAX_HOLD_TIME_MS                5
#define MAX_FALL_TIME_MS                7
#define MAX_OFF_TIME_MS                 5


static int aw2013_write(struct udevice *dev, u8 reg, u8 val)
{
	uchar buf[1];
	
	buf[0] = val;
	
	return dm_i2c_write(dev, reg, buf, 1);
}

static int aw2013_read(struct udevice *dev, u8 reg, u8 *val)
{
	uchar buf[1];
	int ret;

	ret = dm_i2c_read(dev, reg, buf, 1);
	if (ret)
		return ret;
	
	*val = buf[0];
	
	return 0;
}


static int aw2013_check_chipid(struct udevice *dev)
{
	uchar buf[1];
	int  ret;

	ret = aw2013_write(dev, AW_REG_RESET, AW_LED_RESET_MASK);
	if (ret < 0)
		return ret;
	udelay(AW_LED_RESET_DELAY);
	
	ret = aw2013_read(dev, AW_REG_RESET, buf);
	if (ret < 0)
		return ret;
	
	if (buf[0] != AW2013_CHIPID)
		return -EINVAL;
	else
		return 0;
}

static void aw2013_power_on(struct udevice *dev, int id)
{
	u8 val = 0;
	
	aw2013_read(dev, AW_REG_LED_ENABLE, &val);
	if (val == 0) {
		val = 1 << id;
		aw2013_write(dev, AW_REG_LED_ENABLE, val);
	}

	return ;
}

static void aw2013_led_blink_set(struct udevice *dev, unsigned long blinking)
{
	u8 val = 0;
	struct aw2013_info *aw2013_info = dev_get_priv(dev);
	int id = aw2013_info->id;
	if (id > 3)
		return ;
	
	aw2013_info->brightness = blinking ? aw2013_info->max_brightness : 0;
	if (blinking > 0) {
		aw2013_write(dev, AW_REG_GLOBAL_CONTROL, AW_LED_MOUDLE_ENABLE_MASK);
		aw2013_write(dev, AW_REG_LED_CONFIG_BASE + id,
			AW_LED_FADE_OFF_MASK | AW_LED_FADE_ON_MASK |
			AW_LED_BREATHE_MODE_MASK | aw2013_info->max_current);
		aw2013_write(dev, AW_REG_LED_BRIGHTNESS_BASE + id,
			aw2013_info->brightness);
			
		aw2013_write(dev, AW_REG_TIMESET0_BASE + id * 3,
			aw2013_info->rise_time_ms << 4 |
			aw2013_info->hold_time_ms);
		aw2013_write(dev, AW_REG_TIMESET1_BASE + id * 3,
			aw2013_info->fall_time_ms << 4 |
			aw2013_info->off_time_ms);
		aw2013_read(dev, AW_REG_LED_ENABLE, &val);
		aw2013_write(dev, AW_REG_LED_ENABLE, val | (1 << id));
	} else {
		aw2013_read(dev, AW_REG_LED_ENABLE, &val);
		aw2013_write(dev, AW_REG_LED_ENABLE, val & (~(1 << id)));
	}

	aw2013_power_on(dev, id);

	return ;

}

static int aw2013_led_probe(struct udevice *dev)
{
	struct aw2013_info *aw2013_info = dev_get_priv(dev);
	void *blob = (void *)gd->fdt_blob;
	int node = dev_of_offset(dev);
	int ret = -1;
	
	aw2013_info->max_brightness = fdtdec_get_int(blob, node, "max-brightness", 0);
	aw2013_info->max_current 	= fdtdec_get_int(blob, node, "max-current", 0);
	aw2013_info->rise_time_ms 	= fdtdec_get_int(blob, node, "rise-time-ms", 0);
	aw2013_info->hold_time_ms	= fdtdec_get_int(blob, node, "hold-time-ms", 0);
	aw2013_info->fall_time_ms	= fdtdec_get_int(blob, node, "fall-time-ms", 0);
	aw2013_info->off_time_ms	= fdtdec_get_int(blob, node, "off-time-ms", 0);
	aw2013_info->id 			= fdtdec_get_int(blob, node, "breathing-id", 0);

	ret = aw2013_check_chipid(dev);
	if (ret)
		return ret;
	
	aw2013_led_blink_set(dev, 1);
	
	return 0;
}

static const struct udevice_id aw2013_ids[] = {
	{ .compatible = "awinic,aw2013" },
	{ }
};

U_BOOT_DRIVER(led_aw2013) = {
	.name	= "aw2013_led",
	.id	= UCLASS_LED,
/* 	.ops	= &aw2013_led_ops, */
	.probe	= aw2013_led_probe,
	.priv_auto_alloc_size = sizeof(struct aw2013_info),
	.of_match = aw2013_ids,
};

