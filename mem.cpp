#include "header.h"
#include <sys/statvfs.h> // For statvfs
#include <dirent.h>      // For opendir, readdir, closedir
#include <algorithm>     // For std::remove
#include <cctype>        // For isdigit

// Helper function to read a value from a /proc file
template <typename T>
T getProcValue(const string &path, const string &key)
{
    ifstream file(path);
    string line;
    T value = T();
    while (getline(file, line))
    {
        if (line.rfind(key, 0) == 0)
        { // Check if line starts with key
            size_t colonPos = line.find(":");
            if (colonPos != string::npos)
            {
                string valStr = line.substr(colonPos + 1);
                valStr.erase(remove_if(valStr.begin(), valStr.end(), ::isspace), valStr.end());
                try
                {
                    if constexpr (is_same_v<T, int>)
                        value = stoi(valStr);
                    else if constexpr (is_same_v<T, long long int>)
                        value = stoll(valStr);
                    else if constexpr (is_same_v<T, float>)
                        value = stof(valStr);
                    else if constexpr (is_same_v<T, string>)
                        value = valStr;
                }
                catch (...)
                {
                    // Conversion error
                }
            }
            break;
        }
    }
    return value;
}

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

// Function to get all running processes
vector<Proc> getAllProcesses()
{
    vector<Proc> processes;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/proc")) == NULL)
    {
        perror("opendir");
        return processes;
    }

    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_type == DT_DIR)
        {
            bool is_pid = true;
            for (char *p = ent->d_name; *p; ++p)
            {
                if (!isdigit(*p))
                {
                    is_pid = false;
                    break;
                }
            }

            if (is_pid)
            {
                int pid = stoi(ent->d_name);
                string statPath = "/proc/" + string(ent->d_name) + "/stat";
                string statusPath = "/proc/" + string(ent->d_name) + "/status";

                ifstream statFile(statPath);
                string statLine;
                if (getline(statFile, statLine))
                {
                    Proc p;
                    p.pid = pid;

                    // Parse /proc/[pid]/stat
                    // Example: 1 (systemd) S 0 1 1 0 -1 4194304 1000 0 0 0 0 0 0 0 20 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
                    // char comm[256];
                    char state_char;
                    unsigned long vsize_ul, rss_ul, utime_ul, stime_ul, starttime_ul;

                    // Read the process name, which can contain spaces and be enclosed in parentheses
                    size_t first_paren = statLine.find('(');
                    size_t last_paren = statLine.rfind(')');
                    if (first_paren != string::npos && last_paren != string::npos && last_paren > first_paren) {
                        p.name = statLine.substr(first_paren + 1, last_paren - (first_paren + 1));
                    } else {
                        p.name = "Unknown"; // Fallback if name parsing fails
                    }

                    // Extract the rest of the fields after the name
                    string remaining_stat_line = statLine.substr(last_paren + 2); // +2 to skip ') '
                    // Corrected sscanf to accurately parse fields from /proc/[pid]/stat
                    // Fields needed: state, utime, stime, vsize, rss, starttime
                    // The format string needs to account for all fields before the ones we need.
                    // Based on proc(5) man page, fields are:
                    // pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads rtpriority processor vsize rss starttime ...
                    // We need to parse: state (field 3), utime (field 14), stime (field 15), vsize (field 23), rss (field 24), starttime (field 22)
                    // The remaining_stat_line starts after 'comm' (field 2), so state is the 1st field in remaining_stat_line.
                    // utime is the 12th field in remaining_stat_line.
                    // stime is the 13th field in remaining_stat_line.
                    // vsize is the 21st field in remaining_stat_line.
                    // rss is the 22nd field in remaining_stat_line.
                    // starttime is the 23rd field in remaining_stat_line.
                    sscanf(remaining_stat_line.c_str(), "%c %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu %*s %*s %*s %*s %*s %*s %*s %lu %lu %lu",
                           &state_char, &utime_ul, &stime_ul, &vsize_ul, &rss_ul, &starttime_ul);
 
                    p.state = state_char;
                    p.vsize = vsize_ul;
                    p.rss = rss_ul;
                    p.utime = utime_ul;
                    p.stime = stime_ul;
                    p.starttime = starttime_ul;

                    // Get memory usage from /proc/[pid]/status (VmRSS)
                    string vmRSS = getProcValue<string>(statusPath, "VmRSS:");
                    if (!vmRSS.empty())
                    {
                        try
                        {
                            p.rss = stoll(vmRSS.substr(0, vmRSS.find(" "))); // Extract numeric part before "kB"
                        }
                        catch (...)
                        {
                            p.rss = 0;
                        }
                    }

                    processes.push_back(p);
                }
                statFile.close();
            }
        }
    }

    closedir(dir);
    return processes;
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
    // unsigned long long availableBlocks = stat.f_bavail; // Blocks available to non-privileged user

    unsigned long long totalSpace = totalBlocks * stat.f_bsize;
    unsigned long long freeSpace = freeBlocks * stat.f_bsize;
    unsigned long long usedSpace = totalSpace - freeSpace;

    if (totalSpace == 0)
        return 0.0f;

    return (float)usedSpace / (float)totalSpace * 100.0f;
}
