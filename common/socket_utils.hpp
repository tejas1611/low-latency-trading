#pragma once

#include <iostream>
#include <string>
#include <unordered_set>
#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "macros.hpp"
#include "logger.hpp"
#include "time_utils.hpp"

namespace Common {
    constexpr int MaxTCPServerBacklog = 1024;

    inline auto getIfaceIP(const std::string &iface) -> std::string {
        char buf[NI_MAXHOST] = {'\0'};
        ifaddrs* ifaddr = nullptr;

        if (getifaddrs(&ifaddr) != -1) {
        for (ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name) {
            getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
            break;
            }
        }
        freeifaddrs(ifaddr);
        }

        return buf;
    }

    // Sockets will not block on read, but instead return immediately if data is not available.
    inline auto setNonBlocking(int fd) -> bool {
        const auto flags = fcntl(fd, F_GETFL, 0);
        if (flags & O_NONBLOCK)
        return true;
        return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    // Disable Nagle's algorithm and associated delays.
    inline auto disableNagle(int fd) -> bool {
        int one = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void *>(&one), sizeof(one)) != -1);
    }

    // Allow software receive timestamps on incoming packets.
    inline auto setSOTimestamp(int fd) -> bool {
        int one = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, reinterpret_cast<void *>(&one), sizeof(one)) != -1);
    }

    // Create a TCP / UDP socket to either connect to or listen for data on or listen for connections on the specified interface and IP:port information.
    inline auto createSocket(Logger &logger, const std::string& t_ip, const std::string& iface, int port, 
    bool is_udp, bool is_blocking, bool is_listening, bool needs_so_timestamp) -> int {
        std::string time_str;
        const auto ip = t_ip.empty() ? getIfaceIP(iface) : t_ip;

        logger.log("%:% %() % ip:% iface:% port:% is_udp:% is_blocking:% is_listening:% SO_time:%\n", 
        __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str), ip, iface, port, is_udp, is_blocking, is_listening, needs_so_timestamp);
        
        const int input_flags = (is_listening ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
        const addrinfo hints{input_flags, AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM,
                            is_udp ? IPPROTO_UDP : IPPROTO_TCP, 0, 0, nullptr, nullptr};
        
        addrinfo *result = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
        ASSERT(!rc, "getaddrinfo() failed. error:" + std::string(gai_strerror(rc)) + "errno:" + strerror(errno));

        int fd = -1;
        int one = 1;
        for (addrinfo *rp = result; rp; rp = rp->ai_next) {
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            ASSERT(fd != -1, "socket() failed. errno:" + std::string(strerror(errno)));

            if (!is_blocking) {
                ASSERT(setNonBlocking(fd), "setNonBlocking() failed. errno:" + std::string(strerror(errno)));
            }

            if (!is_udp) {          // disable Nagle for TCP sockets
                ASSERT(disableNagle(fd), "disableNagle() failed. errno:" + std::string(strerror(errno)));
            }

            if (!is_listening) {    // establish connection to specified address
                ASSERT(connect(fd, rp->ai_addr, rp->ai_addrlen) != 1, "connect() failed. errno:" + std::string(strerror(errno)));
            }

            if (is_listening) {     // allow re-using the address in the call to bind()
                ASSERT(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&one), sizeof(one)) == 0, "setsockopt() SO_REUSEADDR failed. errno:" + std::string(strerror(errno)));
            }

            if (is_listening) {
                // bind to the specified port number.
                const sockaddr_in addr{AF_INET, htons(port), {htonl(INADDR_ANY)}, {}};
                ASSERT(bind(fd, is_udp ? reinterpret_cast<const struct sockaddr *>(&addr) : rp->ai_addr, sizeof(addr)) == 0, "bind() failed. errno:%" + std::string(strerror(errno)));
            }

            if (!is_udp && is_listening) { // listen for incoming TCP connections.
                ASSERT(listen(fd, MaxTCPServerBacklog) == 0, "listen() failed. errno:" + std::string(strerror(errno)));
            }

            if (needs_so_timestamp) { // enable software receive timestamps.
                ASSERT(setSOTimestamp(fd), "setSOTimestamp() failed. errno:" + std::string(strerror(errno)));
            }
        }

        return fd;
    }
}