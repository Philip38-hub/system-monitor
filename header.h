// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
};

struct TX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int frame;
    int compressed;
    int multicast;
};

struct RX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int colls;
    int carrier;
    int compressed;
};

struct NetworkUsage {
    float rxRate;  // MB/s
    float txRate;  // MB/s
};

// student TODO : system stats
string CPUinfo();
const char *getOsName();
string getLoggedInUser();
string getHostname();
int getTotalProcesses();

struct HistoryData
{
    vector<float> values;
    int offset;
    float max_value;
    float min_value;
    ImVec4 color;
    string overlay_text;

    HistoryData() : offset(0), max_value(0.0f), min_value(0.0f), color(1.0f, 1.0f, 1.0f, 1.0f) {}

    void addValue(float val)
    {
        if (values.empty())
        {
            values.resize(90); // Default history size
            max_value = val;
            min_value = val;
        }
        values[offset] = val;
        offset = (offset + 1) % values.size();
        max_value = fmax(max_value, val);
        min_value = fmin(min_value, val);
    }
};

float getCPUUsage();
string getFanStatus();
float getFanSpeed();
float getCPUTemperature();

// student TODO : memory and processes
float getMemoryUsage();
float getSwapUsage();
float getDiskUsage();
vector<Proc> getAllProcesses();

// student TODO : network
vector<IP4> getIPv4Addresses();
map<string, RX> getRXStats();
map<string, TX> getTXStats();
NetworkUsage getNetworkUsage();
string formatBytes(int bytes);
void networkWindow(const char *id, ImVec2 size, ImVec2 position);
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position);

#endif
