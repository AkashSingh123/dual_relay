#include <pcap.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <cstring>
#include <algorithm> // For std::search
#include <cmath>     // For std::round

// Function to convert raw bytes to a readable format
std::string bytesToHexString(const u_char* bytes, int length) {
    std::ostringstream oss;
    for (int i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
    }
    return oss.str();
}


std::string bytesToIpString(const u_char* bytes) {
    std::ostringstream oss;
    oss << (int)bytes[0] << "." << (int)bytes[1] << "." << (int)bytes[2] << "." << (int)bytes[3];
    return oss.str();
}

uint16_t bytesToUint16(const u_char* bytes) {
    return (bytes[0] << 8) | bytes[1];
}


int calculateDomainId(uint16_t port) {
    double domainId = (port - 7400) / 250.0;
    return std::round(domainId);
}

// Function to print RTPS packet details
void printRtpsPacketDetails(const u_char* packet, int length, const struct pcap_pkthdr* pkthdr) {
    // RTPS magic header bytes
    const u_char rtps_magic[] = {0x52, 0x54, 0x50, 0x53};

    // Search for the RTPS magic header
    const u_char* magic = std::search(packet, packet + length, std::begin(rtps_magic), std::end(rtps_magic));
    if (magic == packet + length) {
        return; // RTPS magic header not found
    }
    const std::vector<std::pair<const char*, const char*>> writerEntities = {
        {"\x00\x00\x0e\x02", "swamp_gcs"},
        {"\x00\x00\x12\x02", "target_update"},
        {"\x00\x00\x16\x02", "command"},
        {"\x00\x00\x18\x02", "zones"}
    };

    // Writer Entity ID is located 12 bytes after the Submessage ID
    if (magic + 36 <= packet + length) {
           // if (std::memcmp(magic + 32, "\x00\x00\x0e\x02", 4) == 0) {
            // Print timestamp with millisecond precision
        std::string writerEntityId = bytesToHexString(magic + 32, 4);
        if (writerEntityId == "00 00 0e 02 " || writerEntityId == "00 00 12 02 " ||
            writerEntityId == "00 00 16 02 " || writerEntityId == "00 00 18 02 ") {

            char timestampStr[64];
            std::tm* tm_info = std::localtime(&pkthdr->ts.tv_sec);
            std::strftime(timestampStr, sizeof(timestampStr), "%Y-%m-%d %H:%M:%S", tm_info);
            std::cout << "Packet received at: " << timestampStr << "." << std::setfill('0') << std::setw(6) << pkthdr->ts.tv_usec << std::endl;

            std::cout << "Packet length: " << length << std::endl;
            std::cout << "RTPS magic header found at offset: " << (magic - packet) << std::endl;

            // Protocol version is located after the magic header
            if (magic + 4 + 2 <= packet + length) {
                std::cout << "Protocol version: " << (int)magic[4] << "." << (int)magic[5] << std::endl;
            }

            // GUID prefix starts at offset 8 after the magic header
            if (magic + 16 + 12 <= packet + length) {
                std::cout << "GUID prefix: " << bytesToHexString(magic + 8, 12) << std::endl;

                // Extract the domain ID from the GUID prefix
                //uint32_t domainId = (magic[8] << 24) | (magic[9] << 16) | (magic[10] << 8) | magic[11];
                //std::cout << "Domain ID: " << domainId << std::endl;
            }

            // Submessage ID starts at offset 20 after the magic header
            if (magic + 24 <= packet + length) {
                std::cout << "Submessage ID: " << std::hex << (int)magic[20] << std::dec << std::endl;
            }

            // Writer Entity ID
            std::cout << "Writer Entity ID: " << bytesToHexString(magic + 32, 4) << std::endl;

            // Writer Entity Key is located immediately after the Writer Entity ID
            if (magic + 40 <= packet + length) {
                std::cout << "Writer Entity Key: " << bytesToHexString(magic + 36, 4) << std::endl;
            }
            
             
            if (magic + 36 <= packet + length) {
                std::cout << "Source IP: " << bytesToIpString(magic - 16) << std::endl;  // Source IP is 4 bytes before destination IP
                std::cout << "Destination IP: " << bytesToIpString(magic - 12) << std::endl;
                
            }
            if (magic + 48 <= packet + length) {
                uint16_t destPort = bytesToUint16(magic - 6);
                int calculatedDomainId = calculateDomainId(destPort);

                std::cout << "Destination port: " << destPort << std::endl;
                std::cout << "Domain ID: " << calculatedDomainId << std::endl;
            }
            if (writerEntityId == "00 00 0e 02 ") {
              std::cout << "Topic: swamp_gcs" << std::endl;
            } else if (writerEntityId == "00 00 12 02 ") {
                std::cout << "Topic: target_update" << std::endl;
            } else if (writerEntityId == "00 00 16 02 ") {
                std::cout << "Topic: command" << std::endl;
            } else if (writerEntityId == "00 00 18 02 ") {
                std::cout << "Topic: zones" << std::endl;
            }

            // Serialized data starts at offset 48 after the magic header
            if (magic + 48 <= packet + length) {
                std::cout << "Serialized data: " << bytesToHexString(magic + 44, length - (magic + 44 - packet)) << std::endl;
                std::cout << "Real Serialized data: " << bytesToHexString(magic + 48, length - (magic + 48 - packet)) << std::endl;
            }
            std::cout << std::endl << std::endl; // Two-line space after each loop

            
        }
    }
}

// Callback function to process captured packets
void packetHandler(u_char* userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    printRtpsPacketDetails(packet, pkthdr->len, pkthdr);
}

int main() {
    char errorBuffer[PCAP_ERRBUF_SIZE];
    pcap_if_t* interfaces;
    pcap_if_t* device;
    pcap_t* handle;

    // Find available devices
    if (pcap_findalldevs(&interfaces, errorBuffer) == -1) {
        std::cerr << "Error finding devices: " << errorBuffer << std::endl;
        return 1;
    }

    // Select the first device
    device = interfaces;
    if (!device) {
        std::cerr << "No devices found" << std::endl;
        return 1;
    }

    // Open the selected device for packet capture
    handle = pcap_open_live(device->name, BUFSIZ, 1, 1000, errorBuffer);
    if (!handle) {
        std::cerr << "Could not open device: " << device->name << ": " << errorBuffer << std::endl;
        return 1;
    }

    std::cout << "Using device: " << device->name << std::endl;

    // Set a filter to capture only RTPS packets
    struct bpf_program fp;
    char filterExp[] = "udp";
    if (pcap_compile(handle, &fp, filterExp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Could not parse filter: " << pcap_geterr(handle) << std::endl;
        return 1;
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        std::cerr << "Could not install filter: " << pcap_geterr(handle) << std::endl;
        return 1;
    }

    // Start capturing packets
    pcap_loop(handle, 0, packetHandler, nullptr);

    // Cleanup
    pcap_freealldevs(interfaces);
    pcap_close(handle);

    return 0;
}
