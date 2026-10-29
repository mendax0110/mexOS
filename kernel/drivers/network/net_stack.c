#include "net_stack.h"
#include "network.h"
#include "lib/log.h"
#include "lib/string.h"

static net_stack_t g_stack;
static net_stack_config_t g_config;

static const uint8_t BROADCAST_MAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint16_t net_htons(const uint16_t host16)
{
    return (uint16_t)((host16 << 8) | (host16 >> 8));
}

uint32_t net_htonl(const uint32_t host32)
{
    return ((host32 & 0x000000FFu) << 24) |
           ((host32 & 0x0000FF00u) << 8)  |
           ((host32 & 0x00FF0000u) >> 8)  |
           ((host32 & 0xFF000000u) >> 24);
}

uint16_t net_ntohs(const uint16_t net16)
{
    return net_htons(net16); // Symmetric
}

uint32_t net_ntohl(const uint32_t net32)
{
    return net_htonl(net32); // Symmetric
}

int net_aton(const char* ip_addr, uint32_t* out_ip)
{
    if (!ip_addr || !out_ip)
    {
        return 0;
    }

    uint32_t result = 0;
    int octet_count = 0;
    const char* ptr = ip_addr;

    while (*ptr && octet_count < 4)
    {
        char* endptr;
        const long_t octet = strtol(ptr, &endptr, 10);

        if (octet < 0 || octet > 255 || endptr == ptr)
        {
            return 0;
        }

        result = (result << 8) | (uint32_t)octet;
        octet_count++;

        if (*endptr == '.')
        {
            ptr = endptr + 1;
        }
        else if (*endptr == '\0' && octet_count == 4)
        {
            break;
        }
        else
        {
            return 0;
        }
    }

    if (octet_count != 4)
    {
        return 0;
    }

    *out_ip = result;
    return 1;
}

struct net_stack_config net_stack_get_config(void)
{
    return g_config;
}

