/* $Id: wiimote_link.c 16 2007-01-22 21:51:27Z bja $ 
 *
 * Copyright (C) 2007, Joel Andersson <bja@kth.se>
 * Copyright (C) 2007, Krishna Gadepalli
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <arpa/inet.h>
#include <errno.h>

#include "bthid.h"
#include "wiimote.h"
#include "wiimote_report.h"
#include "wiimote_error.h"
#include "wiimote_io.h"

#define WIIMOTE_NAME    "Nintendo RVL-CNT-01"
#define WIIMOTE_CMP_LEN sizeof(WIIMOTE_NAME)

/*
 * Wiimote magic numbers used for identifying
 * controllers during inquiry.
 */
static uint8_t WIIMOTE_DEV_CLASS[] = { 0x04, 0x25, 0x00 };

static int __l2cap_connect(const char *host, uint16_t psm)
{
    struct sockaddr_l2 addr = { 0 };
    int sock = 0;

    sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (!sock) {
	wiimote_error("l2cap_connect(): socket");
	return WIIMOTE_ERROR;
    }

    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(psm);
    str2ba(host, &addr.l2_bdaddr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
	wiimote_error("l2cap_connect(): connect");
	return WIIMOTE_ERROR;
    }

    return sock;
}

static void init_calibration_data(wiimote_t *wiimote)
{
    uint8_t buf[16];
    memset(buf,0,16);
    wiimote_read(wiimote, 0x20, buf, 16);
    memcpy(&wiimote->cal, buf, sizeof (wiimote_cal_t));
}

static int set_link_addr(wiimote_t *wiimote, const char *host)
{
    int dev_id=-1, dd=-1;
    bdaddr_t addr;
	
    dev_id = hci_get_route(BDADDR_LOCAL);
    if (dev_id < 0) {
	wiimote_error("wiimote_connect(): hci_get_route: %m");
	return WIIMOTE_ERROR;
    }
	
    dd = hci_open_dev(dev_id);
    if (dd < 0) {
	wiimote_error("wiimote_connect(): hci_open_dev: %m");
	return WIIMOTE_ERROR;
    }
	
    if (hci_read_bd_addr(dd, &addr, 0) < 0) {
    	wiimote_error("wiimote_connect(): hci_read_bd_addr: %m");
    	return WIIMOTE_ERROR;
    }

    if (hci_close_dev(dd) < 0) {
    	wiimote_error("wiimote_connect(): hci_close_dev: %m");
    	return WIIMOTE_ERROR;		
    }
    
    ba2str(&addr, wiimote->link.l_addr);
    strncpy(wiimote->link.r_addr, host, 19);
	
    return WIIMOTE_OK;
}

static int is_wiimote(int hci_sock, inquiry_info *dev)
{
    char dev_name[WIIMOTE_CMP_LEN];

    if (memcmp(dev->dev_class, WIIMOTE_DEV_CLASS, sizeof (WIIMOTE_DEV_CLASS))) {
	return 0;
    }

    if (hci_remote_name(hci_sock, &dev->bdaddr, WIIMOTE_CMP_LEN, dev_name, 5000)) {
	wiimote_error("wiimote_discover(): Error reading device name\n");
	return 0;
    }

    if (strncmp(dev_name, WIIMOTE_NAME, WIIMOTE_CMP_LEN)) {
	return 0;
    }

    return 1;
}

int wiimote_discover(wiimote_t *devices, uint8_t size)
{
    int dev_id, hci, dev_count;
    int i, numdevices = 0;
    inquiry_info *dev_list = NULL;

    if (size <= 0) {
        wiimote_error("wiimote_discover(): less than 0 devices specified");
        return WIIMOTE_ERROR;
    }

    if (devices == NULL) {
        wiimote_error("wiimote_discover(): Error allocating devices");
        return WIIMOTE_ERROR;
    }

    if ((dev_id = hci_get_route(NULL)) < 0) {
        wiimote_error("wiimote_discover(): no bluetooth devices found");
        return WIIMOTE_ERROR;
    }
    
    if ((hci = hci_open_dev(dev_id)) < 0) {
        wiimote_error("wiimote_discover(): Error opening Bluetooth device\n");
        return WIIMOTE_ERROR;
    }

    /* Get device list. */
    if ((dev_count = hci_inquiry(dev_id, 2, 256, NULL, &dev_list, IREQ_CACHE_FLUSH)) < 0) {
        wiimote_error("wiimote_discover(): Error on device inquiry");
        return WIIMOTE_ERROR;
    }

    /* Check class and name for Wiimotes. */
    for (i=0; i<dev_count; i++) {
	inquiry_info *dev = &dev_list[i];
	if (is_wiimote(hci, dev)) {
	    ba2str(&dev->bdaddr, devices[numdevices].link.r_addr);
	    numdevices++;
	}
    }

    hci_close_dev(hci);

    if (dev_list) {
        free(dev_list);
    }

    if (numdevices <= 0) {
        wiimote_error("wiimote_discover(): No wiimotes found");
        return WIIMOTE_ERROR;
    }

    return numdevices;
}


