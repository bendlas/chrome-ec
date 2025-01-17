/* Copyright 2022 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Intel-RVP family-specific configuration */

#include "console.h"
#include "gpio/gpio_int.h"
#include "hooks.h"
#include "include/gpio.h"
#include "intelrvp.h"
#include "ioexpander.h"
#include "system.h"
#include "tcpm/tcpci.h"
#include "usbc_ppc.h"

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ##args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ##args)

void tcpc_alert_event(enum gpio_signal signal)
{
	int i;

	for (i = 0; i < CONFIG_USB_PD_PORT_MAX_COUNT; i++) {
		/* No alerts for embedded TCPC */
		if (tcpc_config[i].bus_type == EC_BUS_TYPE_EMBEDDED) {
			continue;
		}

		if (signal == tcpc_aic_gpios[i].tcpc_alert) {
			schedule_deferred_pd_interrupt(i);
			break;
		}
	}
}

uint16_t tcpc_get_alert_status(void)
{
	uint16_t status = 0;
	int i;

	/* Check which port has the ALERT line set */
	for (i = 0; i < CONFIG_USB_PD_PORT_MAX_COUNT; i++) {
		/* No alerts for embdeded TCPC */
		if (tcpc_config[i].bus_type == EC_BUS_TYPE_EMBEDDED) {
			continue;
		}

		if (!gpio_get_level(tcpc_aic_gpios[i].tcpc_alert)) {
			status |= PD_STATUS_TCPC_ALERT_0 << i;
		}
	}

	return status;
}

int ppc_get_alert_status(int port)
{
	return tcpc_aic_gpios[port].ppc_intr_handler &&
	       !gpio_get_level(tcpc_aic_gpios[port].ppc_alert);
}

/* PPC support routines */
void ppc_interrupt(enum gpio_signal signal)
{
	int i;

	for (i = 0; i < CONFIG_USB_PD_PORT_MAX_COUNT; i++) {
		if (tcpc_aic_gpios[i].ppc_intr_handler &&
		    signal == tcpc_aic_gpios[i].ppc_alert) {
			tcpc_aic_gpios[i].ppc_intr_handler(i);
			break;
		}
	}
}

void board_charging_enable(int port, int enable)
{
	int rv;

	if (tcpc_aic_gpios[port].ppc_intr_handler) {
		rv = ppc_vbus_sink_enable(port, enable);
	} else {
		rv = tcpc_config[port].drv->set_snk_ctrl(port, enable);
	}

	if (rv) {
		CPRINTS("C%d: sink path %s failed", port,
			enable ? "en" : "dis");
	}
}
