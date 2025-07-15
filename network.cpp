#include "header.h"
#include <fstream>
#include <sstream>
#include <map>

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
        int dummy;
        for (int i = 0; i < 8; i++) {
            iss >> dummy;
        }

        TX tx;
        iss >> tx.bytes >> tx.packets >> tx.errs >> tx.drop
            >> tx.fifo >> tx.frame >> tx.compressed >> tx.multicast;
        
        txStats[interface] = tx;
    }
    return txStats;
}

// ... getIPv4Addresses() function above ...

void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    ImGui::Text("Network IPv4 Addresses:");
    vector<IP4> ip4s = getIPv4Addresses();
    for (const auto &ip : ip4s) {
        ImGui::BulletText("%s: %s", ip.name, ip.addressBuffer);
    }

    ImGui::End();
}
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>
#include <string>

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
