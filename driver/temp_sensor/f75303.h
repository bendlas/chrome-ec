/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* F75303 temperature sensor module for Chrome EC */

#ifndef __CROS_EC_F75303_H
#define __CROS_EC_F75303_H

#ifdef BOARD_MUSHU
#define F75303_I2C_ADDR_FLAGS 0x4D
#else
#define F75303_I2C_ADDR_FLAGS 0x4C
#endif

enum f75303_index {
	F75303_IDX_LOCAL = 0,
	F75303_IDX_REMOTE1,
	F75303_IDX_REMOTE2,
	F75303_IDX_COUNT,
};

/* F75303 register */
#define F75303_TEMP_LOCAL 0x00
#define F75303_TEMP_REMOTE1 0x01
#define F75303_TEMP_REMOTE2 0x23

/**
 * Get the last polled value of a sensor.
 *
 * @param idx	Index to read. Idx indicates whether to read die
 *		temperature or external temperature.
 * @param temp	Destination for temperature in K.
 *
 * @return EC_SUCCESS if successful, non-zero if error.
 */
int f75303_get_val(int idx, int *temp);

#endif /* __CROS_EC_F75303_H */
