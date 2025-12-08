#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H

#include "../include/types.h"

#define ACPI_RSDP_SIGNATURE "RDS PTR "
#define ACPI_RSDP_ALIGN 16

#define ACPI_SIG_RSDT   0x54445352
#define ACPI_SIG_XSDT   0x54445358
#define ACPI_SIG_MADT   0x43495041
#define ACPI_SIG_FADT   0x50434146
#define ACPI_SIG_HPET   0x54455048
#define ACPI_SIG_MCFG   0x4746434D
#define ACPI_SIG_DSDT   0x54445344

#define ACPI_MADT_TYPE_LOCAL_APIC       0
#define ACPI_MADT_TYPE_IO_APIC          1
#define ACPI_MADT_TYPE_INT_OVERRIDE     2
#define ACPI_MADT_TYPE_NMI              3
#define ACPI_MADT_TYPE_LOCAL_APIC_NMI   4

/**
 * @brief RSDP struct (root sys despr ptr) \struct acpi_rdsp
 */
struct acpi_rdsp
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    // ACPI 2.0+ fields
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

/**
 * @brief ACPI SDT header \struct acpi_sdt_header
 */
struct acpi_sdt_header
{
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

/**
 * @brief RSDT struct \struct acpi_rsdt
 */
struct acpi_rsdt
{
    struct acpi_sdt_header header;
    uint32_t tables[];
} __attribute__((packed));

/**
 * @brief MADT header (Multiple APIC Description Table) \struct acpi_madt
 */
struct acpi_madt
{
    struct acpi_sdt_header header;
    uint32_t local_apic_address;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed));

/**
 * @brief MADT entry header \struct acpi_madt_entry
 */
struct acpi_madt_entry
{
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

/**
 * @brief MADT Local APIC entry \struct acpi_madt_local_apic
 */
struct acpi_madt_local_apic
{
    struct acpi_madt_entry header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

/**
 * @brief MADT I/O APIC entry \struct acpi_madt_io_apic
 */
struct acpi_madt_io_apic
{
    struct acpi_madt_entry header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

/**
 * @brief FADT structure (Fixed ACPI Description Table) \struct acpi_fadt
 */
struct acpi_fadt
{
    struct acpi_sdt_header header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
} __attribute__((packed));

/**
 * @brief Initialize ACPI subsystem and parse tables
 */
void acpi_init(void);

/**
 * @brief Check if ACPI is available
 * @return bool true if ACPI tables found, false otherwise
 */
bool acpi_is_available(void);

/**
 * @brief Find ACPI table by signature
 * @param signature 4-byte table signature (e.g., ACPI_SIG_MADT)
 * @return struct acpi_sdt_header* Pointer to table header, NULL if not found
 */
struct acpi_sdt_header* acpi_find_table(uint32_t signature);

/**
 * @brief Get number of detected CPUs from MADT
 * @return uint32_t Number of CPUs
 */
uint32_t acpi_get_cpu_count(void);

/**
 * @brief Get Local APIC base address from MADT
 * @return uint32_t Physical address of Local APIC
 */
uint32_t acpi_get_local_apic_address(void);

/**
 * @brief Get I/O APIC base address from MADT
 * @return uint32_t Physical address of I/O APIC, 0 if not found
 */
uint32_t acpi_get_io_apic_address(void);

/**
 * @brief Print all detected ACPI tables
 */
void acpi_list_tables(void);


#endif // KERNEL_ACPI_H