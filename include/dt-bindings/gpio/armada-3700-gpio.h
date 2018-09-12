/*
 * This header provides constants for binding marvell,armada-3700-gpio.
 *
 * The first cell in armada-3700's GPIO specifier is the GPIO bank reference.
 *
 * The second cell in armada-3700's GPIO specifier is the global GPIO ID. The macros below
 * provide names for this.
 *
 * The third cell contains standard flag values specified in gpio.h.
 */

#ifndef _DT_BINDINGS_GPIO_ARMADA_3700_GPIO_H
#define _DT_BINDINGS_GPIO_ARMADA_3700_GPIO_H

#include <dt-bindings/gpio/gpio.h>


#define WLED           			"GPIO111"
#define SYS_LED        			"GPIO112"
#define LTE_LED        			"GPIO113"
#define PWM3_LED       			"GPIO114"
#define VERCTL_0      			"GPIO121"
#define VERCTL_1       			"GPIO122"
#define VERCTL_2       			"GPIO123"

#define LED_INT					"GPIO14"
#define FIQ_INT					"GPIO119"

#define EN_5V					"GPIO20"
#define KEY_RESET				"GPIO23"
#define FAN_CTL					"GPIO24"
#define PHY_INT			 		"GPIO220"
#define PHY_RESET		 		"GPIO221"


#endif
