/*
 * Copyright (C) 2016 Stefan Roese <sr@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _CONFIG_MVEBU_ARMADA_H
#define _CONFIG_MVEBU_ARMADA_H

#include <asm/arch/soc.h>

/*
 * High Level Configuration Options (easy to change)
 */

#define	CONFIG_SYS_TEXT_BASE	0x00000000

/* additions for new ARM relocation support */
#define CONFIG_SYS_SDRAM_BASE	0x00000000


/* auto boot */
#define CONFIG_PREBOOT

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, \
					  115200, 230400, 460800, 921600 }

/* Default Env vars */
#define CONFIG_IPADDR			192.168.1.11	/* In order to cause an error */
#define CONFIG_SERVERIP			192.168.1.100	/* In order to cause an error */
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_GATEWAYIP		10.4.50.254
#define CONFIG_HAS_ETH1
#define CONFIG_HAS_ETH2
#define CONFIG_ETHPRIME			"eth0"
#define CONFIG_EXTRA_ENV_SETTINGS	"kernel_addr=0x8000000\0"	\
					"initrd_addr=0xa00000\0"	\
					"initrd_size=0x2000000\0"	\
					"fdt_addr=0x7f00000\0"		\
					"loadaddr=0x8000000\0"		\
					"fdt_high=0xffffffffffffffff\0"	\
					"hostname=marvell\0"		\
					"fdt_name=armada-3720-catdrive.dtb\0"		\
					"netdev=eth0\0"			\
					"ethaddr=00:51:82:11:22:00\0"	\
					"mode=normal\0" \
					"factory_os=menuboot-armada3720.bin\0" \
					"update_os=catdrive_update\0" \
					"normal_os=catdrive_normal\0" \
					"image_name=-\0" \
					"get_dtb=if test $fdt_name != -; then " \
							"$get_dtb_medium $fdt_addr $fdt_name; " \
							"setenv boot_cmd \"booti $kernel_addr - $fdt_addr\"; " \
						"else " \
							"setenv boot_cmd \"bootm $kernel_addr#config@1\"; " \
						"fi\0"	\
					"get_mode=if test $mode = factory; then " \
							"setenv image_name $factory_os; " \
						"elif  test $mode = update; then " \
							"setenv image_name $update_os; " \
						"else " \
							"setenv image_name $normal_os; " \
						"fi\0"	\
					"get_images_tftp=setenv get_dtb_medium tftpboot; " \
						"$get_dtb_medium $kernel_addr $image_name; " \
						"run get_dtb\0"	 \
					"get_images_usb=usb reset; setenv get_dtb_medium \"fatload usb 0\"; " \
						"$get_dtb_medium $kernel_addr $image_name; "\
						"run get_dtb\0"		\
					"get_images_mmc=mmc dev 1; setenv get_dtb_medium \"ext4load mmc 1:1\"; "\
						"$get_dtb_medium $kernel_addr $image_name; " \
						"run get_dtb\0"	\
					"console=" CONFIG_DEFAULT_CONSOLE "\0"\
					"set_bootargs=if test $mode = factory; then " \
							"setenv bootargs \"$console quiet\"; " \
						"else " \
							"setenv bootargs $console; " \
						"fi\0"	\
					"bootcmd_tftp=run get_mode; run get_images_tftp; run set_bootargs; ""$boot_cmd\0" \
					"bootcmd_usb=run get_mode; run get_images_usb; run set_bootargs; $boot_cmd\0" \
					"bootcmd_mmc=run get_mode; run get_images_mmc; run set_bootargs; $boot_cmd\0"
#define CONFIG_BOOTCOMMAND	"run get_mode; run get_images_mmc; run set_bootargs; $boot_cmd"
#define CONFIG_ENV_OVERWRITE	/* ethaddr can be reprogrammed */
/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs  */
#define CONFIG_INITRD_TAG		/* enable INITRD tag */
#define CONFIG_SETUP_MEMORY_TAGS	/* enable memory tag */

