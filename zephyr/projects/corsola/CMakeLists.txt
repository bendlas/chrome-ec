# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS "${ZEPHYR_BASE}")

cros_ec_library_include_directories(include)

# Include selected EC source from the baseboard
zephyr_library_sources(
	"src/board.c"
	"src/board_chipset.c"
	"src/hibernate.c"
)

zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC "src/usbc_config.c")
zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC "src/usb_pd_policy.c")
zephyr_library_sources_ifdef(CONFIG_VARIANT_CORSOLA_DB_DETECTION
			     "src/variant_db_detection.c")

if(DEFINED CONFIG_BOARD_KRABBY)
	project(krabby)
	zephyr_library_sources("src/krabby/hooks.c"
			       "src/krabby/charger_workaround.c"
			       "src/krabby/ppc_krabby.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_I2C "src/krabby/i2c.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/krabby/usb_pd_policy.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/krabby/usbc_config.c")
elseif(DEFINED CONFIG_BOARD_KINGLER)
	project(kingler)
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_I2C "src/kingler/i2c.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_LED_COMMON
				     "src/kingler/led.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/kingler/usb_pd_policy.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/kingler/usbc_config.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_BUTTONS_RUNTIME_CONFIG
				     "src/kingler/button.c")
elseif(DEFINED CONFIG_BOARD_STEELIX)
	project(steelix)
	zephyr_library_sources("src/kingler/board_steelix.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_I2C "src/kingler/i2c.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_LED_COMMON
				     "src/kingler/led_steelix.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/kingler/usb_pd_policy.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/kingler/usbc_config.c")

elseif(DEFINED CONFIG_BOARD_TENTACRUEL)
	project(tentacruel)
	zephyr_library_sources("src/krabby/hooks.c"
				"src/krabby/charger_workaround.c"
				"src/krabby/sensor_tentacruel.c"
				"src/krabby/temp_tentacruel.c"
				"src/krabby/ppc_tentacruel.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_I2C "src/krabby/i2c.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				"src/krabby/usb_pd_policy.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				"src/krabby/usbc_config.c")

elseif(DEFINED CONFIG_BOARD_MAGIKARP)
	project(magikarp)
	zephyr_library_sources("src/krabby/hooks.c"
				"src/krabby/sensor_magikarp.c"
				"src/krabby/ppc_magikarp.c"
				"src/krabby/keyboard_magikarp.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_I2C "src/krabby/i2c.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/krabby/usb_pd_policy.c")
	zephyr_library_sources_ifdef(CONFIG_PLATFORM_EC_USBC
				     "src/krabby/usbc_config.c")

endif()

