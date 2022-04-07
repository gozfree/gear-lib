/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libhal.h"
#include <libdstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

struct win_version_info {
    int major;
    int minor;
    int build;
    int revis;
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define is_str_equal(a,b) \
    ((strlen(a) == strlen(b)) && (0 == strcasecmp(a,b)))

#define MAX_NETIF_NUM           8
#define SYS_CLASS_NET           "/sys/class/net"
#define SYS_BLK_MMCBLK0        "/sys/block/mmcblk0/device"
#define DEV_MMCBLK0             "/dev/mmcblk0"
#define DEV_MMCBLK0P1           "/dev/mmcblk0p1"
#define SYSTEM_MOUNTS           "/proc/self/mounts"

int network_get_info(const char *intf, struct network_info *info)
{
    return 0;
}

int sdcard_get_info(const char *mount_point, struct sdcard_info *info)
{
    return 0;
}

int cpu_get_info(struct cpu_info *info)
{
        return 0;
}

int memory_get_info(struct memory_info *mem)
{
    return 0;
}

typedef DWORD(WINAPI *get_file_version_info_size_w_t)(LPCWSTR module, LPDWORD unused);
typedef BOOL(WINAPI *get_file_version_info_w_t)(LPCWSTR module, DWORD unused, DWORD len, LPVOID data);
typedef BOOL(WINAPI *ver_query_value_w_t)(LPVOID data, LPCWSTR subblock, LPVOID *buf, PUINT sizeout);

static bool get_dll_ver(const wchar_t *lib, struct win_version_info *ver_info)
{
    VS_FIXEDFILEINFO *info = NULL;
    UINT len = 0;
    BOOL success;
    LPVOID data;
    DWORD size;
    char utf8_lib[512];
    get_file_version_info_size_w_t get_file_version_info_size = NULL;
    get_file_version_info_w_t get_file_version_info = NULL;
    ver_query_value_w_t ver_query_value = NULL;

    HMODULE ver = GetModuleHandleW(L"version");

    if (!ver) {
        ver = LoadLibraryW(L"version");
        if (!ver) {
            printf("Failed to load windows version library");
            return false;
        }
    }

    get_file_version_info_size = (get_file_version_info_size_w_t)GetProcAddress(ver, "GetFileVersionInfoSizeW");
    get_file_version_info = (get_file_version_info_w_t)GetProcAddress(ver, "GetFileVersionInfoW");
    ver_query_value = (ver_query_value_w_t)GetProcAddress(ver, "VerQueryValueW");

    if (!get_file_version_info_size || !get_file_version_info ||
                    !ver_query_value) {
        printf("Failed to load windows version functions");
        return false;
    }

    wcs_to_utf8(lib, 0, utf8_lib, sizeof(utf8_lib));

    size = get_file_version_info_size(lib, NULL);
    if (!size) {
        printf("Failed to get %s version info size", utf8_lib);
        return false;
    }

    data = malloc(size);
    if (!get_file_version_info(lib, 0, size, data)) {
        printf("Failed to get %s version info", utf8_lib);
        free(data);
        return false;
    }

    success = ver_query_value(data, L"\\", (LPVOID *)&info, &len);
    if (!success || !info || !len) {
        printf("Failed to get %s version info value", utf8_lib);
        free(data);
        return false;
    }

    ver_info->major = (int)HIWORD(info->dwFileVersionMS);
    ver_info->minor = (int)LOWORD(info->dwFileVersionMS);
    ver_info->build = (int)HIWORD(info->dwFileVersionLS);
    ver_info->revis = (int)LOWORD(info->dwFileVersionLS);

    free(data);
    FreeLibrary(ver);

    return true;
}

static void get_win_ver(struct win_version_info *info)
{
#define WINVER_REG_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
    struct win_version_info ver = {0};

    if (!info)
        return;

    get_dll_ver(L"kernel32", &ver);

    if (ver.major == 10) {
        HKEY key;
        DWORD size, win10_revision;
        LSTATUS status;

        status = RegOpenKeyW(HKEY_LOCAL_MACHINE, WINVER_REG_KEY, &key);
        if (status != ERROR_SUCCESS)
            return;

        size = sizeof(win10_revision);

        status = RegQueryValueExW(key, L"UBR", NULL, NULL, (LPBYTE)&win10_revision, &size);
        if (status == ERROR_SUCCESS)
            ver.revis = (int)win10_revision > ver.revis ? (int)win10_revision : ver.revis;
        RegCloseKey(key);
    }

    *info = ver;
}

static bool is_64_bit_windows(void)
{
#if defined(_WIN64)
    return true;
#elif defined(_WIN32)
    BOOL b64 = false;
    return IsWow64Process(GetCurrentProcess(), &b64) && b64;
#endif
}

int os_get_version(struct os_info *os)
{
    bool b64;
    const char *windows_bitness;
    struct win_version_info ver;
    get_win_ver(&ver);

    b64 = is_64_bit_windows();
    windows_bitness = b64 ? "64" : "32";

    os->major = ver.major;
    os->minor = ver.minor;
    os->build = ver.build;
    os->revis = ver.revis;
    os->bit   = b64 ? 64 : 32;

    printf("Windows Version: %d.%d Build %d (revision: %d; %s-bit)",
                    ver.major, ver.minor, ver.build, ver.revis, windows_bitness);
    return 0;
}

int network_get_port_occupied(struct network_ports *np)
{
    return 0;
}

int system_noblock(char **argv)
{
    return 0;
}

ssize_t system_with_result(const char *cmd, void *buf, size_t count)
{
    return 0;
}

bool proc_exist(const char *proc)
{
    return false;
}
