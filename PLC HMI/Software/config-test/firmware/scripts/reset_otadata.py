Import("env")
import os

project_dir = env.subst("$PROJECT_DIR")
print(">>> [reset_otadata] project_dir:", project_dir)
build_dir = env.subst("$BUILD_DIR")
if not os.path.isabs(build_dir):
    build_dir = os.path.normpath(os.path.join(project_dir, build_dir))
print(">>> [reset_otadata] build_dir:", build_dir)
os.makedirs(build_dir, exist_ok=True)
otadata_path = os.path.join(build_dir, "otadata_empty.bin")

# 8KB of 0xFF represents an uninitialized/erased otadata partition.
# ESP-IDF bootloader recognizes this state and unconditionally boots app0.
# Pre-create all build subdirectories for LVGL, source tree, and libraries
# to prevent Windows SCons race conditions where GCC fails to open .d/.o files.
src_dirs = ['src', 'src/src', 'src/include', 'src/include/app', 'src/include/ui', 'src/include/drivers']
for p in src_dirs:
    os.makedirs(os.path.join(build_dir, p), exist_ok=True)

# Ensure config_hwtest.ino.cpp exists and is updated from config_hwtest.ino
ino_path = os.path.join(project_dir, "config_hwtest.ino")
ino_cpp_path = os.path.join(project_dir, "config_hwtest.ino.cpp")
if os.path.exists(ino_path):
    import shutil
    shutil.copyfile(ino_path, ino_cpp_path)

def safe_makedirs(path):
    try:
        norm = os.path.normpath(path)
        os.makedirs(norm, exist_ok=True)
    except Exception as ex:
        print(f"[reset_otadata] safe_makedirs error: {ex}")

lv_src = os.path.join(project_dir, ".pio", "libdeps", "ht_v3a", "lvgl", "src")
print(">>> [reset_otadata] lvgl_src:", lv_src, "exists:", os.path.exists(lv_src))
if os.path.exists(lv_src):
    for lib_candidate in ["lib38d", "libd94"]:
        dst = os.path.join(build_dir, lib_candidate, "lvgl")
        for root, dirs, _ in os.walk(lv_src):
            rel = os.path.relpath(root, lv_src)
            safe_makedirs(os.path.join(dst, rel))


for lib_name in [
    'lib14a/SPI', 'lib5b9/FS', 'lib1af/SD', 'libf56/SPIFFS', 'lib4d7/LittleFS',
    'libd3f/TFT_eSPI', 'lib53e/TFT_eSPI', 'lib552/BLE', 'lib43b/RTClib', 'libc82/Wire',
    'libd87/WiFi', 'lib161/HTTPClient', 'libe3f/NetworkClientSecure', 'lib812/Network',
    'lib285/MAX6675 library', 'libd15/Adafruit BusIO', 'libfef/bb_captouch',
    'libHTTPClient', 'libHTTPUpdate', 'libNetworkClientSecure', 'libWiFi', 'libWire',
    'FrameworkArduino', 'FrameworkArduino/libb64', 'FrameworkArduino/apps', 'FrameworkArduino/apps/sntp'
]:
    safe_makedirs(os.path.join(build_dir, lib_name))


if not os.path.exists(otadata_path) or os.path.getsize(otadata_path) != 8192:
    with open(otadata_path, "wb") as f:
        f.write(b"\xFF" * 8192)

env.Append(
    FLASH_EXTRA_IMAGES=[
        ("0xe000", otadata_path)
    ],
    UPLOADERFLAGS=[
        "0xe000", otadata_path
    ]
)

env.Replace(
    ESP32_APP_OFFSET="0x10000"
)

# Ensure SCons child processes (like esptool) have access to tool-esptoolpy and site-packages
try:
    tool_esptoolpy = env.PioPlatform().get_package_dir("tool-esptoolpy")
except Exception:
    tool_esptoolpy = None
if not tool_esptoolpy or not os.path.exists(tool_esptoolpy):
    tool_esptoolpy = os.path.expanduser(r"~\.platformio\packages\tool-esptoolpy")

penv_site = os.path.expanduser(r"~\.platformio\penv\Lib\site-packages")
pp_parts = [p for p in [tool_esptoolpy, penv_site] if p and os.path.exists(p)]
if pp_parts:
    cur_pp = env["ENV"].get("PYTHONPATH", "")
    env["ENV"]["PYTHONPATH"] = os.pathsep.join(pp_parts) + (os.pathsep + cur_pp if cur_pp else "")

