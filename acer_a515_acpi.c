#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/acpi.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/platform_profile.h>
#include <linux/proc_fs.h>// Module metadata

MODULE_AUTHOR("embeddedt");
MODULE_DESCRIPTION("Acer Aspire A515 platform profile driver");
MODULE_LICENSE("GPL");

#define A515_PLATFORM_MODE_REGISTER 0x10
#define A515_KBD_BL_REGISTER 0x30


#define A515_SILENT_MODE 0x02
#define A515_NORMAL_MODE 0x00
#define A515_PERF_MODE 0x03

/* Platform profile ***********************************************************/
static int platform_profile_get(struct platform_profile_handler *pprof,
				enum platform_profile_option *profile)
{
    u8 result;
    int err;
    err = ec_read(A515_PLATFORM_MODE_REGISTER, &result);
    if(err)
        return err;
    switch(result) {
        case A515_NORMAL_MODE:
            *profile = PLATFORM_PROFILE_BALANCED;
            break;
        case A515_SILENT_MODE:
            *profile = PLATFORM_PROFILE_QUIET;
            break;
        case A515_PERF_MODE:
            *profile = PLATFORM_PROFILE_PERFORMANCE;
            break;
        default:
            return -EINVAL;
    }
	return 0;
}

static int platform_profile_set(struct platform_profile_handler *pprof,
				enum platform_profile_option profile)
{
    u8 reg_v;
    int err;
    switch(profile) {
        case PLATFORM_PROFILE_PERFORMANCE:
            reg_v = A515_PERF_MODE;
            break;
        case PLATFORM_PROFILE_QUIET:
            reg_v = A515_SILENT_MODE;
            break;
        case PLATFORM_PROFILE_BALANCED:
            reg_v = A515_NORMAL_MODE;
            break;
        default:
            return -EOPNOTSUPP;
    }
    err = ec_write(A515_PLATFORM_MODE_REGISTER, reg_v);
    if(err)
        return err;
    return 0;
}

static struct platform_profile_handler acer_profile = {
	.profile_get = platform_profile_get,
	.profile_set = platform_profile_set,
};

static enum led_brightness kbd_led_level_get(struct led_classdev *led_cdev)
{
    u8 result;
    int err = ec_read(A515_KBD_BL_REGISTER, &result);
    if(err)
        return err;
    return result;
}

static int kbd_led_level_set(struct led_classdev *led_cdev,
			     enum led_brightness value)
{
    return ec_write(A515_KBD_BL_REGISTER, value ? 0x1 : 0x0);
}

static struct led_classdev kbd_led = {
	.name           = "acer::kbd_backlight",
	.flags		= LED_CORE_SUSPENDRESUME,
	.brightness_set_blocking = kbd_led_level_set,
	.brightness_get = kbd_led_level_get,
    .max_brightness = 1
};

static struct platform_driver platform_driver = {
	.driver = {
		.name = "acer-a515-laptop-2",
	}
};

static struct platform_device *platform_device;

static int __init acer_a515_acpi_init(void) {
    int err;
    err = platform_driver_register(&platform_driver);
	if (err)
		return err;
	platform_device = platform_device_alloc("acer-a515-laptop-2", -1);
	if (!platform_device) {
        return -ENOMEM;
	}
	err = platform_device_add(platform_device);
	if (err)
		return err;
    set_bit(PLATFORM_PROFILE_QUIET, acer_profile.choices);
	set_bit(PLATFORM_PROFILE_BALANCED, acer_profile.choices);
	set_bit(PLATFORM_PROFILE_PERFORMANCE, acer_profile.choices);
    err = platform_profile_register(&acer_profile);
    if(err)
        return err;
    err = led_classdev_register(&platform_device->dev, &kbd_led);
    if(err)
        return err;
    printk(KERN_INFO "Acer Aspire E515 ACPI driver loaded.");
    return 0;
}
static void __exit acer_a515_acpi_exit(void) {
    platform_profile_remove();
    led_classdev_unregister(&kbd_led);
    if (platform_device) {
		platform_device_unregister(platform_device);
		platform_driver_unregister(&platform_driver);
	}
}

module_init(acer_a515_acpi_init);
module_exit(acer_a515_acpi_exit);
