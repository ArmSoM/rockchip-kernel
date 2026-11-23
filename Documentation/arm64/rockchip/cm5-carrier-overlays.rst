ArmSoM CM5 carrier overlays
==========================

Hardware reference: CM5 schematic V1.3 and CM5-IO schematic V1.1.
Pin numbers in overlay Help refer to module connector CN1, not the 40-pin header.

Official CM5-IO
--------------

Keep rk3576-armsom-cm5-io.dtb and uEnvarmsom-cm5-io.txt. Existing fixed
devices and boot configuration are unchanged; every additional overlay is
commented out. The eight module-connector options reuse optional display or
camera pins. Disconnect the relevant accessory before enabling an alternative.
The menu separates 40PIN peripherals, Display, Cameras and Module connector
(CN1) using display-only ``# menu-group:`` comments in uEnv. Install the updated
generic armsom-overlay executable once to display these headings; older tools
still read the same overlay selections but display a flat list. No per-carrier
menu code or JSON is needed. Use Help for pin numbers and requirements, save,
then reboot.
PWM selection enables the controller, not an output waveform.

The menu rejects mutually exclusive selections; it does not detect connected
hardware. Manual uEnv edits bypass that check. Do not enable alternative muxes
for one controller simultaneously, even when their GPIOs differ.

Custom carrier
--------------

rk3576-armsom-cm5-custom-io.dts is an opt-in starting point. It inherits the
CM5-IO supply topology and retains module networking, storage and HDMI. It
disables the official audio codec/card, fan, LED, Type-C/DP, USB host power and
PCIe consumers before exposing UART7, I2C8, UART8 and audio-pin PWM alternatives.
It is NOT a complete description of an arbitrary customer PCB.

Review supplies, SD-card wiring, boot medium, clocks and all external devices
against the customer's schematic. Add a carrier-specific compatible, DTS and
uEnv when shipping a product. Never choose this template merely to unlock
extra pins on the official CM5-IO PCB.

For development, use the existing CM5 kernel-6.1 configuration, select the
custom DTS as RK_KERNEL_DTS in the SDK kernel configuration and rebuild the
kernel. Update the kernel/DTB through the normal SDK boot-image or kernel-package
flow; verify that the active DTB is the custom DTB and that /boot/uEnv/uEnv.txt
resolves to uEnvarmsom-cm5-custom-io.txt. Changing only the uEnv symlink does NOT
change the base DTB. The menu rejects custom-only overlays on the official DTB.
The Debian 12 / Ubuntu 22.04 first-boot scripts include a generic fallback:
an otherwise unknown ArmSoM board is accepted only if its matching DTB and uEnv
files exist. Existing board mappings take precedence. A fresh custom-carrier
image needs this first-boot update; an official CM5-IO image does not need a
filesystem rebuild for the eight additional menu entries. Future carriers
following this naming convention need no per-board first-boot edits.
No extra filesystem-side JSON or per-carrier Python code is required.

Electrical limitations
----------------------

Camera I2C and some control pins pass through the module's level translator;
validate baud rates and edge quality on the actual PCB. CAN requires an external
transceiver and appropriate termination. I2C needs suitable pull-ups. DSI/CSI,
HDMI and USB differential pairs are not ordinary GPIO expansion pins. SPI0 M0
is not offered because its MISO signal is not exposed by this module revision.
UART4, I2C2 and other module-internal resources remain reserved for their fixed
devices. SD/ADC/debug signals are not repurposed by this change.

Validation scope
----------------

DTB/DTBO compilation and static overlay-merge/conflict checks do not prove
external signal quality. Newly added muxes require electrical and peripheral
tests on the intended carrier before being advertised as hardware-validated.
