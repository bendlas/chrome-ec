/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* I2C port module for Chrome EC */

#include "board.h"
#include "console.h"
#include "gpio.h"
#include "i2c.h"
#include "task.h"
#include "timer.h"
#include "registers.h"
#include "uart.h"
#include "util.h"

#define NUM_PORTS 6

#define LM4_I2C_MCS_RUN   (1 << 0)
#define LM4_I2C_MCS_START (1 << 1)
#define LM4_I2C_MCS_STOP  (1 << 2)
#define LM4_I2C_MCS_ACK   (1 << 3)
#define LM4_I2C_MCS_HS    (1 << 4)
#define LM4_I2C_MCS_QCMD  (1 << 5)

#define START 1
#define STOP  1
#define NO_START 0
#define NO_STOP  0

static task_id_t task_waiting_on_port[NUM_PORTS];
static struct mutex port_mutex[NUM_PORTS];

static int wait_idle(int port)
{
	int i;
	int event;

	i = LM4_I2C_MCS(port);
	while (i & 0x01) {
		/* Port is busy, so wait for the interrupt */
		task_waiting_on_port[port] = task_get_current();
		LM4_I2C_MIMR(port) = 0x03;
		event = task_wait_event(1000000);
		LM4_I2C_MIMR(port) = 0x00;
		task_waiting_on_port[port] = TASK_ID_INVALID;
		if (event == TASK_EVENT_TIMER)
			return EC_ERROR_TIMEOUT;

		i = LM4_I2C_MCS(port);
	}

	/* Check for errors */
	if (i & 0x02)
		return EC_ERROR_UNKNOWN;

	return EC_SUCCESS;
}

/* Transmit one block of raw data, then receive one block of raw data.
 * <start> flag indicates this smbus session start from idle state.
 * <stop>  flag means this session can be termicate with smbus stop bit
 */
static int i2c_transmit_receive(int port, int slave_addr,
		uint8_t *transmit_data, int transmit_size,
		uint8_t *receive_data, int receive_size,
		int start, int stop)
{
	int rv, i;
	int started = start ? 0 : 1;
	uint32_t reg_mcs;

	if (transmit_size == 0 && receive_size == 0)
		return EC_SUCCESS;

	if (transmit_data) {
		LM4_I2C_MSA(port) = slave_addr & 0xff;
		for (i = 0; i < transmit_size; i++) {
			LM4_I2C_MDR(port) = transmit_data[i];
			/* Setup master control/status register
			 * MCS sequence on multi-byte write:
			 *     0x3 0x1 0x1 ... 0x1 0x5
			 * Single byte write:
			 *     0x7
			 */
			reg_mcs = LM4_I2C_MCS_RUN;
			/* Set start bit on first byte */
			if (!started) {
				started = 1;
				reg_mcs |= LM4_I2C_MCS_START;
			}
			/* Send stop bit if the stop flag is on,
			 * and caller doesn't expect to receive
			 * data.
			 */
			if (stop && receive_size == 0 && i ==
					(transmit_size - 1))
				reg_mcs |= LM4_I2C_MCS_STOP;

			LM4_I2C_MCS(port) = reg_mcs;

			rv = wait_idle(port);
			if (rv)
				return rv;
		}
	}

	if (receive_size) {
		if (transmit_size)
			/* resend start bit when change direction */
			started = 0;

		LM4_I2C_MSA(port) = (slave_addr & 0xff) | 0x01;
		for (i = 0; i < receive_size; i++) {
			LM4_I2C_MDR(port) = receive_data[i];
			/* MCS receive sequence on multi-byte read:
			 *     0xb 0x9 0x9 ... 0x9 0x5
			 * Single byte read:
			 *     0x7
			 */
			reg_mcs = LM4_I2C_MCS_RUN;
			if (!started) {
				started = 1;
				reg_mcs |= LM4_I2C_MCS_START;
			}
			/* ACK all bytes except the last one */
			if (stop && i == (receive_size - 1))
				reg_mcs |= LM4_I2C_MCS_STOP;
			else
				reg_mcs |= LM4_I2C_MCS_ACK;

			LM4_I2C_MCS(port) = reg_mcs;
			rv = wait_idle(port);
			if (rv)
				return rv;
			receive_data[i] = LM4_I2C_MDR(port) & 0xff;
		}
	}

	return EC_SUCCESS;
}



int i2c_read16(int port, int slave_addr, int offset, int *data)
{
	int rv;
	uint8_t reg, buf[2];

	reg = offset & 0xff;
	/* I2C read 16-bit word:
	 * Transmit 8-bit offset, and read 16bits
	 */
	mutex_lock(port_mutex + port);
	rv = i2c_transmit_receive(port, slave_addr, &reg, 1, buf, 2,
					START, STOP);
	mutex_unlock(port_mutex + port);

	if (rv)
		return rv;

	if (slave_addr & I2C_FLAG_BIG_ENDIAN)
		*data = ((int)buf[0] << 8) | buf[1];
	else
		*data = ((int)buf[1] << 8) | buf[0];

	return EC_SUCCESS;
}


