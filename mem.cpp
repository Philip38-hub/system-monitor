#include "header.h"
#include <sys/statvfs.h> // For statvfs

// Function to get physical memory (RAM) usage percentage
float getMemoryUsage()
{
    struct sysinfo memInfo;
    sysinfo(&memInfo);

    long long totalVirtualMem = memInfo.totalram;
    // Add other meminfo like swap here
    totalVirtualMem += memInfo.totalswap;
    totalVirtualMem *= memInfo.mem_unit;

    long long physMemUsed = memInfo.totalram - memInfo.freeram;
    physMemUsed *= memInfo.mem_unit;

    if (memInfo.totalram == 0)
        return 0.0f;

    return (float)physMemUsed / (float)(memInfo.totalram * memInfo.mem_unit) * 100.0f;
}

// Function to get virtual memory (SWAP) usage percentage
float getSwapUsage()
{
    struct sysinfo memInfo;
    sysinfo(&memInfo);

    long long totalSwap = memInfo.totalswap;
    totalSwap *= memInfo.mem_unit;

    long long swapUsed = memInfo.totalswap - memInfo.freeswap;
    swapUsed *= memInfo.mem_unit;

    if (totalSwap == 0)
        return 0.0f;

    return (float)swapUsed / (float)totalSwap * 100.0f;
}

// Function to get disk usage percentage for the root filesystem
float getDiskUsage()
{
    struct statvfs stat;
    if (statvfs("/", &stat) != 0)
    {
        // Error
        return 0.0f;
    }

    unsigned long long totalBlocks = stat.f_blocks;
    unsigned long long freeBlocks = stat.f_bfree;
    unsigned long long availableBlocks = stat.f_bavail; // Blocks available to non-privileged user

    unsigned long long totalSpace = totalBlocks * stat.f_bsize;
    unsigned long long freeSpace = freeBlocks * stat.f_bsize;
    unsigned long long usedSpace = totalSpace - freeSpace;

    if (totalSpace == 0)
        return 0.0f;

    return (float)usedSpace / (float)totalSpace * 100.0f;
}