uint16_t net_checksum(const void* data, size_t length)
{
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t sum = 0;

    while (length > 1)
    {
        const uint16_t word = (uint16_t)((bytes[0] << 8) | bytes[1]);
        sum += word;
        bytes += 2;
        length -= 2;
    }

    if (length == 1)
    {
        sum += (uint16_t)(bytes[0] << 8);
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return net_htons((uint16_t)~sum);
}

void net_stack_init(const net_packet_handler handler)
{
    memset(&g_stack, 0, sizeof(g_stack));
    memset(&g_config, 0, sizeof(g_config));
    g_stack.handler = handler;
    log_info("Network stack initialized");
}

bool configure_hardware_address(const uint8_t* mac_address)
{
    if (!mac_address)
    {
        return false;
    }

    memcpy(g_config.mac_address, mac_address, 6);
    return true;
}

void net_stack_set_ip_config(const uint32_t ip_address, const uint32_t subnet_mask, const uint32_t gateway)
{
    g_config.ip_address = ip_address;
    g_config.subnet_mask = subnet_mask;
    g_config.gateway = gateway;

    log_info_fmt("Network stack IP configured: %d.%d.%d.%d",
                 (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF,
                 (ip_address >> 8) & 0xFF, ip_address & 0xFF);
}

bool net_stack_send_frame(const uint8_t dest_mac[6], const uint16_t ethertype, const uint8_t* payload, const size_t payload_length)
{
    if (!network_is_present() || payload_length > sizeof(((ethernet_frame_t*)0)->payload))
    {
        return false;
    }

    uint8_t frame[sizeof(ethernet_frame_t)];
    ethernet_frame_t* eth = (ethernet_frame_t*)frame;

    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->source_mac, g_config.mac_address, 6);
    eth->ethertype = net_htons(ethertype);
    memcpy(eth->payload, payload, payload_length);

    const size_t total_length = 14 + payload_length; // header (dest+src+ethertype) + payload
    return network_send(frame, total_length);
}

bool net_stack_send_arp_request(const uint32_t target_ip)
{
    arp_packet_t arp;
    memset(&arp, 0, sizeof(arp));

    arp.hardware_type = net_htons(ARP_HTYPE_ETHERNET);
    arp.protocol_type = net_htons(ARP_PTYPE_IPV4);
    arp.hardware_size = 6;
    arp.protocol_size = 4;
    arp.opcode = net_htons(ARP_OPCODE_REQUEST);
    memcpy(arp.sender_mac, g_config.mac_address, 6);
    arp.sender_ip = net_htonl(g_config.ip_address);
    memset(arp.target_mac, 0, 6);
    arp.target_ip = net_htonl(target_ip);

    return net_stack_send_frame(BROADCAST_MAC, ETHERTYPE_ARP, (const uint8_t*)&arp, sizeof(arp));
}

static void net_stack_handle_arp(const arp_packet_t* arp)
{
    if (net_ntohs(arp->hardware_type) != ARP_HTYPE_ETHERNET ||
        net_ntohs(arp->protocol_type) != ARP_PTYPE_IPV4)
    {
        return;
    }

    const uint16_t opcode = net_ntohs(arp->opcode);
    const uint32_t target_ip = net_ntohl(arp->target_ip);

    if (opcode == ARP_OPCODE_REQUEST && g_config.ip_address != 0 && target_ip == g_config.ip_address)
    {
        arp_packet_t reply;
        memset(&reply, 0, sizeof(reply));

        reply.hardware_type = net_htons(ARP_HTYPE_ETHERNET);
        reply.protocol_type = net_htons(ARP_PTYPE_IPV4);
        reply.hardware_size = 6;
        reply.protocol_size = 4;
        reply.opcode = net_htons(ARP_OPCODE_REPLY);
        memcpy(reply.sender_mac, g_config.mac_address, 6);
        reply.sender_ip = net_htonl(g_config.ip_address);
        memcpy(reply.target_mac, arp->sender_mac, 6);
        reply.target_ip = arp->sender_ip;

        net_stack_send_frame(arp->sender_mac, ETHERTYPE_ARP, (const uint8_t*)&reply, sizeof(reply));
    }
}

static void net_stack_handle_icmp(const ip_v4_header_t* ip, const uint8_t* icmp_data, const size_t icmp_length, const uint8_t sender_mac[6])
{
    if (icmp_length < sizeof(icmp_header_t))
    {
        return;
    }

    const icmp_header_t* header = (const icmp_header_t*)icmp_data;

    if (header->type != ICMP_TYPE_ECHO_REQUEST || g_config.ip_address == 0)
    {
        return;
    }

    // Build reply: same ICMP payload, type flipped to echo reply, checksum recomputed.
    uint8_t reply_buf[sizeof(ip_v4_header_t) + 1500];
    if (icmp_length > sizeof(reply_buf) - sizeof(ip_v4_header_t))
    {
        log_warn("ICMP echo request too large to reply to, dropping");
        return;
    }

    ip_v4_header_t* reply_ip = (ip_v4_header_t*)reply_buf;
    icmp_header_t* reply_icmp = (icmp_header_t*)(reply_buf + sizeof(ip_v4_header_t));

    memcpy(reply_icmp, icmp_data, icmp_length);
    reply_icmp->type = ICMP_TYPE_ECHO_REPLY;
    reply_icmp->checksum = 0;
    reply_icmp->checksum = net_checksum(reply_icmp, icmp_length);

    reply_ip->version_ihl = 0x45; // IPv4, 5 * 4 = 20 byte header, no options
    reply_ip->type_of_service = 0;
    reply_ip->total_length = net_htons((uint16_t)(sizeof(ip_v4_header_t) + icmp_length));
    reply_ip->identification = 0;
    reply_ip->flags_fragment_offset = 0;
    reply_ip->time_to_live = 64;
    reply_ip->protocol = IP_PROTO_ICMP;
    reply_ip->header_checksum = 0;
    reply_ip->source_ip = net_htonl(g_config.ip_address);
    reply_ip->dest_ip = ip->source_ip; // Already network order
    reply_ip->header_checksum = net_checksum(reply_ip, sizeof(ip_v4_header_t));

    net_stack_send_frame(sender_mac, ETHERTYPE_IPV4, reply_buf, sizeof(ip_v4_header_t) + icmp_length);
}

static void net_stack_handle_ipv4(const ip_v4_header_t* ip, const size_t frame_remaining, const uint8_t sender_mac[6])
{
    const uint8_t ihl_words = ip->version_ihl & 0x0F;
    const size_t header_length = (size_t)ihl_words * 4;

    if (ihl_words < 5 || header_length > frame_remaining)
    {
        log_warn("Dropping malformed IPv4 packet (bad IHL)");
        return;
    }

    const size_t total_length = net_ntohs(ip->total_length);
    if (total_length < header_length || total_length > frame_remaining)
    {
        log_warn("Dropping malformed IPv4 packet (bad total length)");
        return;
    }

    const uint8_t* payload = (const uint8_t*)ip + header_length;
    const size_t payload_length = total_length - header_length;

    switch (ip->protocol)
    {
        case IP_PROTO_ICMP:
            net_stack_handle_icmp(ip, payload, payload_length, sender_mac);
            break;
        default:
        {
            if (g_stack.handler)
            {
                const struct net_packet packet = { .data = (uint8_t*)ip, .length = total_length };
                g_stack.handler(&packet);
            }
            break;
        }
    }
}

void net_stack_handle_frame(const uint8_t* data, const size_t length)
{
    if (!data || length < 14)
    {
        return;
    }

    const ethernet_frame_t* eth = (const ethernet_frame_t*)data;
    const uint16_t ethertype = net_ntohs(eth->ethertype);
    const size_t payload_length = length - 14;

    switch (ethertype)
    {
        case ETHERTYPE_ARP:
        {
            if (payload_length >= sizeof(arp_packet_t))
            {
                net_stack_handle_arp((const arp_packet_t*)eth->payload);
            }
            break;
        }
        case ETHERTYPE_IPV4:
        {
            if (payload_length >= sizeof(ip_v4_header_t))
            {
                net_stack_handle_ipv4((const ip_v4_header_t*)eth->payload, payload_length, eth->source_mac);
            }
            break;
        }
        default:
        {
            // Unknown ethertype: pass the raw frame up if someone registered a catch-all handler.
            if (g_stack.handler)
            {
                const struct net_packet packet = { .data = (uint8_t*)data, .length = length };
                g_stack.handler(&packet);
            }
            break;
        }
    }
}