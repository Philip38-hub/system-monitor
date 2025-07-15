#include "header.h"

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
