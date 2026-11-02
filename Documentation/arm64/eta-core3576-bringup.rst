ETA-CORE3576 bring-up status
===========================

Board: ETA-CORE3576 V1.0, RK3576, based on the ArmSoM Sige5 device tree.
Source schematic: ETA-CORE3576-1V0-Sch.pdf.

Build and boot
--------------

Select an ETA-CORE3576 SDK defconfig, then run ``./build.sh kernel-6.1``.
ETA SDK defconfigs must include ``eta_core3576_network.config`` in
``RK_KERNEL_CFG_FRAGMENTS``. The SDK defconfigs and rootfs board-detection
changes live outside this kernel repository and are not included here.

The kernel package post-install hook maps ``eta,core3576`` to
``rk3576-eta-core3576.dtb`` and ``uEnveta-core3576.txt``. Kernel and header
Debian packages were built, installed and booted through version #236 on
2026-11-02. No rootfs rebuild was required for this network bring-up.

Wi-Fi
-----

The SDIO device 02d0:aae8 is BCM43752. The board uses the SDK ap6275s
firmware profile with fw_bcm43752a2_ag.bin and nvram_ap6275s.txt. These
firmware files must already exist in /lib/firmware on the root filesystem.
WL_REG_ON is controlled by the SDIO power sequence; the WLAN platform
node supplies the host-wake interrupt configuration.

Verified: 2.4 GHz and 5 GHz scans; 5 GHz association; DHCP; gateway and
Internet ICMP; DNS/HTTPS; automatic reconnection after reboot. No network
credentials are stored in this repository.

Ethernet: hardware correction required
-------------------------------------

Both RTL8363NB-VB switches identify as 6511/0060. Their copper PHYs are
ports 1 and 3; the CPU RGMII port is 6. Separate DSA tree IDs are required.
The driver changes provide chip detection, PHY OCP access and initial
switch configuration; they are bring-up support, not a verified Ethernet
data path. Existing chip initialization tables are retained.

RJ45-P4 is U21 copper port 3, exposed as lan3 on the second GMAC. The
copper link negotiated 1000 Mbps full duplex, but the SoC reports DMA
reset failure. The V1.0 schematic connects RGMII TX to TX and RX to RX.
Realtek RTL8363NB-VB Rev1.2, section 10.6.1, Figure 21 (printed page 42)
requires crossing the CPU MAC and switch MAC signals:

======================= ===============================
RK3576 GMAC1 signal     Required U21 signal / pin
======================= ===============================
TXCLK                   RG1_RXCLK / 48
RXCLK                   RG1_TXCLK / 47
TXCTL                   RG1_RXCTL / 49
RXCTL                   RG1_TXCTL / 46
TXD0, TXD1, TXD2, TXD3   RG1_RXD0..3 / 50, 51, 52, 53
RXD0, RXD1, RXD2, RXD3   RG1_TXD0..3 / 45, 44, 43, 42
======================= ===============================

Confirm actual PCB connectivity and correct the entire RGMII bus before
continuing DHCP, packet transfer, delay tuning or throughput validation.
The Ethernet data path has not passed testing. Other interfaces retain
initial schematic-based configuration unless separately verified.
