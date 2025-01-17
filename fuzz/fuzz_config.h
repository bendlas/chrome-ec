/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Fuzzer target config flags */

#ifndef __FUZZ_FUZZ_CONFIG_H
#define __FUZZ_FUZZ_CONFIG_H
#ifdef TEST_FUZZ

/* Disable hibernate: We never want to exit while fuzzing. */
#undef CONFIG_HIBERNATE

#ifdef TEST_HOST_COMMAND_FUZZ
#undef CONFIG_HOSTCMD_DEBUG_MODE

/* Defining this makes fuzzing slower, but exercises additional code paths. */
#define FUZZ_HOSTCMD_VERBOSE

#ifdef FUZZ_HOSTCMD_VERBOSE
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_PARAMS
#else
#define CONFIG_HOSTCMD_DEBUG_MODE HCDEBUG_OFF
#endif /* ! FUZZ_HOSTCMD_VERBOSE */

/* The following are for fpsensor host commands. */
#define CONFIG_AES
#define CONFIG_AES_GCM
#define CONFIG_ROLLBACK_SECRET_SIZE 32
#define CONFIG_SHA256

#endif /* TEST_HOST_COMMAND_FUZZ */

#ifdef TEST_USB_PD_FUZZ
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_TCPMV1
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_SHA256
#define CONFIG_SW_CRC
#endif /* TEST_USB_PD_FUZZ */

#ifdef TEST_USB_TCPM_V2_REV30_FUZZ
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_USB_PD_TCPC_LOW_POWER
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_PID 0x5555
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PRL_SM
#define CONFIG_USB_PD_REV30
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_DECODE_SOP
#define CONFIG_USB_DRP_ACC_TRYSRC
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USBC_SS_MUX
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_USBC_VCONN_SWAP_DELAY_US 5000
#define CONFIG_SHA256
#define CONFIG_SW_CRC
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#endif /* TEST_USB_TCPM_V2_REV30_FUZZ */

#ifdef TEST_USB_TCPM_V2_REV20_FUZZ
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_PORT_MAX_COUNT 2
#define CONFIG_USB_PD_TCPC_LOW_POWER
#define CONFIG_USB_PD_TRY_SRC
#define CONFIG_USB_PID 0x5555
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PRL_SM
#define CONFIG_USB_PD_TCPMV2
#define CONFIG_USB_PD_DECODE_SOP
#define CONFIG_USB_DRP_ACC_TRYSRC
#define CONFIG_USB_PD_ALT_MODE_DFP
#define CONFIG_USBC_SS_MUX
#define CONFIG_USBC_VCONN
#define CONFIG_USBC_VCONN_SWAP
#define CONFIG_USBC_VCONN_SWAP_DELAY_US 5000
#define CONFIG_SHA256
#define CONFIG_SW_CRC
#define CONFIG_USB_PD_3A_PORTS 0 /* Host does not define a 3.0 A PDO */
#endif /* TEST_USB_TCPM_V2_REV20_FUZZ */

#ifdef TEST_PCHG_FUZZ
#define CONFIG_CTN730
#define CONFIG_DEVICE_EVENT
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
#define CONFIG_PERIPHERAL_CHARGER
#define I2C_PORT_WLC 0
#define GPIO_WLC_IRQ_CONN 1
#define GPIO_WLC_NRST_CONN 2
#define GPIO_PCHG_P0 GPIO_WLC_IRQ_CONN
#endif /* TEST_PCHG_FUZZ */

#endif /* TEST_FUZZ */
#endif /* __FUZZ_FUZZ_CONFIG_H */
