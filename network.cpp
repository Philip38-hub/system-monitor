#include "header.h"
#include <fstream>
#include <sstream>
#include <map>

// Convert bytes to appropriate unit
string formatBytes(int bytes) {
    float converted = bytes;
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    
    while (converted >= 1024.0f && unitIndex < 3) {
        converted /= 1024.0f;
        unitIndex++;
    }
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", converted, units[unitIndex]);
    return string(buffer);
}

// Get network usage for visual display
NetworkUsage getNetworkUsage() {
    static int lastRxBytes = 0;
    static int lastTxBytes = 0;
    
    NetworkUsage usage = {0.0f, 0.0f};
    map<string, RX> rxStats = getRXStats();
    map<string, TX> txStats = getTXStats();
    
    // Sum up all interfaces
    int totalRxBytes = 0;
    int totalTxBytes = 0;
    
    for (const auto& [interface, rx] : rxStats) {
        totalRxBytes += rx.bytes;
    }
    
    for (const auto& [interface, tx] : txStats) {
        totalTxBytes += tx.bytes;
    }
    
    // Calculate deltas
    usage.rxRate = (totalRxBytes - lastRxBytes) / 1024.0f / 1024.0f; // Convert to MB/s
    usage.txRate = (totalTxBytes - lastTxBytes) / 1024.0f / 1024.0f; // Convert to MB/s
    
    lastRxBytes = totalRxBytes;
    lastTxBytes = totalTxBytes;
    
    return usage;
}

vector<IP4> getIPv4Addresses() {
    vector<IP4> ip4s;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return ip4s;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            IP4 ip;
            ip.name = ifa->ifa_name;
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, ip.addressBuffer, INET_ADDRSTRLEN);
            ip4s.push_back(ip);
        }
    }
    freeifaddrs(ifaddr);
    return ip4s;
}

// Get RX (receive) statistics for all network interfaces
map<string, RX> getRXStats() {
    map<string, RX> rxStats;
    ifstream netdev("/proc/net/dev");
    string line;

    // Skip header lines
    getline(netdev, line); // Inter-|   Receive...
    getline(netdev, line); // face  |   bytes....

    while (getline(netdev, line)) {
        istringstream iss(line);
        string interface;
        getline(iss, interface, ':');
        // Trim whitespace from interface name
        interface.erase(0, interface.find_first_not_of(" \t"));
        interface.erase(interface.find_last_not_of(" \t") + 1);

        RX rx;
        iss >> rx.bytes >> rx.packets >> rx.errs >> rx.drop
            >> rx.fifo >> rx.colls >> rx.carrier >> rx.compressed;
        
        rxStats[interface] = rx;
    }
    return rxStats;
}

// Get TX (transmit) statistics for all network interfaces
map<string, TX> getTXStats() {
    map<string, TX> txStats;
    ifstream netdev("/proc/net/dev");
    string line;

    // Skip header lines
    getline(netdev, line);
    getline(netdev, line);

    while (getline(netdev, line)) {
        istringstream iss(line);
        string interface;
        getline(iss, interface, ':');
        // Trim whitespace from interface name
        interface.erase(0, interface.find_first_not_of(" \t"));
        interface.erase(interface.find_last_not_of(" \t") + 1);

        // Skip RX stats
        // Skip RX stats (8 fields)
        string dummy;
        for (int i = 0; i < 8; ++i) {
            iss >> dummy;
        }

        TX tx;
        iss >> tx.bytes >> tx.packets >> tx.errs >> tx.drop
            >> tx.fifo >> tx.frame >> tx.compressed >> tx.multicast;
        
        txStats[interface] = tx;
    }
    return txStats;
}


