/*
* ESPAsyncZCPP.h
*
* Project: ESPAsyncZCPP - Asynchronous ZCPP library for Arduino ESP8266 and ESP32
* Copyright (c) 2019 Keith Westley
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#ifndef ESPASYNCZCPP_H_
#define ESPASYNCZCPP_H_

#ifdef ESP32
#include <WiFi.h>
#include <AsyncUDP.h>
#elif defined (ESP8266)
#include <ESPAsyncUDP.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#else
#error Platform not supported
#endif

#include <lwip/ip_addr.h>
#include <lwip/igmp.h>
#include <Arduino.h>
#include "RingBuf.h"

#if LWIP_VERSION_MAJOR == 1
typedef struct ip_addr ip4_addr_t;
#endif

#include "ZCPP.h"

/*
Code ripped from main sketch
uint32_t            *seqZCPPError;  // Sequence error tracking for each universe
uint16_t            lastZCPPConfig; // last config we saw
uint8_t             seqZCPPTracker; // sequence number of zcpp frames


setup():
    lastZCPPConfig = -1;
    if (zcpp.begin(ourLocalIP)) {
        LOG_PORT.println(F("- ZCPP Enabled"));
        ZCPPSub();
    } else {
        LOG_PORT.println(F("*** ZCPP INIT FAILED ****"));
    }

updateConfig():
    seqZCPPTracker = 0;
    seqZCPPError = 0;
    zcpp.stats.num_packets = 0;

void ZCPPSub() {
    ip_addr_t ifaddr;
    ifaddr.addr = static_cast<uint32_t>(ourLocalIP);
    ip_addr_t multicast_addr;
    multicast_addr.addr = static_cast<uint32_t>(IPAddress(224, 0, 30, 5));
    igmp_joingroup(&ifaddr, &multicast_addr);
    LOG_PORT.println(F("- ZCPP Subscribed to multicast 224.0.30.5"));
}

void sendZCPPConfig(ZCPP_packet_t& packet) {

    LOG_PORT.println("Sending ZCPP Configuration query response.");

    memset(&packet, 0x00, sizeof(packet));
    memcpy(packet.QueryConfigurationResponse.Header.token, ZCPP_token, sizeof(ZCPP_token));
    packet.QueryConfigurationResponse.Header.type = ZCPP_TYPE_QUERY_CONFIG_RESPONSE;
    packet.QueryConfigurationResponse.Header.protocolVersion = ZCPP_CURRENT_PROTOCOL_VERSION;
    packet.QueryConfigurationResponse.sequenceNumber = ntohs(lastZCPPConfig);
    strncpy(packet.QueryConfigurationResponse.userControllerName, config.id.c_str(), std::min((int)strlen(config.id.c_str()), (int)sizeof(packet.QueryConfigurationResponse.userControllerName)));
    if (config.channel_count == 0) {
        packet.QueryConfigurationResponse.ports = 0;
    }
    else {
        packet.QueryConfigurationResponse.ports = 1;
        packet.QueryConfigurationResponse.PortConfig[0].port = 0;
//TODO: Add unified mode switching or simplify

//        #if defined(ESPS_MODE_SERIAL)
//        packet.QueryConfigurationResponse.PortConfig[0].port |= 0x80;
//        #endif

        packet.QueryConfigurationResponse.PortConfig[0].string = 0;
        packet.QueryConfigurationResponse.PortConfig[0].startChannel = ntohl((uint32_t)config.channel_start);

//TODO: Add unified mode switching or simplify
//#if defined(ESPS_MODE_PIXEL)
        switch(config.pixel_type) {
//#elif defined(ESPS_MODE_SERIAL)
//        switch(config.serial_type) {
//#endif
//#if defined(ESPS_MODE_PIXEL)
          case  PixelType::WS2811:
              packet.QueryConfigurationResponse.PortConfig[0].protocol = ZCPP_PROTOCOL_WS2811;
              break;
          case  PixelType::GECE:
              packet.QueryConfigurationResponse.PortConfig[0].protocol = ZCPP_PROTOCOL_GECE;
              break;

#elif defined(ESPS_MODE_SERIAL)
          case  SerialType::DMX512:
              packet.QueryConfigurationResponse.PortConfig[0].protocol = ZCPP_PROTOCOL_DMX;
              break;
          case  SerialType::RENARD:
              packet.QueryConfigurationResponse.PortConfig[0].protocol = ZCPP_PROTOCOL_RENARD;
              break;
#endif

        }
        packet.QueryConfigurationResponse.PortConfig[0].channels = ntohl((uint32_t)config.channel_count);

//TODO: Add unified mode switching or simplify
//#if defined(ESPS_MODE_PIXEL)
        packet.QueryConfigurationResponse.PortConfig[0].grouping = config.groupSize;

        switch(config.pixel_color) {
          case PixelColor::RGB:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_RGB;
              break;
          case PixelColor::RBG:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_RBG;
              break;
          case PixelColor::GRB:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_GRB;
              break;
          case PixelColor::GBR:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_GBR;
              break;
          case PixelColor::BRG:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_BRG;
              break;
          case PixelColor::BGR:
              packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = ZCPP_COLOUR_ORDER_BGR;
              break;
        }

        packet.QueryConfigurationResponse.PortConfig[0].brightness = config.briteVal * 100.0f;
        packet.QueryConfigurationResponse.PortConfig[0].gamma = config.gammaVal * 10;
//TODO: Add unified mode switching or simplify
#else
        packet.QueryConfigurationResponse.PortConfig[0].grouping = 0;
        packet.QueryConfigurationResponse.PortConfig[0].directionColourOrder = 0;
        packet.QueryConfigurationResponse.PortConfig[0].brightness = 100.0f;
        packet.QueryConfigurationResponse.PortConfig[0].gamma = 0;
#endif
    }

    zcpp.sendConfigResponse(&packet);
}

loop():
bool abortPacketRead = false;
while (!zcpp.isEmpty() && !abortPacketRead) {
    ZCPP_packet_t zcppPacket;
    idleTicker.attach(config.effect_idletimeout, idleTimeout);
    if (config.ds == DataSource::IDLEWEB || config.ds == DataSource::E131) {
        config.ds = DataSource::ZCPP;
    }

    zcpp.pull(&zcppPacket);

    switch (zcppPacket.Discovery.Header.type) {
        case ZCPP_TYPE_DISCOVERY: // discovery
            {
                LOG_PORT.println("ZCPP Discovery received.");
                int pixelPorts = 0;
                int serialPorts = 0;
//TODO: Add unified mode switching or simplify
#if defined(ESPS_MODE_PIXEL)
                pixelPorts = 1;
#elif defined(ESPS_MODE_SERIAL)
                serialPorts = 1;
#endif
                char version[9];
                memset(version, 0x00, sizeof(version));
                for (uint8_t i = 0; i < min(strlen_P(VERSION), sizeof(version)-1); i++)
                version[i] = pgm_read_byte(VERSION + i);

                uint8_t mac[WL_MAC_ADDR_LENGTH];
                zcpp.sendDiscoveryResponse(&zcppPacket, version, WiFi.macAddress(mac), config.id.c_str(), pixelPorts, serialPorts, 680 * 3, 512, 680 * 3, static_cast<uint32_t>(ourLocalIP), static_cast<uint32_t>(ourSubnetMask));
            }
            break;
        case ZCPP_TYPE_CONFIG: // config
            LOG_PORT.println("ZCPP Config received.");
            if (htons(zcppPacket.Configuration.sequenceNumber) != lastZCPPConfig) {
            // a new config to apply
            LOG_PORT.print("    The config is new: ");
            LOG_PORT.println(htons(zcppPacket.Configuration.sequenceNumber));

            config.id = String(zcppPacket.Configuration.userControllerName);
            LOG_PORT.print("    Controller Name: ");
            LOG_PORT.println(config.id);

            ZCPP_PortConfig* p = zcppPacket.Configuration.PortConfig;
            for (int i = 0; i < zcppPacket.Configuration.ports; i++) {
                if (p->port == 0) {
                    switch(p->protocol) {
//TODO: Add unified mode switching or simplify
#if defined(ESPS_MODE_PIXEL)
                        case ZCPP_PROTOCOL_WS2811:
                            config.pixel_type = PixelType::WS2811;
                            break;
                        case ZCPP_PROTOCOL_GECE:
                            config.pixel_type = PixelType::GECE;
                            break;
#elif defined(ESPS_MODE_SERIAL)
                        case ZCPP_PROTOCOL_DMX:
                            config.serial_type = SerialType::DMX512;
                            break;
                        case ZCPP_PROTOCOL_RENARD:
                            config.serial_type = SerialType::RENARD;
                            break;
#endif
                        default:
                            LOG_PORT.print("Attempt to configure invalid protocol ");
                            LOG_PORT.print(p->protocol);
                            break;
                    }
                    LOG_PORT.print("    Protocol: ");
//TODO: Add unified mode switching or simplify
#if defined(ESPS_MODE_PIXEL)
                    LOG_PORT.println((int)config.pixel_type);
#elif defined(ESPS_MODE_SERIAL)
                    LOG_PORT.println((int)config.serial_type);
#endif
                    config.channel_start = htonl(p->startChannel);
                    LOG_PORT.print("    Start Channel: ");
                    LOG_PORT.println(config.channel_start);
                    config.channel_count = htonl(p->channels);
                    LOG_PORT.print("    Channel Count: ");
                    LOG_PORT.println(config.channel_count);
TODO: Add unified mode switching or simplify
#if defined(ESPS_MODE_PIXEL)
                    config.groupSize = p->grouping;
                    LOG_PORT.print("    Group Size: ");
                    LOG_PORT.println(config.groupSize);
                    switch(ZCPP_GetColourOrder(p->directionColourOrder)) {
                        case ZCPP_COLOUR_ORDER_RGB:
                            config.pixel_color = PixelColor::RGB;
                            break;
                        case ZCPP_COLOUR_ORDER_RBG:
                            config.pixel_color = PixelColor::RBG;
                            break;
                        case ZCPP_COLOUR_ORDER_GRB:
                            config.pixel_color = PixelColor::GRB;
                            break;
                        case ZCPP_COLOUR_ORDER_GBR:
                            config.pixel_color = PixelColor::GBR;
                            break;
                        case ZCPP_COLOUR_ORDER_BRG:
                            config.pixel_color = PixelColor::BRG;
                            break;
                        case ZCPP_COLOUR_ORDER_BGR:
                            config.pixel_color = PixelColor::BGR;
                            break;
                        default:
                            LOG_PORT.print("Attempt to configure invalid colour order ");
                            LOG_PORT.print(ZCPP_GetColourOrder(p->directionColourOrder));
                            break;
                    }
                    LOG_PORT.print("    Colour Order: ");
                    LOG_PORT.println((int)config.pixel_color);
                    config.briteVal = (float)p->brightness / 100.0f;
                    LOG_PORT.print("    Brightness: ");
                    LOG_PORT.println(config.briteVal);
                    config.gammaVal = ZCPP_GetGamma(p->gamma);
                    LOG_PORT.print("    Gamma: ");
                    LOG_PORT.println(config.gammaVal);
#endif
                }
                else {
                    LOG_PORT.print("Attempt to configure invalid port ");
                    LOG_PORT.print(p->port);
                }

                p += sizeof(zcppPacket.Configuration.PortConfig);
                }

                if (zcppPacket.Configuration.flags & ZCPP_CONFIG_FLAG_LAST) {
                    lastZCPPConfig = htons(zcppPacket.Configuration.sequenceNumber);
                    saveConfig();
                    if ((zcppPacket.Configuration.flags & ZCPP_CONFIG_FLAG_QUERY_CONFIGURATION_RESPONSE_REQUIRED) != 0) {
                    sendZCPPConfig(zcppPacket);
                    }
                }
            }
            else {
            LOG_PORT.println("    The config has not changed.");
            }
            break;
        case ZCPP_TYPE_QUERY_CONFIG: // query config
            sendZCPPConfig(zcppPacket);
            break;
        case ZCPP_TYPE_SYNC: // sync
        doShow = true;
        // exit read and send data to the pixels
        abortPacketRead = true;
        break;
        case ZCPP_TYPE_DATA: // data
            uint8_t seq = zcppPacket.Data.sequenceNumber;
            uint32_t offset = htonl(zcppPacket.Data.frameAddress);
            bool frameLast = zcppPacket.Data.flags & ZCPP_DATA_FLAG_LAST;
            uint16_t len = htons(zcppPacket.Data.packetDataLength);
            bool sync = (zcppPacket.Data.flags & ZCPP_DATA_FLAG_SYNC_WILL_BE_SENT) != 0;

            if (sync) {
            // suppress display until we see a sync
            doShow = false;
            }

            if (seq != seqZCPPTracker) {
            LOG_PORT.print(F("Sequence Error - expected: "));
            LOG_PORT.print(seqZCPPTracker);
            LOG_PORT.print(F(" actual: "));
            LOG_PORT.println(seq);
            seqZCPPError++;
            }

            if (frameLast)
            seqZCPPTracker = seq + 1;

            zcpp.stats.num_packets++;

            for (int i = offset; i < offset + len; i++) {
//TODO: Add unified mode switching or simplify
#if defined(ESPS_MODE_PIXEL)
            pixels.setValue(i, zcppPacket.Data.data[i - offset]);
#elif defined(ESPS_MODE_SERIAL)
            serial.setValue(i, zcppPacket.Data.data[i - offset]);
#endif
            }

            break;
    }
}


*/

