# Write header
file(WRITE "${OUT_FILE}" "#include \"initrd.h\"\n")
file(APPEND "${OUT_FILE}" "#include \"../../shared/string.h\"\n\n")  # freestanding string.h

# Declare initrd_entries array
file(APPEND "${OUT_FILE}" "__attribute__((section(\".initrd.entries\")))\n")
file(APPEND "${OUT_FILE}" "struct initrd_entry initrd_entries[] = {\n")

foreach(elf ${INITRD_ELFS})
    get_filename_component(name ${elf} NAME)       # e.g., shell.elf
    get_filename_component(name_we ${elf} NAME_WE) # e.g., shell

    file(APPEND "${OUT_FILE}" "    {\n")
    file(APPEND "${OUT_FILE}" "        .name = \"${name}\",\n")
    file(APPEND "${OUT_FILE}" "        .data = _binary_${name_we}_elf_start,\n")
    file(APPEND "${OUT_FILE}" "        .size = 0,\n")
    file(APPEND "${OUT_FILE}" "    },\n")
endforeach()

file(APPEND "${OUT_FILE}" "};\n\n")

# Initialize sizes at runtime
file(APPEND "${OUT_FILE}" "void initrd_entries_init(void) {\n")
foreach(elf ${INITRD_ELFS})
    get_filename_component(name ${elf} NAME)
    get_filename_component(name_we ${elf} NAME_WE)

    file(APPEND "${OUT_FILE}" "    for (size_t i = 0; i < sizeof(initrd_entries)/sizeof(initrd_entries[0]); i++) {\n")
    file(APPEND "${OUT_FILE}" "        if (strcmp(initrd_entries[i].name, \"${name}\") == 0) {\n")
    file(APPEND "${OUT_FILE}" "            ((struct initrd_entry*)&initrd_entries[i])->size = _binary_${name_we}_elf_end - _binary_${name_we}_elf_start;\n")
    file(APPEND "${OUT_FILE}" "        }\n")
    file(APPEND "${OUT_FILE}" "    }\n")
endforeach()
file(APPEND "${OUT_FILE}" "}\n")
