#ifndef KERNEL_AHCI_H
#define KERNEL_AHCI_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// HBA (Host Bus Adapter) Memory Registers
#define AHCI_HBA_CAP 0x00
#define AHCI_HBA_GHC 0x04
#define AHCI_HBA_IS 0x08
#define AHCI_HBA_PI 0x0C
#define AHCI_HBA_VS 0x10

// Port Registers (offset from port base)
#define AHCI_PORT_CLB 0x00
#define AHCI_PORT_CLBU 0x04
#define AHCI_PORT_FB 0x08
#define AHCI_PORT_FBU 0x0C
#define AHCI_PORT_IS 0x10
#define AHCI_PORT_IE 0x14
#define AHCI_PORT_CMD 0x18
#define AHCI_PORT_TFD 0x20
#define AHCI_PORT_SIG 0x24
#define AHCI_PORT_SSTS 0x28
#define AHCI_PORT_SCTL 0x2C
#define AHCI_PORT_SERR 0x30
#define AHCI_PORT_SACT 0x34
#define AHCI_PORT_CI 0x38

// Port Command Register bits
#define AHCI_PORT_CMD_ST (1 << 0)
#define AHCI_PORT_CMD_FRE (1 << 4)
#define AHCI_PORT_CMD_FR (1 << 14)
#define AHCI_PORT_CMD_CR (1 << 15)

// Global HBA Control bits
#define AHCI_GHC_AHCI_EN (1 << 31)
#define AHCI_GHC_IE (1 << 1)
#define AHCI_GHC_HR (1 << 0)

// Device types
#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SATAPI 2
#define AHCI_DEV_SEMB 3
#define AHCI_DEV_PM 4

// SATA signature values
#define AHCI_SIG_ATA 0x00000101
#define AHCI_SIG_ATAPI 0xEB140101
#define AHCI_SIG_SEMB 0xC33C0101
#define AHCI_SIG_PM 0x96690101

// FIS (Frame Information Structure) types
#define FIS_TYPE_REG_H2D 0x27
#define FIS_TYPE_REG_D2H 0x34
#define FIS_TYPE_DMA_ACT 0x39
#define FIS_TYPE_DMA_SETUP 0x41
#define FIS_TYPE_DATA 0x46
#define FIS_TYPE_BIST 0x58
#define FIS_TYPE_PIO_SETU 0x5F
#define FIS_TYPE_DEV_BITS 0xA1

// ATA commands
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY 0xEC

/**
 * @brief FIS - Register Host to Device \struct fis_reg_h2d
 */
struct fis_reg_h2d
{
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    uint8_t command;
    uint8_t featurel;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    uint16_t count;
    uint8_t icc;
    uint8_t control;

    uint8_t rsv1[4];
} __attribute__((packed));

/**
 * @brief HBA Command Header \struct hba_cmd_header
 */
struct hba_cmd_header
{
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t w:1;
    uint8_t p:1;

    uint8_t r:1;
    uint8_t b:1;
    uint8_t c:1;
    uint8_t rsv0:1;
    uint8_t pmp:4;

    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} __attribute__((packed));

/**
 * @brief Physical Region Descriptor Table entry \struct hba_prdt_entry
 */
struct hba_prdt_entry
{
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9;
    uint32_t i:1;
} __attribute__((packed));

/**
 * @brief Command Table \struct hba_cmd_tbl
 */
struct hba_cmd_tbl
{
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    struct hba_prdt_entry prdt_entry[1];
} __attribute__((packed));

/**
 * @brief Received FIS structure
 */
struct hba_fis
{
    uint8_t dsfis[0x20];
    uint8_t rsv0[4];
    uint8_t psfis[0x20];
    uint8_t rsv1[12];
    uint8_t rfis[0x18];
    uint8_t rsv2[4];
    uint8_t sdbfis[8];
    uint8_t ufis[64];
    uint8_t rsv3[96];
} __attribute__((packed));

/**
 * @brief HBA Port structure \struct hba_port
 */
struct hba_port
{
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} __attribute__((packed));

/**
 * @brief HBA Memory structure \struct hba_mem
 */
struct hba_mem
{
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t rsv[0xA0-0x2C];
    uint8_t vendor[0x100-0xA0];
    struct hba_port ports[32];
} __attribute__((packed));

/**
 * @brief Initialize AHCI driver
 * @return 0 on success, negative on error
 */
int ahci_init(void);

/**
 * @brief Read sectors from AHCI drive
 * @param port Port number (0-31)
 * @param lba Logical block address
 * @param count Number of sectors to read
 * @param buffer Buffer to store read data
 * @return 0 on success, negative on error
 */
int ahci_read_sectors(uint8_t port, uint64_t lba, uint16_t count, void* buffer);

/**
 * @brief Write sectors to AHCI drive
 * @param port Port number (0-31)
 * @param lba Logical block address
 * @param count Number of sectors to write
 * @param buffer Buffer containing data to write
 * @return 0 on success, negative on error
 */
int ahci_write_sectors(uint8_t port, uint64_t lba, uint16_t count, const void* buffer);

/**
 * @brief Check if a port has a device attached
 * @param port Port number (0-31)
 * @return true if device exists, false otherwise
 */
bool ahci_port_exists(uint8_t port);

/**
 * @brief Get device size in sectors
 * @param port Port number (0-31)
 * @return Number of sectors, or 0 if port doesn't exist
 */
uint64_t ahci_get_port_size(uint8_t port);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_AHCI_H
