/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_INCLUDE_VBOOT_H
#define __CROS_EC_INCLUDE_VBOOT_H

#include "common.h"
#include "vb21_struct.h"
#include "rsa.h"
#include "sha256.h"
#include "stdbool.h"
#include "timer.h"

/**
 * Validate key contents.
 *
 * @param key
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vb21_is_packed_key_valid(const struct vb21_packed_key *key);

/**
 * Validate signature contents.
 *
 * @param sig Signature to be validated.
 * @param key Key to be used for validating <sig>.
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vb21_is_signature_valid(const struct vb21_signature *sig,
			    const struct vb21_packed_key *key);

/**
 * Returns the public key in RO that was used to sign RW.
 *
 * @return pointer to key, never NULL
 */
const struct vb21_packed_key *vb21_get_packed_key(void);

/**
 * Check data region is filled with ones
 *
 * @param data  Data to be validated.
 * @param start Offset where validation starts.
 * @param end   Offset where validation ends. data[end] won't be checked.
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vboot_is_padding_valid(const uint8_t *data, uint32_t start, uint32_t end);

/**
 * Verify data by RSA signature
 *
 * @param data Data to be verified.
 * @param len  Number of bytes in <data>.
 * @param key  Key to be used for verification.
 * @param sig  Signature of <data>
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vboot_verify(const uint8_t *data, int len, const struct rsa_public_key *key,
		 const uint8_t *sig);

/**
 * Entry point of EC EFS
 */
void vboot_main(void);

/**
 * Get if vboot requires PD comm to be enabled or not
 *
 * @return 1: need PD communication. 0: PD communication is not needed.
 */
int vboot_need_pd_comm(void);

/**
 * Callback for boards to notify users of vboot error when no display is
 * available.
 *
 * Typically this happens when a Chromebox is booting on a Type-C adapter and
 * EFS failed.
 */
__override_proto void show_critical_error(void);

/**
 * Callback for boards to notify the user of power shortage.
 */
__override_proto void show_power_shortage(void);

/*
 * Board level packet mode enable function.
 */
__override_proto void board_enable_packet_mode(bool enable);

/**
 * Interrupt handler for packet mode entry.
 *
 * @param signal	GPIO id for packet mode interrupt pin.
 */
void packet_mode_interrupt(enum gpio_signal signal);

/* Maximum number of times EC retries packet transmission before giving up. */
#define CR50_COMM_MAX_RETRY 5

/* EC's timeout for packet transmission to Cr50. */
#define CR50_COMM_TIMEOUT (50 * MSEC)

/* Preamble character repeated before the packet header starts. */
#define CR50_COMM_PREAMBLE 0xec

/* Magic characters used to identify ec-cr50-comm packets */
#define CR50_PACKET_MAGIC 0x4345 /* 'EC' in little endian */

/* version of struct cr50_comm_request */
#define CR50_COMM_PACKET_VERSION (0 << 4 | 0 << 0) /* 0.0 */

/**
 * EC-Cr50 data frame looks like the following:
 *
 *   [preamble][header][payload]
 *
 * preamble: 0xec ...
 * header: struct cr50_comm_request
 * payload: data[]
 */
struct cr50_comm_request {
	/* Header */
	uint16_t magic; /* CR50_PACKET_MAGIC */
	uint8_t struct_version; /* version of this struct msb:lsb=major:minor */
	uint8_t crc; /* checksum computed from all bytes after crc */
	uint16_t type; /* CR50_CMD_* */
	uint8_t size; /* Payload size. Be easy on Cr50 buffer. */
	/* Payload */
	uint8_t data[];
} __packed;

struct cr50_comm_response {
	uint16_t error;
} __packed;

#define CR50_COMM_MAX_REQUEST_SIZE \
	(sizeof(struct cr50_comm_request) + UINT8_MAX)
#define CR50_UART_RX_BUFFER_SIZE 32 /* TODO: Get from Cr50 header */

/* commands */
enum cr50_comm_cmd {
	CR50_COMM_CMD_HELLO = 0x0000,
	CR50_COMM_CMD_SET_BOOT_MODE = 0x0001,
	CR50_COMM_CMD_VERIFY_HASH = 0x0002,
	CR50_COMM_CMD_LIMIT = 0xffff,
} __packed;
BUILD_ASSERT(sizeof(enum cr50_comm_cmd) == sizeof(uint16_t));

#define CR50_COMM_ERR_PREFIX 0xec

/* return code */
enum cr50_comm_err {
	CR50_COMM_SUCCESS = 0xec00,
	CR50_COMM_ERR_UNKNOWN = 0xec01,
	CR50_COMM_ERR_MAGIC = 0xec02,
	CR50_COMM_ERR_CRC = 0xec03,
	CR50_COMM_ERR_SIZE = 0xec04,
	CR50_COMM_ERR_TIMEOUT = 0xec05, /* Generated by EC */
	CR50_COMM_ERR_UNDEFINED_CMD = 0xec06,
	CR50_COMM_ERR_BAD_PAYLOAD = 0xec07,
	CR50_COMM_ERR_STRUCT_VERSION = 0xec08,
	CR50_COMM_ERR_NVMEM = 0xec09,
} __packed;
BUILD_ASSERT(sizeof(enum cr50_comm_err) == sizeof(uint16_t));

/*
 * BIT(1) : NO_BOOT flag
 * BIT(0) : RECOVERY flag
 */
enum boot_mode {
	BOOT_MODE_NORMAL = 0x00,
	BOOT_MODE_NO_BOOT = 0x01,
} __packed;
BUILD_ASSERT(sizeof(enum boot_mode) == sizeof(uint8_t));

/**
 * Indicate PD is allowed (in RO) by vboot or not.
 *
 * Overridden by each EFS implementation (EFS1 and EFS2) not by boards.
 *
 * @return true - allowed. false - disallowed.
 */
__override_proto bool vboot_allow_usb_pd(void);

#ifdef TEST_BUILD
/**
 * Set the vboot_allow_usb_pd flag to false.
 */
__test_only void vboot_disable_pd(void);
#endif

#endif /* __CROS_EC_INCLUDE_VBOOT_H */