// Error Types
typedef enum {
    ERROR_ZCPP_NONE,
    ERROR_ZCPP_IGNORE,
    ERROR_ZCPP_ID,
    ERROR_ZCPP_PROTOCOL_VERSION,
} ZCPP_error_t;

// Status structure
typedef struct {
    uint32_t    num_packets;
    uint32_t    packet_errors;
    IPAddress   last_clientIP;
    uint16_t    last_clientPort;
    unsigned long    last_seen;
} ZCPP_stats_t;

class ESPAsyncZCPP {
 private:

	ZCPP_packet_t   *sbuff;       // Pointer to scratch packet buffer
    AsyncUDP        udp;          // UDP
    RingBuf         *pbuff;       // Ring Buffer of universe packet buffers
	bool            suspend;      // suspends all ZCPP processing until discovery is responded to

    // Internal Initializers
    bool initUDP(IPAddress ourIP);

    // Packet parser callback
    void parsePacket(AsyncUDPPacket _packet);

 public:
    ZCPP_stats_t  stats;    // Statistics tracker

    ESPAsyncZCPP(uint8_t buffers = 1);

    // Generic UDP listener, no physical or IP configuration
    bool begin(IPAddress ourIP);

    // Ring buffer access
    inline bool isEmpty() { return pbuff->isEmpty(pbuff); }
    inline void *pull(ZCPP_packet_t *packet) { return pbuff->pull(pbuff, packet); }
	  void sendDiscoveryResponse(ZCPP_packet_t* packet, const char* firmwareVersion, const uint8_t* mac, const char* controllerName, int pixelPorts, int serialPorts, uint32_t maxPixelPortChannels, uint32_t maxSerialPortChannels, uint32_t maximumChannels, uint32_t ipAddress, uint32_t ipMask);
    void sendConfigResponse(ZCPP_packet_t* packet);

    // Diag functions
    void dumpError(ZCPP_error_t error);
};
#endif  // ESPASYNCZCPP_H_