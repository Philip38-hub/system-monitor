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
    static unsigned long long lastTotalUser = 0, lastTotalUserNice = 0, lastTotalSystem = 0, lastTotalIdle = 0;
    unsigned long long totalUser, totalUserNice, totalSystem, totalIdle;
    float cpu_usage = 0.0;

    ifstream file("/proc/stat");
    string line;
    getline(file, line);
    file.close();

    sscanf(line.c_str(), "cpu %llu %llu %llu %llu", &totalUser, &totalUserNice, &totalSystem, &totalIdle);

    if (totalUser < lastTotalUser || totalUserNice < lastTotalUserNice || totalSystem < lastTotalSystem || totalIdle < lastTotalIdle)
    {
        // Overflow detection. Just skip this value.
        cpu_usage = 0.0;
    }
    else
    {
        unsigned long long totalUserDiff = totalUser - lastTotalUser;
        unsigned long long totalUserNiceDiff = totalUserNice - lastTotalUserNice;
        unsigned long long totalSystemDiff = totalSystem - lastTotalSystem;
        unsigned long long totalIdleDiff = totalIdle - lastTotalIdle;
        unsigned long long totalDiff = totalUserDiff + totalUserNiceDiff + totalSystemDiff + totalIdleDiff;

        if (totalDiff > 0)
        {
            cpu_usage = (float)(totalUserDiff + totalUserNiceDiff + totalSystemDiff) / (float)totalDiff * 100.0f;
        }
        else
        {
            cpu_usage = 0.0f;
        }
    }

    lastTotalUser = totalUser;
    lastTotalUserNice = totalUserNice;
    lastTotalSystem = totalSystem;
    lastTotalIdle = totalIdle;

    return cpu_usage;
}

CPUStats getCPUStats()
{
    CPUStats stats = {};
    ifstream file("/proc/stat");
    string line;
    if (getline(file, line))
    {
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
               &stats.user, &stats.nice, &stats.system, &stats.idle, &stats.iowait, &stats.irq, &stats.softirq, &stats.steal, &stats.guest, &stats.guestNice);
    }
    file.close();
    return stats;
}

// Function to get fan status (e.g., "active", "inactive")
// Note: Fan status is typically found in /sys, not /proc.
// This implementation attempts to read from a common /sys path.
string getFanStatus()
{
    // Scan for all available fan inputs and return "Active" if any are found
    for (int i = 0; i < 10; ++i) { // Check up to 10 hwmon devices
        for (int j = 1; j < 10; ++j) { // Check up to 9 fan inputs per device
            string path = "/sys/class/hwmon/hwmon" + to_string(i) + "/fan" + to_string(j) + "_input";
            ifstream file(path);
            if (file.is_open()) {
                return "Active";
            }
        }
    }
    return "Inactive";
}

// Function to get fan speed (RPM)
// Note: Fan speed is typically found in /sys, not /proc.
// This implementation attempts to read from a common /sys path.
float getFanSpeed()
{
    // Scan for the first available fan speed
    for (int i = 0; i < 10; ++i) {
        for (int j = 1; j < 10; ++j) {
            string path = "/sys/class/hwmon/hwmon" + to_string(i) + "/fan" + to_string(j) + "_input";
            ifstream file(path);
            string line;
            if (file.is_open() && getline(file, line)) {
                try {
                    return stof(line);
                } catch (const std::exception &e) {
                    // Ignore conversion errors and continue scanning
                }
            }
        }
    }
    return 0.0f; // Default if no fan speed is found
}

// Function to get CPU temperature (Celsius)
// Note: CPU temperature is typically found in /sys, not /proc.
// This implementation attempts to read from a common /sys path.
float getCPUTemperature()
{
    // Scan for the first available temperature sensor
    for (int i = 0; i < 10; ++i) { // Check up to 10 thermal zones
        string path = "/sys/class/thermal/thermal_zone" + to_string(i) + "/temp";
        ifstream file(path);
        string line;
        if (file.is_open() && getline(file, line)) {
            try {
                return stof(line) / 1000.0f;
            } catch (const std::exception &e) {
                // Ignore conversion errors and continue scanning
            }
        }
    }
    return 0.0f; // Default if no temperature is found
}
// Function to get system uptime in seconds
float getSystemUptime()
{
    ifstream file("/proc/uptime");
    string line;
    getline(file, line);
    file.close();
    return stof(line.substr(0, line.find(" ")));
}