int i2c_write16(int port, int slave_addr, int offset, int data)
{
	int rv;
	uint8_t buf[3];

	buf[0] = offset & 0xff;

	if (slave_addr & I2C_FLAG_BIG_ENDIAN) {
		buf[1] = (data >> 8) & 0xff;
		buf[2] = data & 0xff;
	} else {
		buf[1] = data & 0xff;
		buf[2] = (data >> 8) & 0xff;
	}

	mutex_lock(port_mutex + port);
	rv = i2c_transmit_receive(port, slave_addr, buf, 3, 0, 0,
					START, STOP);
	mutex_unlock(port_mutex + port);

	return rv;
}

int i2c_read8(int port, int slave_addr, int offset, int* data)
{
	int rv;
	uint8_t reg, val;

	reg = offset;

	mutex_lock(port_mutex + port);
	rv = i2c_transmit_receive(port, slave_addr, &reg, 1, &val, 1,
					START, STOP);
	mutex_unlock(port_mutex + port);

	if (!rv)
		*data = val;

	return rv;
}

int i2c_write8(int port, int slave_addr, int offset, int data)
{
	int rv;
	uint8_t buf[2];

	buf[0] = offset;
	buf[1] = data;

	mutex_lock(port_mutex + port);
	rv = i2c_transmit_receive(port, slave_addr, buf, 2, 0, 0,
					START, STOP);
	mutex_unlock(port_mutex + port);

	return rv;
}

/* Read ascii string using smbus read block protocol.
 * The return data <data> will be null terminated.
 */
int i2c_read_string(int port, int slave_addr, int offset, uint8_t *data,
	int len)
{
	int rv;
	uint8_t reg, block_length;

	mutex_lock(port_mutex + port);

	reg = offset;
	/* Send device reg space offset, and read back block length.
	 * Keep this session open without a stop
	 */
	rv = i2c_transmit_receive(port, slave_addr, &reg, 1, &block_length, 1,
					START, NO_STOP);
	if (rv)
		goto exit;

	if (len && block_length > (len - 1))
		block_length = len - 1;

	rv = i2c_transmit_receive(port, slave_addr, 0, 0, data, block_length,
					NO_START, STOP);
	data[block_length] = 0;

exit:
	mutex_unlock(port_mutex + port);
	return rv;
}


/*****************************************************************************/
/* Interrupt handlers */

/* Handles an interrupt on the specified port. */
static void handle_interrupt(int port)
{
	int id = task_waiting_on_port[port];

	/* Clear the interrupt status*/
	LM4_I2C_MICR(port) = LM4_I2C_MMIS(port);

	/* Wake up the task which was waiting on the interrupt, if any */
	/* TODO: set event based on I2C port number? */
	if (id != TASK_ID_INVALID)
		task_wake(id);
}


static void i2c0_interrupt(void) { handle_interrupt(0); }
static void i2c1_interrupt(void) { handle_interrupt(1); }
static void i2c2_interrupt(void) { handle_interrupt(2); }
static void i2c3_interrupt(void) { handle_interrupt(3); }
static void i2c4_interrupt(void) { handle_interrupt(4); }
static void i2c5_interrupt(void) { handle_interrupt(5); }

DECLARE_IRQ(LM4_IRQ_I2C0, i2c0_interrupt, 2);
DECLARE_IRQ(LM4_IRQ_I2C1, i2c1_interrupt, 2);
DECLARE_IRQ(LM4_IRQ_I2C2, i2c2_interrupt, 2);
DECLARE_IRQ(LM4_IRQ_I2C3, i2c3_interrupt, 2);
DECLARE_IRQ(LM4_IRQ_I2C4, i2c4_interrupt, 2);
DECLARE_IRQ(LM4_IRQ_I2C5, i2c5_interrupt, 2);

/*****************************************************************************/
/* Console commands */


static void scan_bus(int port, char *desc)
{
	int rv;
	int a;

	uart_printf("Scanning %s I2C bus (%d)...\n", desc, port);

	mutex_lock(port_mutex + port);

	for (a = 0; a < 0x100; a += 2) {
		uart_puts(".");

		/* Do a single read */
		LM4_I2C_MSA(port) = a | 0x01;
		LM4_I2C_MCS(port) = 0x07;
		rv = wait_idle(port);
		if (rv == EC_SUCCESS)
			uart_printf("\nFound device at 8-bit addr 0x%02x\n", a);
}

	mutex_unlock(port_mutex + port);

	uart_puts("\n");
}


