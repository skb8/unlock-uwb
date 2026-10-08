# Unlock UWB (Zygisk + LSPlant)

[English](README.md) | [Русский](README.ru.md)

A Magisk / KernelSU / APatch module that unlocks **Ultra-Wideband (UWB)** features and the **UWB Labs** engineering menu on Samsung Galaxy (One UI) phones and other Android devices.

## What this module does

1. **Unlocks the UWB menu and toggle in One UI Settings** by bypassing `isMenuUnavailable()`, `isRestrictionMode()`, and `isRegulationMode` checks in Samsung and AOSP controllers.
2. **Enables the hidden UWB Labs menu** by setting `mLabsEnabled` and the `uwb.labs.enable` system property. This provides access to built-in tests:
   - FiRa One-to-One Ranging Test (`UwbFiraTestFragment`)
   - Simple Ranging Test (`UwbSimpleTestFragment`)
   - DL-TDoA Test (`UwbDltdoaTestFragment`)
   - Extended statistics and session history
3. **Bypasses regional restrictions for channels 5 and 9** by overriding the country code with an allowed value (`US`) in `CountryDetectorService` and `UwbCountryCode`, and setting the Samsung HAL flag `uwb.regulation.skip = true` to skip regulatory-domain checks in the chip firmware.
4. **Enables the Android hardware feature declaration** `android.hardware.uwb` through a systemless overlay at `/vendor/etc/permissions/android.hardware.uwb.xml`.

## Architecture

This is a **Zygisk module** powered by **LSPlant**, the ART hooking engine also used by LSPosed. It hooks into processes as they start:

- `com.android.settings`: bypasses UWB availability checks.
- `system_server`: hooks country-detection methods.
- Native code and system properties: sets `uwb.regulation.skip` and `uwb.labs.enable` before services initialize.

## Installation

1. Download the latest `unlock-uwb-v*.zip` from [Releases](https://github.com/skb8/unlock-uwb/releases) or from the Actions artifacts.
2. Install the ZIP using **Magisk**, **KernelSU**, or **APatch**. The archive also includes the installer files required for recovery flashing.
3. Make sure **Zygisk** is enabled in your root manager.
4. Reboot your device.
5. Open **Settings → Connections → Ultra-Wideband (UWB)**. The option should be enabled and open **UWB Labs**.

> **Note:** UWB requires a physical UWB chip. On Samsung devices, this is available on Plus, Ultra, and Fold models starting with the Galaxy S21+ / S21 Ultra generation.

## Build

### GitHub Actions

Every push to `main` builds an artifact. Pushing a version tag such as `v1.0.0` builds the release ZIP with `arm64-v8a` and `armeabi-v7a` libraries and publishes a GitHub Release.

### Local build

Requirements:

- Android NDK r26c or newer
- Python 3
- CMake 3.22 or newer
- Java JDK 17 or newer

```bash
git clone --recursive https://github.com/skb8/unlock-uwb.git
cd unlock-uwb

export ANDROID_NDK_HOME=/path/to/android-ndk
python3 build.py
```

The completed archive will be placed in `release/`.
