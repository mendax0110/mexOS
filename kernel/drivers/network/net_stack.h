#ifndef KERNEL_NET_STACK_H
#define KERNEL_NET_STACK_H

#include "../../shared/types.h"

/**
 * @brief Represents a network packet with its data and length \struct net_packet
 */
struct net_packet
{
    uint8_t* data;
    size_t length;
};

/**
 * @brief Callback type for handling received network packets \struct net_packet_handler
 * @param packet The received network packet
 */
typedef void (*net_packet_handler)(const struct net_packet* packet);

/**
 * @brief Represents the network stack with its packet handler \struct net_stack_t
 */
typedef struct net_stack
{
    net_packet_handler handler;
} net_stack_t;

/**
 * @brief Ethertype values (network byte order handled internally, these are host-order constants)
 */
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806

/**
 * @brief IPv4 protocol numbers
 */
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

/**
 * @brief ARP hardware/protocol types and opcodes
 */
#define ARP_HTYPE_ETHERNET 1
#define ARP_PTYPE_IPV4 0x0800
#define ARP_OPCODE_REQUEST 1
#define ARP_OPCODE_REPLY 2

/**
 * @brief ICMP message types used by the minimal handler
 */
#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8

/**
 * @brief IPv4 header structure (20 bytes, no options) \struct ip_v4_header_t
 */
typedef struct ip_v4_header
{
    uint8_t version_ihl;
    uint8_t type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
} PACKED ip_v4_header_t;

/**
 * @brief Ethernet frame structure (14 bytes header + payload) \struct ethernet_frame_t
 */
typedef struct ethernet_frame
{
    uint8_t dest_mac[6];
    uint8_t source_mac[6];
    uint16_t ethertype;
    uint8_t payload[1500];
} PACKED ethernet_frame_t;

/**
 * @brief ARP packet structure (28 bytes) \struct arp_packet_t
 */
typedef struct arp_packet
{
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_size;
    uint8_t protocol_size;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} PACKED arp_packet_t;

/**
 * @brief UDP header structure (8 bytes) \struct udp_header_t
 */
typedef struct udp_header
{
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

/**
 * @brief TCP header structure (20 bytes, no options) \struct tcp_header_t
 */
typedef struct tcp_header
{
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint8_t data_offset_reserved;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} PACKED tcp_header_t;

/**
 * @brief ICMP header structure (8 bytes) \struct icmp_header_t
 */
typedef struct icmp_header
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest_of_header;
} PACKED icmp_header_t;

/**
 * @brief Network stack configuration structure \struct net_stack_config_t
 */
typedef struct net_stack_config
{
    uint8_t mac_address[6];
    uint32_t ip_address;
    uint32_t subnet_mask;
    uint32_t gateway;
} net_stack_config_t;

/**
 * @brief Initialize the network stack.
 * @param handler Optional callback invoked for every packet the stack doesn't
 */
void net_stack_init(net_packet_handler handler);

/**
 * @brief Record the NIC's MAC address, used as the source for outgoing frames and to answer ARP requests.
 * @param mac_address Pointer to a 6-byte MAC address
 * @return true if the MAC address was accepted and stored, false if invalid
 */
bool configure_hardware_address(const uint8_t* mac_address);

/**
 * @brief Assign the stack's IPv4 address, subnet mask, and default gateway.
 * @param ip_address IPv4 address in host byte order
 * @param subnet_mask Subnet mask in host byte order
 * @param gateway Default gateway in host byte order
 */
void net_stack_set_ip_config(uint32_t ip_address, uint32_t subnet_mask, uint32_t gateway);

/**
 * @brief Entry point called by the NIC driver's RX path for every received frame.
 * @param data Pointer to the raw ethernet frame (starting at dest MAC), owned by the driver
 * @param length Frame length in bytes, not including the trailing CRC
 */
void net_stack_handle_frame(const uint8_t* data, size_t length);

/**
 * @brief Send a raw ethernet frame via the active NIC.
 * @param dest_mac Pointer to a 6-byte destination MAC address
 * @param ethertype EtherType value (host byte order)
 * @param payload Pointer to the payload data to send
 * @param payload_length Length of the payload in bytes (must be <= 1500)
 * @return true if handed off to the driver successfully
 */
bool net_stack_send_frame(const uint8_t dest_mac[6], uint16_t ethertype, const uint8_t* payload, size_t payload_length);

/**
 * @brief Broadcast an ARP "who-has" request for the given target IP (host byte order).
 * @param target_ip IPv4 address to query in host byte order
 * @return true if the request was sent successfully, false otherwise
 */
bool net_stack_send_arp_request(uint32_t target_ip);

/**
 * @brief Convert a 16-bit value from host to network byte order.
 * @param host16 Value in host byte order
 * @return Value in network byte order
 */
uint16_t net_htons(uint16_t host16);

/**
 * @brief Convert a 32-bit value from host to network byte order.
 * @param host32 Value in host byte order
 * @return Value in network byte order
 */
uint32_t net_htonl(uint32_t host32);

/**
 * @brief Convert a 16-bit value from network to host byte order.
 * @param net16 Value in network byte order
 * @return Value in host byte order
 */
uint16_t net_ntohs(uint16_t net16);

/**
 * @brief Convert a 32-bit value from network to host byte order.
 * @param net32 Value in network byte order
 * @return Value in host byte order
 */
uint32_t net_ntohl(uint32_t net32);

/**
 * @brief Check if a string is a valid IPv4 address and convert it to a 32-bit integer in host byte order.
 * @param ip_addr The null-terminated string containing the IPv4 address (e.g., "
 * @param out_ip The pointer to a uint32_t that will receive the converted IP address in host byte order
 * @return A non-zero value if the conversion was successful, zero if the string is not a valid IPv4 address
 */
int net_aton(const char* ip_addr, uint32_t* out_ip);

/**
 * @brief Standard Internet checksum (RFC 1071) over a buffer.
 * @param data Pointer to the data to checksum
 * @param length Length in bytes
 * @return Checksum in network byte order, ready to place directly into a header field
 */
uint16_t net_checksum(const void* data, size_t length);

/**
 * @brief Retrieve the current network stack configuration.
 */
struct net_stack_config net_stack_get_config(void);

#endif // KERNEL_NET_STACK_H