static int command_i2cread(int argc, char **argv)
{
	int port, addr, count = 1;
	char *e;
	int rv;
	int d, i;

	if (argc < 3) {
		uart_puts("Usage: i2cread <port> <addr> [count]\n");
		return EC_ERROR_UNKNOWN;
	}

	port = strtoi(argv[1], &e, 0);
	if (*e) {
		uart_puts("Invalid port\n");
		return EC_ERROR_INVAL;
	}
	if (port != I2C_PORT_THERMAL && port != I2C_PORT_BATTERY &&
	    port != I2C_PORT_CHARGER) {
		uart_puts("Unsupported port\n");
		return EC_ERROR_UNKNOWN;
	}

	addr = strtoi(argv[2], &e, 0);
	if (*e || (addr & 0x01)) {
		uart_puts("Invalid addr; try 'i2cscan' command\n");
		return EC_ERROR_INVAL;
	}

	if (argc > 3) {
		count = strtoi(argv[3], &e, 0);
		if (*e) {
			uart_puts("Invalid count\n");
			return EC_ERROR_INVAL;
		}
	}

	uart_printf("Reading %d bytes from I2C device %d:0x%02x...\n",
		    count, port, addr);
	mutex_lock(port_mutex + port);
	LM4_I2C_MSA(port) = addr | 0x01;
	for (i = 0; i < count; i++) {
		if (i == 0)
			LM4_I2C_MCS(port) = (count > 1 ? 0x0b : 0x07);
		else
			LM4_I2C_MCS(port) = (i == count - 1 ? 0x05 : 0x09);
		rv = wait_idle(port);
		if (rv != EC_SUCCESS) {
			mutex_unlock(port_mutex + port);
			return rv;
		}
		d = LM4_I2C_MDR(port) & 0xff;
		uart_printf("0x%02x ", d);
	}
	mutex_unlock(port_mutex + port);
	uart_puts("\n");
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(i2cread, command_i2cread);


static int command_scan(int argc, char **argv)
{
	scan_bus(I2C_PORT_THERMAL, "thermal");
	scan_bus(I2C_PORT_BATTERY, "battery");
	scan_bus(I2C_PORT_CHARGER, "charger");
	uart_puts("done.\n");
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(i2cscan, command_scan);


/*****************************************************************************/
/* Initialization */

/* Configures GPIOs for the module. */
static void configure_gpio(void)
{
#ifdef BOARD_link
	/* PA6:7 = I2C1 SCL/SDA; PB2:3 = I2C0 SCL/SDA; PB6:7 = I2C5 SCL/SDA */
	gpio_set_alternate_function(LM4_GPIO_A, 0xc0, 3);
	gpio_set_alternate_function(LM4_GPIO_B, 0xcc, 3);

	/* Configure SDA as open-drain.  SCL should not be open-drain,
	 * since it has an internal pull-up. */
	LM4_GPIO_ODR(LM4_GPIO_A) |= 0x80;
	LM4_GPIO_ODR(LM4_GPIO_B) |= 0x88;
#else
	/* PG6:7 = I2C5 SCL/SDA */
	gpio_set_alternate_function(LM4_GPIO_G, 0xc0, 3);

	/* Configure SDA as open-drain.  SCL should not be open-drain,
	 * since it has an internal pull-up. */
	LM4_GPIO_ODR(LM4_GPIO_G) |= 0x80;
#endif
}


int i2c_init(void)
{
	volatile uint32_t scratch  __attribute__((unused));
	int i;

	/* Enable I2C modules and delay a few clocks */
	LM4_SYSTEM_RCGCI2C |= (1 << I2C_PORT_THERMAL) |
		(1 << I2C_PORT_BATTERY) | (1 << I2C_PORT_CHARGER) |
		(1 << I2C_PORT_LIGHTBAR);
	scratch = LM4_SYSTEM_RCGCI2C;

	/* Configure GPIOs */
	configure_gpio();

	/* No tasks are waiting on ports */
	for (i = 0; i < NUM_PORTS; i++)
		task_waiting_on_port[i] = TASK_ID_INVALID;

	/* Initialize ports as master, with interrupts enabled */
	LM4_I2C_MCR(I2C_PORT_THERMAL) = 0x10;
	LM4_I2C_MTPR(I2C_PORT_THERMAL) =
		(CPU_CLOCK / (I2C_SPEED_THERMAL * 10 * 2)) - 1;

	LM4_I2C_MCR(I2C_PORT_BATTERY) = 0x10;
	LM4_I2C_MTPR(I2C_PORT_BATTERY) =
		(CPU_CLOCK / (I2C_SPEED_BATTERY * 10 * 2)) - 1;

	LM4_I2C_MCR(I2C_PORT_CHARGER) = 0x10;
	LM4_I2C_MTPR(I2C_PORT_CHARGER) =
		(CPU_CLOCK / (I2C_SPEED_CHARGER * 10 * 2)) - 1;

	LM4_I2C_MCR(I2C_PORT_LIGHTBAR) = 0x10;
	LM4_I2C_MTPR(I2C_PORT_LIGHTBAR) =
		(CPU_CLOCK / (I2C_SPEED_LIGHTBAR * 10 * 2)) - 1;

	/* Enable irqs */
	task_enable_irq(LM4_IRQ_I2C0);
	task_enable_irq(LM4_IRQ_I2C1);
	task_enable_irq(LM4_IRQ_I2C2);
	task_enable_irq(LM4_IRQ_I2C3);
	task_enable_irq(LM4_IRQ_I2C4);
	task_enable_irq(LM4_IRQ_I2C5);

	return EC_SUCCESS;
}
