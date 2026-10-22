#ifndef KERNEL_NETWORK_H
#define KERNEL_NETWORK_H

#include "../../../shared/types.h"
#include "net_stack.h"

/**
 * @brief Known Intel e1000-family PCI vendor/device IDs.
 * Covers QEMU's default `-net nic,model=e1000` (82540EM, 0x100E) and the common
 * 8254x/8257x desktop and server NICs. If your hardware isn't in this list,
 * add its device ID here (`lspci -nn | grep Eth` on the target machine).
 */
#define E1000_VENDOR_INTEL 0x8086

#define E1000_KNOWN_DEVICE_IDS                                                                  \
    0x1000, 0x1001, 0x1004, 0x1008, 0x1009, 0x100C, 0x100D, 0x100E, 0x100F, 0x1010,             \
    0x1011, 0x1012, 0x1013, 0x1015, 0x1016, 0x1017, 0x1018, 0x1019, 0x101A, 0x101D,             \
    0x101E, 0x1026, 0x1027, 0x1028, 0x105E, 0x105F, 0x1060, 0x1075, 0x1076, 0x1077,             \
    0x1078, 0x1079, 0x107A, 0x107B, 0x107C, 0x107D, 0x107E, 0x107F, 0x1096, 0x1098,             \
    0x1099, 0x109A, 0x10B5, 0x10B9, 0x10D3, 0x10F5, 0x2E6E

/**
 * @brief E1000 register offsets (byte offsets from the MMIO base)
 */
#define E1000_REG_CTRL 0x0000
#define E1000_REG_STATUS 0x0008
#define E1000_REG_EECD 0x0010
#define E1000_REG_EERD 0x0014
#define E1000_REG_ICR 0x00C0
#define E1000_REG_ITR 0x00C4
#define E1000_REG_ICS 0x00C8
#define E1000_REG_IMS 0x00D0
#define E1000_REG_IMC 0x00D8
#define E1000_REG_RCTL 0x0100
#define E1000_REG_TCTL 0x0400
#define E1000_REG_TIPG 0x0410
#define E1000_REG_RDBAL 0x2800
#define E1000_REG_RDBAH 0x2804
#define E1000_REG_RDLEN 0x2808
#define E1000_REG_RDH 0x2810
#define E1000_REG_RDT 0x2818
#define E1000_REG_TDBAL 0x3800
#define E1000_REG_TDBAH 0x3804
#define E1000_REG_TDLEN 0x3808
#define E1000_REG_TDH 0x3810
#define E1000_REG_TDT 0x3818
#define E1000_REG_RAL0 0x5400
#define E1000_REG_RAH0 0x5404
#define E1000_REG_MTA  0x5200

/**
 * @brief CTRL register bits
 */
#define E1000_CTRL_SLU (1u << 6)
#define E1000_CTRL_ASDE (1u << 5)
#define E1000_CTRL_RST (1u << 26)

/**
 * @brief EECD register bits
 */
#define E1000_EECD_PRES (1u << 8)

/**
 * @brief EERD register layout
 */
#define E1000_EERD_START (1u << 0)
#define E1000_EERD_DONE (1u << 4)
#define E1000_EERD_ADDR_SHIFT 8
#define E1000_EERD_DATA_SHIFT 16

/**
 * @brief RCTL register bits
 */
#define E1000_RCTL_EN (1u << 1)
#define E1000_RCTL_SBP (1u << 2)
#define E1000_RCTL_UPE (1u << 3)
#define E1000_RCTL_MPE (1u << 4)
#define E1000_RCTL_BAM (1u << 15)
#define E1000_RCTL_BSIZE_2048 (0u << 16)
#define E1000_RCTL_SECRC (1u << 26)
#define E1000_RCTL_DEFAULT (E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC)

/**
 * @brief TCTL register bits
 */
