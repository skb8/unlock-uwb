#!/system/bin/sh
MODDIR=${0%/*}

# Wait until boot completed
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 2
done

# Samsung One UI specific: bypass regional regulation domain restriction in UWB HAL / firmware
resetprop -n uwb.regulation.skip true

# Samsung One UI specific: enable hidden UWB Labs engineering test menu
resetprop -n uwb.labs.enable true

# Ensure UWB state is enabled in global settings database
settings put global uwb_enabled 1
