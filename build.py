#!/usr/bin/env python3
import os
import sys
import shutil
import zipfile
import subprocess
import argparse
from pathlib import Path

MODULE_ID = "unlock_uwb"
MODULE_NAME = "unlock-uwb"
MODULE_VER = "v1.0.0"

ROOT_DIR = Path(__file__).parent.resolve()
TEMPLATE_DIR = ROOT_DIR / "template"
BUILD_DIR = ROOT_DIR / "build"
RELEASE_DIR = ROOT_DIR / "release"


def build_dex():
    print("[*] Checking helper dex...")
    dex_script = ROOT_DIR / "helper" / "build-dex.sh"
    header_file = ROOT_DIR / "native" / "jni" / "helper_dex.h"
    if not header_file.exists():
        print("[*] Compiling helper DEX...")
        subprocess.check_call(["bash", str(dex_script)], cwd=str(ROOT_DIR / "helper"))
    else:
        print("[+] helper_dex.h already exists.")


def build_native(ndk_path, abi="arm64-v8a", api_level=28):
    print(f"[*] Building native library for {abi} (API {api_level})...")
    cmake_toolchain = Path(ndk_path) / "build" / "cmake" / "android.toolchain.cmake"
    if not cmake_toolchain.exists():
        raise FileNotFoundError(f"CMake toolchain not found at {cmake_toolchain}")

    out_dir = BUILD_DIR / abi
    out_dir.mkdir(parents=True, exist_ok=True)

    cmake_args = [
        "cmake",
        f"-B{out_dir}",
        f"-S{ROOT_DIR / 'native'}",
        f"-DCMAKE_TOOLCHAIN_FILE={cmake_toolchain}",
        f"-DANDROID_ABI={abi}",
        f"-DANDROID_PLATFORM=android-{api_level}",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DANDROID_STL=c++_static"
    ]
    subprocess.check_call(cmake_args)
    subprocess.check_call(["cmake", "--build", str(out_dir), "--config", "Release", "--parallel"])

    so_file = out_dir / "libunlock_uwb.so"
    if not so_file.exists():
        raise FileNotFoundError(f"Expected binary {so_file} not found")

    return so_file


def package_zip(built_libs):
    print("[*] Packaging flashable Magisk/KernelSU ZIP...")
    RELEASE_DIR.mkdir(parents=True, exist_ok=True)
    zip_path = RELEASE_DIR / f"{MODULE_NAME}-{MODULE_VER}.zip"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        # Template files
        for root, _, files in os.walk(TEMPLATE_DIR):
            for file in files:
                p = Path(root) / file
                arcname = p.relative_to(TEMPLATE_DIR)
                z.write(p, arcname)

        # Zygisk libraries
        for abi, lib in built_libs.items():
            z.write(lib, f"zygisk/{abi}.so")

    print(f"[+] Successfully built: {zip_path}")
    return zip_path


def main():
    parser = argparse.ArgumentParser(description="Build unlock-uwb Zygisk module")
    parser.add_argument("--ndk", help="Path to Android NDK (or set ANDROID_NDK_HOME / ANDROID_NDK_ROOT)")
    parser.add_argument("--abi", default="arm64-v8a", help="Target ABI (default: arm64-v8a)")
    args = parser.parse_args()

    build_dex()

    ndk = args.ndk or os.environ.get("ANDROID_NDK_HOME") or os.environ.get("ANDROID_NDK_ROOT")
    if not ndk:
        print("[!] Warning: ANDROID_NDK_HOME not set. Native compilation skipped locally.")
        print("[!] The module can be built via GitHub Actions CI automatically upon push.")
        return

    built_libs = {}
    so = build_native(ndk, abi=args.abi)
    built_libs[args.abi] = so

    package_zip(built_libs)


if __name__ == "__main__":
    main()
