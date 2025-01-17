# Zephyr Troubleshooting

[TOC]

## Devicetree

The devicetree is made out of different dts and dtsi files, gets aggregated
into a single `zephyr.dts` file and generates a `devicetree_generated.h` header
with all the definitions used by the `DT_` macros.

The build system lists the various overlay files specified by `BUILD.py`, for
example:

```
-- Found devicetree overlay: /mnt/host/source/src/platform/ec/zephyr/projects/brya/adc.dts
-- Found devicetree overlay: /mnt/host/source/src/platform/ec/zephyr/projects/brya/battery.dts
-- Found devicetree overlay: /mnt/host/source/src/platform/ec/zephyr/projects/brya/cbi_eeprom.dts
-- Found devicetree overlay: /mnt/host/source/src/platform/ec/zephyr/projects/brya/fan.dts
...
```

Useful artifacts (always present):

Aggregated devicetree file, after all the overlays, preprocessor and
`gen_defines.py`:

```
build/zephyr/$PROJECT/build-ro/zephyr/zephyr.dts
```

Main devicetree output, flat representation of the tree and various node
references, including ordinals of `dts_ord_...` structs:

```
./build/zephyr/$PROJECT/build-ro/zephyr/include/generated/devicetree_generated.h
```

For more details see: [CMake configuration phase](https://docs.zephyrproject.org/latest/build/cmake/index.html?highlight=gen_defines%20py#configuration-phase)

## Node nomenclature

```
/ {
        a-node {
                subnode_nodelabel: a-sub-node {
                        foo = <3>;
                        label = "SUBNODE";
                };
        };
};
```

- `/` is the root node
- `a-node` and `a-sub-node` are node names
- `subnode_nodelabel` is a nodelabel
- `foo` is a property, `3` is the value
- `label` is a property, `SUBNODE` is the value

NOTE: `subnode_nodelabel` is a nodelabel, `label` is a label property.

## Adding multiple nodelabels

Code can have hardcoded nodelables, so sometimes it's useful to add extra
nodelabels to an existing node (referenced by another nodelabel). To do that
add an overlay with something similar to:

```
another_node_label: &subnode_nodelabel {
};
```

## Undefined reference to \_\_device\_dts\_ord\_...

This happens when some code refer to a device using `DT_DEVICE_GET`, but the
corresponding `struct device` is not instantiated, either because the driver
has not been enabled or because of a devicetree misconfiguration (missing
`status = "okay"`).

Quick fix: enable the corresponding driver (CONFIG_...=y) or fix the devicetree.

Proper fix: find the code referencing to the undefined node, make sure that the
corresponding Kconfig option depends on the subsystem being enabled (ADC,
I2C...), make sure that the specific platform driver is enabled based on the
devicetree (`default y` and `depends on DT_HAS_...`).

## error: 'CONFIG_..._LOG_LEVEL' undeclared

The `CONFIG_..._LOG_LEVEL` symbols are not defined directly (i.e. there's no
Kconfig `config ..._LOG_LEVEL`), they are generated using the
`subsys/logging/Kconfig.template.log_config` template.

Quick fix: enable the logging subsystem (normally `CONFIG_LOG=y`
`CONFIG_LOG_MODE_MINIMAL=y` in the project `prj.conf`), or change the code so
that the driver builds without this config.

Fix: make the driver depends on the logging subsystem being enabled (`depends
on LOG`) or change the code to compile with `CONFIG_LOG=n`.

## Menuconfig

Sometimes it's useful to run the `menuconfig` target on a specific project,
this can be done with:

```
ninja -C build/zephyr/$PROJECT/build-ro menuconfig
```

This exposes all the available options from the various Kconfig fragments and
can be useful to validate that config constraints are working correctly.

For example, searching for `^SSHELL$` (using the `/` key) shows:

```
Name: SHELL
Prompt: Shell
Type: bool
Value: y

Symbols currently n-selecting this symbol (no effect):
...

Symbols currently y-implying this symbol:
  - CROS_EC
  - PLATFORM_EC
```

## LTO

Many compiler and linker error are very uninformative if LTO is enabled, for
example a missing `struct device` can show as

```
/tmp/ccCiGy7c.ltrans0.ltrans.o:(.rodata+0x6a0): undefined reference to `__device_dts_ord_75'
```

Adding `CONFIG_LTO=n` to the corresponding `prj.conf` usually results in more
useful error messages, for example:

```
modules/ec/libec_shim.a(adc.c.obj):(.rodata.adc_channels+0x58): undefined reference to `__device_dts_ord_75'
```

## Build artifacts

The buildsystem can be configured to leave the build artifact next to the
object files, this is useful to inspect the macro output. To do that use the
`zmake` flag:

```
zmake build $PROJECT --extra-cflags=-save-temps=obj
```

This leaves a bunch of `.i` files in the build/ directory.

For more information see: [Look at the preprocessor output](https://docs.zephyrproject.org/latest/build/dts/troubleshooting.html?highlight=save%20temps#look-at-the-preprocessor-output).

This is also useful to analyze assembly errors, for example

```
/tmp/cctFuB4N.s: Assembler messages:
/tmp/cctFuB4N.s:1869: Error: missing expression
```

becomes

```
zephyr/CMakeFiles/zephyr.dir/misc/generated/configs.c.s: Assembler messages:
zephyr/CMakeFiles/zephyr.dir/misc/generated/configs.c.s:1869: Error: missing expression
```

## Statically initialized objects

The `zephyr.elf` output file can be used with gdb to analyze all the statically
allocated structures, for example:

```
$ arm-none-eabi-gdb build/zephyr/$PROJECT/output/zephyr-ro.elf
(gdb) p fan_config
$1 = {{pwm = {dev = 0x100ad244 <__device_dts_ord_169>, channel = 0, period = 1000000, flags = 0}, tach = 0x100ad43c <__device_dts_ord_172>}}
(gdb) p __device_dts_ord_172.name
$3 = 0x100ba480 "tach@400e1000"
```

If the symbol has been optimized, try rebuilding with `CONFIG_LTO=n`.
