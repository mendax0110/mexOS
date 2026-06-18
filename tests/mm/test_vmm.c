#include "test_vmm.h"
#include "../../kernel/mm/vmm.h"
#include "../kernel/mm/pmm.h"
#include "../include/cast.h"

TEST_CASE(vmm_map_unmap_page)
{
    int result = vmm_map_page(vmm_get_current_directory(), 0x400000, 0x100000, PAGE_PRESENT | PAGE_WRITE);
    TEST_ASSERT_EQ(result, 0);
    return TEST_PASS;
}

TEST_CASE(vmm_get_physical_address)
{
    vmm_map_page(vmm_get_current_directory(), 0x500000, 0x200000, PAGE_PRESENT | PAGE_WRITE);
    uint32_t phys_addr = vmm_get_physical_address(vmm_get_current_directory(), 0x500000);
    TEST_ASSERT_EQ(phys_addr, 0x200000);
    return TEST_PASS;
}

TEST_CASE(vmm_alloc_free_page)
{
    int result = vmm_alloc_page(vmm_get_current_directory(), 0x600000, PAGE_PRESENT | PAGE_WRITE);
    TEST_ASSERT_EQ(result, 0);
    vmm_free_page(vmm_get_current_directory(), 0x600000);
    uint32_t phys_addr = vmm_get_physical_address(vmm_get_current_directory(), 0x600000);
    TEST_ASSERT_EQ(phys_addr, 0);
    return TEST_PASS;
}

TEST_CASE(vmm_unmap_nonexistent_page)
{
    vmm_unmap_page(vmm_get_current_directory(), 0x700000);
    uint32_t phys_addr = vmm_get_physical_address(vmm_get_current_directory(), 0x700000);
    TEST_ASSERT_EQ(phys_addr, 0);
    return TEST_PASS;
}

TEST_CASE(physi_to_virt)
{
    void* phys = pmm_alloc_block();
    if (!phys)
    {
        return TEST_SKIP;
    }

    uint32_t phys_addr = PTR_TO_U32(phys);
    void* virt = phys_to_virt(phys_addr);
    TEST_ASSERT_NOT_NULL(virt);
    pmm_free_block(phys);
    return TEST_PASS;
}

TEST_CASE(vmm_clone_address_space)
{
    page_directory_t* src = vmm_create_address_space();
    if (!src) return TEST_SKIP;

    page_directory_t* clone = vmm_clone_address_space(src);
    TEST_ASSERT_NOT_NULL(clone);

    vmm_destroy_address_space(clone);
    vmm_destroy_address_space(src);
    return TEST_PASS;
}

static struct test_case vmm_cases[] = {
    TEST_ENTRY(vmm_map_unmap_page),
    TEST_ENTRY(vmm_get_physical_address),
    TEST_ENTRY(vmm_alloc_free_page),
    TEST_ENTRY(vmm_unmap_nonexistent_page),
    TEST_ENTRY(physi_to_virt),
    TEST_ENTRY(vmm_clone_address_space)
};

static struct test_suite vmm_suite = {
    .name = "VMM Test Suite",
    .cases = vmm_cases,
    .count = 6
};

struct test_suite* test_vmm_get_suite(void)
{
    return &vmm_suite;
}