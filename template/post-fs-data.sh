#!/system/bin/sh
MODDIR=${0%/*}

# Early property injection before system services read them
resetprop -n uwb.regulation.skip true
resetprop -n uwb.labs.enable true
