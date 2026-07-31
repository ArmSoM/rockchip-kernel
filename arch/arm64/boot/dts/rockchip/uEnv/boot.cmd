echo [boot.cmd] run boot.cmd scripts ...;

setenv initrd_load
setenv kernel_load

if test -e ${devtype} ${devnum}:${distro_bootpart} /uEnv/uEnv.txt; then

    echo [boot.cmd] load ${devtype} ${devnum}:${distro_bootpart} ${env_addr_r} /uEnv/uEnv.txt ...;
    load ${devtype} ${devnum}:${distro_bootpart} ${env_addr_r} /uEnv/uEnv.txt;

    echo [boot.cmd] Importing environment from ${devtype} ...
    env import -t ${env_addr_r} 0x8000

    part number ${devtype} ${devnum} "rootfs" rootfs_part
    setenv bootargs ${bootargs} root=/dev/mmcblk${devnum}p${rootfs_part} boot_part=${distro_bootpart} ${cmdline}
    printenv bootargs

    echo "[boot.cmd] try load ${devtype} ${devnum}:${distro_bootpart} ${ramdisk_addr_r} /initrd ..."
    if load ${devtype} ${devnum}:${distro_bootpart} ${ramdisk_addr_r} /initrd; then
        setenv initrd_size ${filesize}
        setenv initrd_load yes
    else
        echo "[boot.cmd] try load ${devtype} ${devnum}:${distro_bootpart} ${ramdisk_addr_r} /initrd-${uname_r} ..."
        if load ${devtype} ${devnum}:${distro_bootpart} ${ramdisk_addr_r} /initrd-${uname_r}; then
            setenv initrd_size ${filesize}
            setenv initrd_load yes
        fi
    fi

    echo "[boot.cmd] try load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /Image ..."
    if load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /Image; then
        setenv kernel_load yes
    else
        echo "[boot.cmd] try load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /Image-${uname_r} ..."
        if load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} /Image-${uname_r}; then
            setenv kernel_load yes
        fi
    fi

    echo [boot.cmd] loading default rk-kernel.dtb
    load ${devtype} ${devnum}:${distro_bootpart} ${fdt_addr_r} /rk-kernel.dtb

    fdt addr  ${fdt_addr_r}
    fdt set /chosen bootargs

    setenv dev_bootpart ${devnum}:${distro_bootpart}

    if test "${enable_uboot_overlays}" = "1"; then
        echo [boot.cmd] dtoverlay from /uEnv/uEnv.txt
        dtfile ${fdt_addr_r} ${fdt_over_addr}  /uEnv/uEnv.txt ${env_addr_r}
    fi

    echo [boot.cmd] [${devtype} ${devnum}:${distro_bootpart}] ...
    if test "${initrd_load}" = "yes"; then
        echo [boot.cmd] booti ${kernel_addr_r} ${ramdisk_addr_r}:${initrd_size} ${fdt_addr_r} ...
        booti ${kernel_addr_r} ${ramdisk_addr_r}:${initrd_size} ${fdt_addr_r}
    else
        echo [boot.cmd] booti ${kernel_addr_r} - ${fdt_addr_r} ...
        booti ${kernel_addr_r} - ${fdt_addr_r}
    fi
fi

echo [boot.cmd] run boot.cmd scripts failed ...;

# Recompile with:
# mkimage -C none -A arm -T script -d /boot/boot.cmd /boot/boot.scr