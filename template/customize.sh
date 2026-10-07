# shellcheck disable=SC2034
SKIPUNZIP=1

ui_print "*********************************************************"
ui_print "*                     Unlock UWB                        *"
ui_print "*           Zygisk + LSPlant UWB Unlocker               *"
ui_print "*********************************************************"

ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'service.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'post-fs-data.sh' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'system/*' -d "$MODPATH" >&2 2>/dev/null || true

mkdir -p "$MODPATH/zygisk"

# Check 32-bit support
HAS32BIT=false
if [ -n "$(getprop ro.product.cpu.abilist32)" ] || [ -n "$(getprop ro.system.product.cpu.abilist32)" ]; then
    HAS32BIT=true
fi

if [ "$ARCH" = "arm64" ]; then
    ui_print "- Extracting arm64-v8a library..."
    unzip -o "$ZIPFILE" "zygisk/arm64-v8a.so" -d "$MODPATH" >&2
    if [ "$HAS32BIT" = true ]; then
        unzip -o "$ZIPFILE" "zygisk/armeabi-v7a.so" -d "$MODPATH" >&2 2>/dev/null || true
    fi
elif [ "$ARCH" = "arm" ]; then
    ui_print "- Extracting armeabi-v7a library..."
    unzip -o "$ZIPFILE" "zygisk/armeabi-v7a.so" -d "$MODPATH" >&2
elif [ "$ARCH" = "x64" ]; then
    ui_print "- Extracting x86_64 library..."
    unzip -o "$ZIPFILE" "zygisk/x86_64.so" -d "$MODPATH" >&2
elif [ "$ARCH" = "x86" ]; then
    ui_print "- Extracting x86 library..."
    unzip -o "$ZIPFILE" "zygisk/x86.so" -d "$MODPATH" >&2
fi

ui_print "- Setting permissions..."
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755

ui_print "- Installation complete! Please reboot."
