#include "pci.h"
#include "arch/i686/arch.h"
#include "../../shared/log.h"
#include "mm/heap.h"
#include "../../shared/string.h"

static struct pci_device* pci_device_list = NULL;

static const char* pci_class_names[] =
{
    "Unclassified",
    "Mass Storage",
    "Network",
    "Display",
    "Multimedia",
    "Memory",
    "Bridge",
    "Communication",
    "Peripheral",
    "Input Device",
    "Docking Station",
    "Processor",
    "Serial Bus",
    "Wireless",
    "Intelligent I/O",
    "Satellite",
    "Encryption",
    "Signal Processing"
};

uint8_t pci_config_read_byte(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return (uint8_t)((inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF);
}

uint16_t pci_config_read_word(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint32_t pci_config_read_dword(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write_byte(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset, const uint8_t value)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    const uint8_t shift = (offset & 3) * 8;
    data &= ~(0xFF << shift);
    data |= ((uint32_t)value << shift);
    outl(PCI_CONFIG_DATA, data);
}

void pci_config_write_word(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset, const uint16_t value)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    const uint8_t shift = (offset & 2) * 8;
    data &= ~(0xFFFF << shift);
    data |= ((uint32_t)value << shift);
    outl(PCI_CONFIG_DATA, data);
}

void pci_config_write_dword(const uint8_t bus, const uint8_t device, const uint8_t function, const uint8_t offset, const uint32_t value)
{
    const uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static void pci_check_function(const uint8_t bus, const uint8_t device, const uint8_t function)
{
    const uint16_t vendor_id = pci_config_read_word(bus, device, function, PCI_REG_VENDOR_ID);

    if (vendor_id == PCI_VENDOR_INVALID)
    {
        return;
    }

    struct pci_device* dev = (struct pci_device*)kmalloc(sizeof(struct pci_device));
    if (!dev)
    {
        log_error("PCI: Failed to allocate device structure");
        return;
    }

    memset(dev, 0, sizeof(struct pci_device));

    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    dev->vendor_id = vendor_id;
    dev->device_id = pci_config_read_word(bus, device, function, PCI_REG_DEVICE_ID);
    dev->class_code = pci_config_read_byte(bus, device, function, PCI_REG_CLASS);
    dev->subclass = pci_config_read_byte(bus, device, function, PCI_REG_SUBCLASS);
    dev->prog_if = pci_config_read_byte(bus, device, function, PCI_REG_PROG_IF);
    dev->revision = pci_config_read_byte(bus, device, function, PCI_REG_REVISION);
    dev->interrupt_line = pci_config_read_byte(bus, device, function, PCI_REG_INTERRUPT_LINE);
    dev->interrupt_pin = pci_config_read_byte(bus, device, function, PCI_REG_INTERRUPT_PIN);

    for (int i = 0; i < 6; i++)
    {
        dev->bar[i] = pci_config_read_dword(bus, device, function, PCI_REG_BAR0 + (i * 4));
    }

    dev->next = pci_device_list;
    pci_device_list = dev;

    log_info_fmt("PCI: %d:%d.%d - Vendor: 0x%x Device: 0x%x Class: 0x%x Sub: 0x%x",
                 bus, device, function, vendor_id, dev->device_id,
                 dev->class_code, dev->subclass);
}

static void pci_check_device(const uint8_t bus, const uint8_t device)
{
    uint16_t vendor_id = pci_config_read_word(bus, device, 0, PCI_REG_VENDOR_ID);

    if (vendor_id == PCI_VENDOR_INVALID)
    {
        return;
    }

    pci_check_function(bus, device, 0);

    const uint8_t header_type = pci_config_read_byte(bus, device, 0, PCI_REG_HEADER_TYPE);
    if (header_type & 0x80)
    {
        for (uint8_t function = 1; function < PCI_MAX_FUNCTION; function++)
        {
            vendor_id = pci_config_read_word(bus, device, function, PCI_REG_VENDOR_ID);
            if (vendor_id != PCI_VENDOR_INVALID)
            {
                pci_check_function(bus, device, function);
            }
        }
    }
}

static void pci_check_bus(const uint8_t bus)
{
    for (uint8_t device = 0; device < PCI_MAX_DEVICE; device++)
    {
        pci_check_device(bus, device);
    }
}

void pci_init(void)
{
    log_info("PCI: Initializing PCI bus enumeration");

    pci_device_list = NULL;

    const uint8_t header_type = pci_config_read_byte(0, 0, 0, PCI_REG_HEADER_TYPE);

    if ((header_type & 0x80) == 0)
    {
        pci_check_bus(0);
    }
    else
    {
        for (uint8_t function = 0; function < PCI_MAX_FUNCTION; function++)
        {
            const uint16_t vendor_id = pci_config_read_word(0, 0, function, PCI_REG_VENDOR_ID);
            if (vendor_id != PCI_VENDOR_INVALID)
            {
                pci_check_bus(function);
            }
        }
    }

    int count = 0;
    const struct pci_device* dev = pci_device_list;
    while (dev)
    {
        count++;
        dev = dev->next;
    }

    log_info_fmt("PCI: Total devices found: %d", count);
}

struct pci_device* pci_get_devices(void)
{
    return pci_device_list;
}

struct pci_device* pci_find_device_by_class(const uint8_t class_code, const uint8_t subclass)
{
    struct pci_device* dev = pci_device_list;

    while (dev)
    {
        if (dev->class_code == class_code && dev->subclass == subclass)
        {
            return dev;
        }
        dev = dev->next;
    }

    return NULL;
}

struct pci_device* pci_find_device_by_id(const uint16_t vendor_id, const uint16_t device_id)
{
    struct pci_device* dev = pci_device_list;

    while (dev)
    {
        if (dev->vendor_id == vendor_id && dev->device_id == device_id)
        {
            return dev;
        }
        dev = dev->next;
    }

    return NULL;
}

uint32_t pci_get_bar(struct pci_device* dev, const uint8_t bar_index, uint8_t* is_io)
{
    if (!dev || bar_index >= 6)
    {
        return 0;
    }

    const uint32_t bar = dev->bar[bar_index];

    if (bar & 1)
    {
        if (is_io)
        {
            *is_io = 1;
        }
        return bar & 0xFFFFFFFC;
    }
    else
    {
        if (is_io)
        {
            *is_io = 0;
        }
        return bar & 0xFFFFFFF0;
    }
}

void pci_enable_bus_mastering(struct pci_device* dev)
{
    if (!dev)
    {
        return;
    }

    uint16_t command = pci_config_read_word(dev->bus, dev->device, dev->function, PCI_REG_COMMAND);
    command |= 0x04;
    pci_config_write_word(dev->bus, dev->device, dev->function, PCI_REG_COMMAND, command);

    log_info_fmt("PCI: Enabled bus mastering for %d:%d.%d", dev->bus, dev->device, dev->function);
}

void pci_list_devices(void)
{
    const struct pci_device* dev = pci_device_list;
    int count = 0;

    log_info("\nPCI Devices:\n");
    log_info("============\n");

    while (dev)
    {
        const char* class_name = "Unknown";
        if (dev->class_code < 18)
        {
            class_name = pci_class_names[dev->class_code];
        }

        log_info_fmt("PCI: %d:%d.%d - Vendor: 0x%x Device: 0x%x Class: 0x%x Sub: 0x%x",
                     dev->bus, dev->device, dev->function,
                     dev->vendor_id, dev->device_id,
                     dev->class_code, dev->subclass);
        log_info_fmt("Class Name: %s", class_name);
        count++;
        dev = dev->next;
    }

    log_info_fmt("PCI: Total devices listed: %d", count);
}

