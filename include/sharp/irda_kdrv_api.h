/* include/sharp/irda_kdrv_api.h (sharp IrDA driver)
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
#ifndef	_IRDA_KDRV_API_H
#define	_IRDA_KDRV_API_H

#include <linux/ioctl.h>
#include <linux/tty.h>

#include "irda_common.h"


#define	SHIRDA_DEVFILE_NAME		"msm_shirda"
#define	SHIRDA_DEVFILE			"/dev/"SHIRDA_DEVFILE_NAME

#define	SHIRDA_DEVFILE_NAME2		"msm_shirda2"
#define	SHIRDA_DEVFILE2			"/dev/"SHIRDA_DEVFILE_NAME2
#define	SHIRDA_DEVFILE_NAME3		"msm_shirda3"
#define	SHIRDA_DEVFILE3			"/dev/"SHIRDA_DEVFILE_NAME3

#define SHIRDA_LDISC_DRIVER_NAME	"shirda"

#define	SHIRB_LDISC_NAME		"shirb_ldisc"


#ifdef	__KERNEL__
#define	N_SHIRDA	N_IRDA

#else
#include	<utils/Log.h>
#include	<ctype.h>
#include	<dirent.h>
#include	<string.h>
#include	<stdio.h>

#define	SHIRDA_PARA_PATH	"/sys/module/shirda_msm_kdrv/parameters/"
#define	SHIRDA_NODE_PATH	SHIRDA_PARA_PATH"node"
#define	SHIRDA_TTYDEV_PATH	"/sys/bus/platform/devices/%s/tty"

inline static char *get_tty_devname() {
	int		fd;
	int		i, n;
	static char	tty_device_name[64];
	static char	devname_path[64];
	char		node[64];
	DIR		*dir;
	struct dirent	*ent;

	fd = open(SHIRDA_NODE_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("NODE = %s open error", SHIRDA_NODE_PATH);
		return NULL;
	}
	n = read(fd, node, 63);
	close(fd);

	for (i = 0; i < n; i++) {
		if (iscntrl((int)node[i]) != 0) {
			n = i;
			ALOGE("%dth charactor = 0x%02x", i, node[i]);
			break;
		}
	}
	if (n < 1) {
		ALOGE("Charactor deleate error");
		return NULL;
	}
	node[n] = (char)0x00;

	sprintf(devname_path, SHIRDA_TTYDEV_PATH, node);

	if ((dir = opendir(devname_path)) == NULL) {
		ALOGE("DIR %s open error", devname_path);
		return NULL;
	}
	do {
		if ((ent = readdir(dir)) == NULL) {
			closedir(dir);
			ALOGE("DIR read error");
			return NULL;
		}
	} while (strstr(ent->d_name, "tty") == NULL);
	closedir(dir);

	sprintf(tty_device_name, "/dev/%s", ent->d_name);
	return tty_device_name;
}

#define	TTYHS_DEVFILE ((const char *)(get_tty_devname()))

#define	SHIRDA_PROC_TTY_LDISCS_PATH	"/proc/tty/ldiscs"

inline static int get_shirda_ldisc_id(const char *ldisc_name)
{
	FILE	*fd;
	int	num_param;
	int	id;
	char	sbuf[64];
	char	name[64];

	fd = fopen(SHIRDA_PROC_TTY_LDISCS_PATH, "r");
	if (fd == NULL) {
		ALOGE("tty ldisc info file %s open error",
						SHIRDA_PROC_TTY_LDISCS_PATH);
		return -1;
	}

	while (fgets(sbuf, 64, fd) != NULL) {
		num_param = sscanf(sbuf, "%s%d", name, &id);
		if (num_param != 2) {
			ALOGE("ldisc info param error (%d)", num_param);
			return -1;
		}

		if (strcmp(name, ldisc_name) == 0) {
			return id;
		}
	}

	return -1;
}

#define	N_SHIRDA	(get_shirda_ldisc_id(SHIRDA_LDISC_DRIVER_NAME))
#define	N_SHIRB		(get_shirda_ldisc_id(SHIRB_LDISC_NAME))

#endif

#define	IRDA_DRV_IOCTL_MAGIC	'i'
#define	IRDA_DRV_IOCTL_SET_QOS		_IOW(IRDA_DRV_IOCTL_MAGIC, 1, \
							irda_qos_info)
#define	IRDA_DRV_IOCTL_GET_QOS		_IOR(IRDA_DRV_IOCTL_MAGIC, 2, \
							irda_qos_info)
#define	IRDA_DRV_IOCTL_READ_WAKEUP	_IO (IRDA_DRV_IOCTL_MAGIC, 3)
#define	IRDA_DRV_IOCTL_GET_ERR		_IOR(IRDA_DRV_IOCTL_MAGIC, 4, int)
#define	IRDA_DRV_IOCTL_LOOPBACK 	_IO (IRDA_DRV_IOCTL_MAGIC, 5)
#define	IRDA_DRV_IOCTL_GET_CAPABILITY 	_IOR(IRDA_DRV_IOCTL_MAGIC, 6, \
							irda_kdrv_capa_notify)
#define	IRDA_DRV_IOCTL_GET_MEDIABUSY	_IOR(IRDA_DRV_IOCTL_MAGIC, 7, int)
#define	IRDA_DRV_IOCTL_CLR_MEDIABUSY	_IO(IRDA_DRV_IOCTL_MAGIC,  8)

#define	IRDA_LDISC_NO_ERR		(1)
#define	IRDA_LDISC_READ_CANCELED	(2)
#define	IRDA_LDISC_LOGICAL_ERR		(3)
#define	IRDA_LDISC_PERMISSION_ERR	(4)
#define	IRDA_LDISC_TX_TIMEOUT		(5)
#define	IRDA_LDISC_RX_TIMEOUT		(6)
#define	IRDA_LDISC_CLOSED		(7)
#define	IRDA_LDISC_RX_BUFFER_OVERFLOW	(8)
#define	IRDA_LDISC_TX_SEND_ERR		(9)

#define	IRDA_LDISC_MEDIA_FREE		(0)
#define	IRDA_LDISC_MEDIA_BUSY		(1)

#endif
