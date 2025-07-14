#include "header.h"
#include <cctype> // For isdigit

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// getLoggedInUser, this will get the currently logged in user
string getLoggedInUser()
{
    char user[LOGIN_NAME_MAX];
    if (getlogin_r(user, sizeof(user)) == 0)
    {
        return string(user);
    }
    return "Unknown";
}

// getHostname, this will get the hostname of the computer
string getHostname()
{
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        return string(hostname);
    }
    return "Unknown";
}

// getTotalProcesses, this will get the total number of processes
int getTotalProcesses()
{
    int count = 0;
    DIR *dir;
    struct dirent *ent;

    // Open the /proc directory
    if ((dir = opendir("/proc")) == NULL)
    {
        perror("opendir");
        return -1;
    }

    // Iterate over all entries in /proc
    while ((ent = readdir(dir)) != NULL)
    {
        // Check if the entry is a directory and its name consists only of digits (PIDs)
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
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

// Function to get CPU usage percentage
float getCPUUsage()
{
    static long long lastTotalUser = 0;
    static long long lastTotalUserNice = 0;
    static long long lastTotalSystem = 0;
    static long long lastTotalIdle = 0;
    static long long lastTotalIowait = 0;
    static long long lastTotalIrq = 0;
    static long long lastTotalSoftirq = 0;
    static long long lastTotalSteal = 0;
    static long long lastTotalGuest = 0;
    static long long lastTotalGuestNice = 0;
    static long long lastTotalNonIdle = 0; // Corrected: Added this static variable

    long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guestNice;
    ifstream file("/proc/stat");
    string line;
    if (getline(file, line))
    {
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestNice);
    }
    file.close();

    long long totalIdle = idle + iowait;
    long long totalNonIdle = user + nice + system + irq + softirq + steal + guest + guestNice;
    long long total = totalIdle + totalNonIdle;

    long long diffIdle = totalIdle - lastTotalIdle;
    long long diffTotal = total - (lastTotalIdle + lastTotalNonIdle); // Corrected: Used lastTotalNonIdle

    float cpu_usage = 0.0f;
    if (diffTotal != 0)
    {
        cpu_usage = (float)(totalNonIdle - lastTotalNonIdle) / (float)diffTotal * 100.0f;
    }

    lastTotalUser = user;
    lastTotalUserNice = nice;
    lastTotalSystem = system;
    lastTotalIdle = idle;
    lastTotalIowait = iowait;
    lastTotalIrq = irq;
    lastTotalSoftirq = softirq;
    lastTotalSteal = steal;
    lastTotalGuest = guest;
    lastTotalGuestNice = guestNice;
    lastTotalNonIdle = totalNonIdle; // Corrected: Updated lastTotalNonIdle

    return cpu_usage;
}