int wiimote_connect(wiimote_t *wiimote, const char *host)
{
    wiimote_report_t r = WIIMOTE_REPORT_INIT;
	
    if (wiimote->link.status == WIIMOTE_STATUS_CONNECTED) {
	wiimote_error("wiimote_connect(): already connected");
	return WIIMOTE_ERROR;
    }

    /* According to the BT-HID specification, the control channel should
       be opened first followed by the interrupt channel. */

    wiimote->link.s_ctrl = __l2cap_connect(host, BTHID_PSM_CTRL);
    if (wiimote->link.s_ctrl < 0) {
	wiimote_error("wiimote_connect(): l2cap_connect");
	return WIIMOTE_ERROR;
    }

    wiimote->link.status = WIIMOTE_STATUS_UNDEFINED;
    wiimote->link.s_intr = __l2cap_connect(host, BTHID_PSM_INTR);
    if (wiimote->link.s_intr < 0) {
	wiimote_error("wiimote_connect(): l2cap_connect");
	return WIIMOTE_ERROR;
    }

    wiimote->link.status = WIIMOTE_STATUS_CONNECTED;
    wiimote->mode.bits = WIIMOTE_MODE_DEFAULT;
    wiimote->old.mode.bits = 0;

    /* Fill in the bluetooth address of the local and remote devices. */

    set_link_addr(wiimote, host);	

    init_calibration_data(wiimote);
	
    /* Prepare and send a status report request. This will initialize the
       nunchuk if it is plugged in as a side effect. */

    r.channel = WIIMOTE_RID_STATUS;
    if (wiimote_report(wiimote, &r, sizeof (r.status)) < 0) {
	wiimote_error("wiimote_connect(): status report request failed");
    }

    return WIIMOTE_OK;
}

int wiimote_disconnect(wiimote_t *wiimote)
{
    struct req_raw_out r = { 0 };
	
    if (wiimote->link.status != WIIMOTE_STATUS_CONNECTED) {
	wiimote_set_error("wiimote_disconnect(): not connected");
	return WIIMOTE_OK;
    }
	
    /* Send a VIRTUAL_CABLE_UNPLUG HID_CONTROL request to the remote device. */
	
    r.header = BTHID_TYPE_HID_CONTROL | BTHID_PARAM_VIRTUAL_CABLE_UNPLUG;	
    r.channel = 0x01;
	
    if (send(wiimote->link.s_ctrl, (uint8_t *) &r, 2, 0) < 0) {
	wiimote_error("wiimote_disconnect(): send: %m");
	return WIIMOTE_ERROR;
    }

    /* BT-HID specification says HID_CONTROL requests should not generate
       any HANDSHAKE responses, but it seems like the wiimote generates a
       ERR_FATAL handshake response on a VIRTUAL_CABLE_UNPLUG request. */

    if (recv(wiimote->link.s_ctrl, (uint8_t *) &r, 2, 0) < 0) {
	wiimote_error("wiimote_disconnect(): recv: %m");
	return WIIMOTE_ERROR;
    }
		
    if (close(wiimote->link.s_intr) < 0) {
	wiimote_error("wiimote_disconnect(): close: %m");
	return WIIMOTE_ERROR;
    }
    
    if (close(wiimote->link.s_ctrl) < 0) {
	wiimote_error("wiimote_disconnect(): close: %m");
	return WIIMOTE_ERROR;
    }
    
    wiimote->link.status = WIIMOTE_STATUS_DISCONNECTED;
    
    ba2str(BDADDR_ANY, wiimote->link.l_addr);
    ba2str(BDADDR_ANY, wiimote->link.r_addr);
    
    return WIIMOTE_OK;
}