#define E1000_TCTL_EN (1u << 1)
#define E1000_TCTL_PSP (1u << 3)
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD_SHIFT 12
#define E1000_TCTL_DEFAULT (E1000_TCTL_EN | E1000_TCTL_PSP | (0x0F << E1000_TCTL_CT_SHIFT) | (0x40 << E1000_TCTL_COLD_SHIFT))

/**
 * @brief Standard full-duplex inter-packet gap timings
 */
#define E1000_TIPG_DEFAULT 0x0060200A

/**
 * @brief Interrupt cause/mask bits we care about
 */
#define E1000_INT_TXDW (1u << 0)
#define E1000_INT_LSC (1u << 2)
#define E1000_INT_RXO (1u << 6)
#define E1000_INT_RXT0 (1u << 7)
#define E1000_IMS_ENABLE (E1000_INT_TXDW | E1000_INT_LSC | E1000_INT_RXO | E1000_INT_RXT0)

/**
 * @brief RX descriptor status byte bits
 */
#define E1000_RXD_STATUS_DD (1u << 0)
#define E1000_RXD_STATUS_EOP (1u << 1)

/**
 * @brief TX descriptor command byte bits
 */
#define E1000_TXD_CMD_EOP (1u << 0)
#define E1000_TXD_CMD_IFCS (1u << 1)
#define E1000_TXD_CMD_RS  (1u << 3)

/**
 * @brief TX descriptor status byte bits
 */
#define E1000_TXD_STATUS_DD (1u << 0)

/**
 * @brief Ring sizes 32 * 16 bytes = 512 bytes per ring.
 */
#define E1000_RX_DESC_COUNT 32
#define E1000_TX_DESC_COUNT 8
#define E1000_BUFFER_SIZE 2048

/**
 * @brief E1000 hardware receive descriptor layout \struct e1000_rx_desc_t
 */
typedef struct e1000_rx_desc
{
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} PACKED e1000_rx_desc_t;

/**
 * @brief E1000 hardware transmit descriptor layout \struct e1000_tx_desc_t
 */
typedef struct e1000_tx_desc
{
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} PACKED e1000_tx_desc_t;

/**
 * @brief Driver-level state for a detected e1000 NIC \struct e1000_device_t
 */
typedef struct e1000_device
{
    volatile uint8_t* mmio_base;
    uint32_t mmio_phys;
    uint32_t mmio_size;
    uint8_t irq_line;
    uint8_t mac_address[6];

    e1000_rx_desc_t* rx_ring;
    uint32_t rx_ring_phys;
    uint8_t* rx_buffer_virt[E1000_RX_DESC_COUNT];
    uint32_t rx_buffer_phys[E1000_RX_DESC_COUNT];
    uint32_t rx_cur;

    e1000_tx_desc_t* tx_ring;
    uint32_t tx_ring_phys;
    uint8_t* tx_buffer_virt[E1000_TX_DESC_COUNT];
    uint32_t tx_buffer_phys[E1000_TX_DESC_COUNT];
    uint32_t tx_tail;

    bool present;
} e1000_device_t;

/**
 * @brief Detect an e1000-family NIC on the PCI bus and bring it online.
 * Maps its MMIO BAR, resets the chip, sets up RX/TX descriptor rings backed
 * by physically-contiguous pages, unmasks interrupts, and registers the IRQ handler.
 * @return true if a device was found and initialized, false otherwise
 */
bool network_init(void);

/**
 * @brief Get the MAC address of the initialized NIC.
 * @param out_mac Buffer of at least 6 bytes to receive the MAC address
 * @return true if a NIC is present and the address was written
 */
bool network_get_mac_address(uint8_t* out_mac);

/**
 * @brief Transmit a raw ethernet frame.
 * @param data Pointer to frame data (dest MAC .. payload), no trailing CRC (hardware appends it)
 * @param length Frame length in bytes (must be <= E1000_BUFFER_SIZE)
 * @return true if the frame was handed off to a TX descriptor
 */
bool network_send(const uint8_t* data, size_t length);

/**
 * @brief Returns true if a NIC was found and initialized successfully.
 */
bool network_is_present(void);

#endif // KERNEL_NETWORK_H