#define	CONFIG_SYS_CBSIZE	1024	/* Console I/O Buff Size */
#define	CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE \
		+sizeof(CONFIG_SYS_PROMPT) + 16)	/* Print Buff */

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN	(8 << 20) /* 8MiB for malloc() */

/*
 * Other required minimal configurations
 */
#define CONFIG_SYS_LONGHELP
#define CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING
#define CONFIG_ARCH_CPU_INIT		/* call arch_cpu_init() */
#define CONFIG_SYS_LOAD_ADDR	0x00800000	/* default load adr- 8M */
#define CONFIG_SYS_MEMTEST_START 0x00800000	/* 8M */
#define CONFIG_SYS_MEMTEST_END	0x00ffffff	/*(_16M -1) */
#define CONFIG_SYS_RESET_ADDRESS 0xffff0000	/* Rst Vector Adr */
#define CONFIG_SYS_MAXARGS	32	/* max number of command args */

#define CONFIG_SYS_ALT_MEMTEST

/* End of 16M scrubbed by training in bootrom */
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SYS_TEXT_BASE + 0xFF0000)

/*
 * SPI Flash configuration
 */
#define CONFIG_ENV_SPI_BUS		0
#define CONFIG_ENV_SPI_CS		0

/* SPI NOR flash default params, used by sf commands */
#define CONFIG_SF_DEFAULT_SPEED		1000000
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE

/* Environment in SPI NOR flash */
#ifdef CONFIG_MVEBU_SPI_BOOT
#define CONFIG_ENV_IS_IN_SPI_FLASH
/* Environment in NAND flash */
#elif defined(CONFIG_MVEBU_NAND_BOOT)
#define CONFIG_ENV_IS_IN_NAND
/* Environment in MMC */
#elif defined(CONFIG_MVEBU_MMC_BOOT)
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_PART		1 /* 0 - DATA, 1 - BOOT0, 2 - BOOT1 */
#endif

/* Assume minimum flash/eMMC boot partition size of 4MB
 * and save the environment at the end of the boot device
*/
#ifdef CONFIG_SPI_FLASH_SPANSION
#define CONFIG_ENV_SIZE				(256 << 10) /* 256KiB */
#define CONFIG_ENV_SECT_SIZE		(256 << 10) /* 256KiB sectors */
#else
#define CONFIG_ENV_SIZE				( 64 << 10) /* 64KiB */
#define CONFIG_ENV_SECT_SIZE		( 64 << 10) /* 64KiB sectors */
#endif
#ifdef CONFIG_MVEBU_NAND_BOOT
#define CONFIG_ENV_OFFSET		0x400000
#else
#define CONFIG_ENV_OFFSET		(0x200000 - CONFIG_ENV_SIZE)
#endif

#define CONFIG_USB_MAX_CONTROLLER_COUNT (CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS + \
					 CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS)

/* USB ethernet */
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_ETHER_MCS7830
#define CONFIG_USB_ETHER_RTL8152
#define CONFIG_USB_ETHER_SMSC95XX

/*
 * SATA/SCSI/AHCI configuration
 */
#define CONFIG_SCSI
#define CONFIG_SCSI_AHCI
#define CONFIG_SCSI_AHCI_PLAT
#define CONFIG_LIBATA
#define CONFIG_LBA48
#define CONFIG_SYS_64BIT_LBA

#define CONFIG_SYS_SCSI_MAX_SCSI_ID	2
#define CONFIG_SYS_SCSI_MAX_LUN		1
#define CONFIG_SYS_SCSI_MAX_DEVICE	(CONFIG_SYS_SCSI_MAX_SCSI_ID * \
					 CONFIG_SYS_SCSI_MAX_LUN)

/* MMC/SD IP block */
#define CONFIG_SUPPORT_VFAT

/*
 * The EEPROM ST M24C64 has 32 byte page write mode and takes up to 10 msec.
 */
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS 10

/*
 * nm01 catdisk Config
 */
#define CONFIG_BOARD_TYPES
#define CONFIG_FAT_WRITE

#endif /* _CONFIG_MVEBU_ARMADA_H */
