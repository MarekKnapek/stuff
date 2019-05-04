#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <cassert>


static constexpr char const s_bad_format[] = "Bad format.";
static constexpr char const s_unknown_error[] = "Unknown error.";


struct dos_header
{
	std::uint16_t m_signature;
	std::uint16_t m_last_size;
	std::uint16_t m_pages;
	std::uint16_t m_relocations;
	std::uint16_t m_header_size;
	std::uint16_t m_min_alloc;
	std::uint16_t m_max_alloc;
	std::uint16_t m_ss;
	std::uint16_t m_sp;
	std::uint16_t m_check_sum;
	std::uint16_t m_ip;
	std::uint16_t m_cs;
	std::uint16_t m_relocs;
	std::uint16_t m_overlay;
	std::uint16_t m_reserved_1[4];
	std::uint16_t m_oem_id;
	std::uint16_t m_oem_info;
	std::uint16_t m_reserved_2[10];
	std::uint16_t m_pe_offset;
};
static_assert(sizeof(dos_header) == 62, "");
static_assert(sizeof(dos_header) == 0x3e, "");

struct coff_header
{
	std::uint16_t m_machine;
	std::uint16_t m_sections;
	std::uint32_t m_date_time;
	std::uint32_t m_symbol_table;
	std::uint32_t m_symbol_table_len;
	std::uint16_t m_optional_header_len;
	std::uint16_t m_characteristics;
};
static_assert(sizeof(coff_header) == 20, "");
static_assert(sizeof(coff_header) == 0x14, "");

struct coff_optional_header_pe32_standard
{
	std::uint16_t m_signature;
	std::uint8_t m_linker_major;
	std::uint8_t m_linker_minor;
	std::uint32_t m_code_size;
	std::uint32_t m_initialized_size;
	std::uint32_t m_uninitialized_size;
	std::uint32_t m_entry_point;
	std::uint32_t m_code_base;
	std::uint32_t m_data_base;
};
static_assert(sizeof(coff_optional_header_pe32_standard) == 28, "");
static_assert(sizeof(coff_optional_header_pe32_standard) == 0x1c, "");

struct coff_optional_header_pe32_plus_standard
{
	std::uint16_t m_signature;
	std::uint8_t m_linker_major;
	std::uint8_t m_linker_minor;
	std::uint32_t m_code_size;
	std::uint32_t m_initialized_size;
	std::uint32_t m_uninitialized_size;
	std::uint32_t m_entry_point;
	std::uint32_t m_code_base;
};
static_assert(sizeof(coff_optional_header_pe32_plus_standard) == 24, "");
static_assert(sizeof(coff_optional_header_pe32_plus_standard) == 0x18, "");

struct coff_optional_header_pe32_windows_specific
{
	std::uint32_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_os_major;
	std::uint16_t m_os_minor;
	std::uint16_t m_image_major;
	std::uint16_t m_image_minor;
	std::uint16_t m_subsystem_major;
	std::uint16_t m_subsystem_minor;
	std::uint32_t m_reserved_1;
	std::uint32_t m_image_size;
	std::uint32_t m_headers_size;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint32_t m_stack_reserve;
	std::uint32_t m_stack_commit;
	std::uint32_t m_heap_reserve;
	std::uint32_t m_heap_commit;
	std::uint32_t m_reserved_2;
	std::uint32_t m_rva_count;
};
static_assert(sizeof(coff_optional_header_pe32_windows_specific) == 68, "");
static_assert(sizeof(coff_optional_header_pe32_windows_specific) == 0x44, "");

struct coff_optional_header_pe32_plus_windows_specific
{
	std::uint64_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_os_major;
	std::uint16_t m_os_minor;
	std::uint16_t m_image_major;
	std::uint16_t m_image_minor;
	std::uint16_t m_subsystem_major;
	std::uint16_t m_subsystem_minor;
	std::uint32_t m_reserved_1;
	std::uint32_t m_image_size;
	std::uint32_t m_headers_size;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint64_t m_stack_reserve;
	std::uint64_t m_stack_commit;
	std::uint64_t m_heap_reserve;
	std::uint64_t m_heap_commit;
	std::uint32_t m_reserved_2;
	std::uint32_t m_rva_count;
};
static_assert(sizeof(coff_optional_header_pe32_plus_windows_specific) == 88, "");
static_assert(sizeof(coff_optional_header_pe32_plus_windows_specific) == 0x58, "");

struct coff_optional_header_pe32
{
	coff_optional_header_pe32_standard m_standard;
	coff_optional_header_pe32_windows_specific m_windows;
};
static_assert(sizeof(coff_optional_header_pe32) == sizeof(coff_optional_header_pe32_standard) + sizeof(coff_optional_header_pe32_windows_specific), "");
static_assert(sizeof(coff_optional_header_pe32) == 96, "");
static_assert(sizeof(coff_optional_header_pe32) == 0x60, "");

struct coff_optional_header_pe32_plus
{
	coff_optional_header_pe32_plus_standard m_standard;
	coff_optional_header_pe32_plus_windows_specific m_windows;
};
static_assert(sizeof(coff_optional_header_pe32_plus) == sizeof(coff_optional_header_pe32_plus_standard) + sizeof(coff_optional_header_pe32_plus_windows_specific), "");
static_assert(sizeof(coff_optional_header_pe32_plus) == 112, "");
static_assert(sizeof(coff_optional_header_pe32_plus) == 0x70, "");

union coff_optional_header_both
{
	coff_optional_header_pe32 m_pe32;
	coff_optional_header_pe32_plus m_pe32_plus;
};

struct coff_optional_header
{
	bool m_is_pe32;
	coff_optional_header_both coff_optional_hdr;
};

struct data_directory
{
	std::uint32_t m_address;
	std::uint32_t m_size;
};
static_assert(sizeof(data_directory) == 8, "");
static_assert(sizeof(data_directory) == 0x8, "");

enum class data_directory_type
{
	eExportTable = 0,
	eImportTable,
	eResourceTable,
	eExceptionTable,
	eCertificateTable,
	eRelocationTable,
	eDebug,
	eArchitecture,
	eGlobalPtr,
	eTLSTable,
	eLoadConfigTable,
	eBoundImportTable,
	eImportAddressTable,
	eDelayImportDescriptor,
	eCLRHeader,
	eReserved
};

struct section_header
{
	std::array<std::uint8_t, 8> m_name;
	std::uint32_t m_virtual_size;
	std::uint32_t m_virtual_address;
	std::uint32_t m_raw_size;
	std::uint32_t m_raw_ptr;
	std::uint32_t m_relocations;
	std::uint32_t m_line_numbers;
	std::uint16_t m_relocation_count;
	std::uint16_t m_line_numbers_count;
	std::uint32_t m_characteristics;
};
static_assert(sizeof(section_header) == 40, "");
static_assert(sizeof(section_header) == 0x28, "");

struct directory_export
{
	std::uint32_t m_characteristics;
	std::uint32_t m_date_time;
	std::uint16_t m_major;
	std::uint16_t m_minor;
	std::uint32_t m_name;
	std::uint32_t m_ordinal_base;
	std::uint32_t m_function_count;
	std::uint32_t m_name_count;
	std::uint32_t m_functions_address;
	std::uint32_t m_names_address;
	std::uint32_t m_ordinals_address;
};
static_assert(sizeof(directory_export) == 40, "");
static_assert(sizeof(directory_export) == 0x28, "");


struct import_directory_entry
{
	std::uint32_t m_import_lookup_table;
	std::uint32_t m_date_time;
	std::uint32_t m_forwarder_chain;
	std::uint32_t m_name; // Relative to image base.
	std::uint32_t m_import_adress_table;
};
static_assert(sizeof(import_directory_entry) == 20, "");
static_assert(sizeof(import_directory_entry) == 0x14, "");

struct import_lookup_pe32
{
	std::uint32_t m_value;
};
static_assert(sizeof(import_lookup_pe32) == 4, "");
static_assert(sizeof(import_lookup_pe32) == 0x4, "");

struct import_lookup_pe32_plus
{
	std::uint64_t m_value;
};
static_assert(sizeof(import_lookup_pe32_plus) == 8, "");
static_assert(sizeof(import_lookup_pe32_plus) == 0x8, "");

struct import_hint_name
{
	std::uint16_t m_export_name_ptr_idx;
	char m_import_name[2];
};
static_assert(sizeof(import_hint_name) == 4, "");
static_assert(sizeof(import_hint_name) == 0x4, "");


#define VERIFY(X) do{ assert(X); if(!(X)){ throw s_bad_format; } }while(false)


int wmain(int const argc, wchar_t const* const* argv)
{
	try
	{
		std::ifstream ifs;
		ifs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		ifs.open(argc == 2 ? argv[1] : argv[0], std::ios_base::in | std::ios_base::binary);
		ifs.seekg(0, std::ios_base::end);
		auto const fs = ifs.tellg();
		static constexpr int const s_max_file_size = 512 * 1024 * 1024;
		VERIFY(fs <= s_max_file_size);
		std::uint32_t const file_size = static_cast<std::uint32_t>(fs);

		ifs.seekg(0, std::ios_base::beg);
		std::vector<std::uint8_t> fd;
		fd.resize(file_size);
		ifs.read(reinterpret_cast<char*>(fd.data()), file_size);
		std::uint8_t const* const file_data = fd.data();

		// headers

		VERIFY(file_size >= sizeof(dos_header));
		dos_header const& dos_hdr = *reinterpret_cast<dos_header const*>(file_data);
		static constexpr uint16_t const s_mz_signature = 0x5a4d;
		VERIFY(dos_hdr.m_signature == s_mz_signature);

		static constexpr std::uint32_t const s_pe_signature_len = 4;
		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len);
		std::uint32_t const& pe_signature = *reinterpret_cast<std::uint32_t const*>(file_data + dos_hdr.m_pe_offset);
		static constexpr uint32_t const s_pe_signature = 0x00004550;
		VERIFY(pe_signature == s_pe_signature);

		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header));
		coff_header const& coff_hdr = *reinterpret_cast<coff_header const*>(file_data + dos_hdr.m_pe_offset + s_pe_signature_len);
		static constexpr uint16_t const s_machine_type_386 = 0x014c;
		static constexpr uint16_t const s_machine_type_amd64 = 0x8664;
		VERIFY(coff_hdr.m_machine == s_machine_type_386 || coff_hdr.m_machine == s_machine_type_amd64);
		static constexpr uint16_t const s_max_coff_header_sections = 96;
		VERIFY(coff_hdr.m_sections <= s_max_coff_header_sections);

		static constexpr int const s_coff_optional_header_signature_len = 2;
		VERIFY(coff_hdr.m_optional_header_len >= s_coff_optional_header_signature_len);
		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + s_coff_optional_header_signature_len);
		std::uint16_t const& coff_optional_header_signature = *reinterpret_cast<std::uint32_t const*>(file_data + dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header));
		static constexpr uint16_t const s_coff_optional_type_pe32 = 0x010b;
		static constexpr uint16_t const s_coff_optional_type_pe32_plus = 0x020b;
		VERIFY(coff_optional_header_signature == s_coff_optional_type_pe32 || coff_optional_header_signature == s_coff_optional_type_pe32_plus);
		bool const is_pe32 = coff_optional_header_signature == s_coff_optional_type_pe32;

		VERIFY(coff_hdr.m_optional_header_len >= (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
		coff_optional_header_both const& coff_optional_hdr_both = *reinterpret_cast<coff_optional_header_both const*>(file_data + dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header));

		VERIFY((is_pe32 ? coff_optional_hdr_both.m_pe32.m_windows.m_rva_count : coff_optional_hdr_both.m_pe32_plus.m_windows.m_rva_count) == 16);
		VERIFY(coff_hdr.m_optional_header_len == (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
		std::array<data_directory, 16> const& data_directories = *reinterpret_cast<std::array<data_directory, 16> const*>(file_data + dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
		VERIFY(data_directories[static_cast<int>(data_directory_type::eArchitecture)].m_address == 0);
		VERIFY(data_directories[static_cast<int>(data_directory_type::eArchitecture)].m_size == 0);
		VERIFY(data_directories[static_cast<int>(data_directory_type::eReserved)].m_address == 0);
		VERIFY(data_directories[static_cast<int>(data_directory_type::eReserved)].m_size == 0);

		VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>) + coff_hdr.m_sections * sizeof(section_header));
		section_header const* const section_headers_begin = reinterpret_cast<section_header const*>(file_data + dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
		section_header const* const section_headers_end = section_headers_begin + coff_hdr.m_sections;
		for(std::uint32_t i = 0; i != coff_hdr.m_sections; ++i)
		{
			VERIFY(file_size >= section_headers_begin[i].m_raw_ptr + section_headers_begin[i].m_raw_size);
		}

		// end of headers

		auto const convert_rva_to_disk_ptr = [&](std::uint32_t const& rva, section_header const* const& section_in = nullptr) -> std::pair<section_header const*, std::uint32_t>
		{
			section_header const* section = nullptr;
			if(section_in)
			{
				section = section_in;
				VERIFY(rva >= section->m_virtual_address && rva < section->m_virtual_address + section->m_virtual_size);
			}
			else
			{
				for(std::uint32_t i = 0; i != coff_hdr.m_sections; ++i)
				{
					if(rva >= section_headers_begin[i].m_virtual_address && rva < section_headers_begin[i].m_virtual_address + section_headers_begin[i].m_virtual_size)
					{
						section = section_headers_begin + i;
						break;
					}
				}
				VERIFY(section);
			}
			return {section, (rva - section->m_virtual_address) + section->m_raw_ptr};
		};

		// export table

		std::printf("export table begin\n");
		std::printf("ordinal,address,hint,name\n");

		std::uint32_t const export_table_rva = data_directories[static_cast<int>(data_directory_type::eExportTable)].m_address;
		std::uint32_t const export_table_size = data_directories[static_cast<int>(data_directory_type::eExportTable)].m_size;
		VERIFY(export_table_size == 0 || export_table_size >= sizeof(directory_export));
		section_header const* const export_section = convert_rva_to_disk_ptr(export_table_rva).first;
		VERIFY(export_section);
		VERIFY(convert_rva_to_disk_ptr(export_table_rva + export_table_size - 1, export_section).first == export_section);
		VERIFY(file_size >= convert_rva_to_disk_ptr(export_table_rva + export_table_size - 1, export_section).second);
		std::uint32_t const export_dir_file_offset = convert_rva_to_disk_ptr(export_table_rva, export_section).second;
		VERIFY(file_size >= export_dir_file_offset + export_table_size);
		directory_export const& dir_export = *reinterpret_cast<directory_export const*>(file_data + export_dir_file_offset);

		std::uint32_t const export_module_name_offset = convert_rva_to_disk_ptr(dir_export.m_name).second;
		VERIFY(file_size >= export_module_name_offset);
		std::uint8_t const* const module_name = file_data + export_module_name_offset;
		std::uint32_t module_name_len = 0;
		for(std::uint32_t i = export_module_name_offset; i != file_size; ++i)
		{
			if(file_data[i] == 0)
			{
				module_name_len = i - export_module_name_offset;
				break;
			}
		}
		VERIFY(module_name_len == 0 || module_name_len <= 1024);

		bool const has_names = dir_export.m_names_address != 0;
		VERIFY(dir_export.m_name_count <= dir_export.m_function_count);
		std::uint32_t const table_name_addresses_offset = has_names ? convert_rva_to_disk_ptr(dir_export.m_names_address).second : 0u;
		VERIFY(file_size >= table_name_addresses_offset + dir_export.m_name_count * sizeof(std::uint32_t));
		std::uint32_t const* const table_name_addresses = has_names ? reinterpret_cast<std::uint32_t const*>(file_data + table_name_addresses_offset)  : nullptr;
		constexpr char const s_no_name[] = "";
		std::uint32_t const table_ordinals_offset = has_names ? convert_rva_to_disk_ptr(dir_export.m_ordinals_address, export_section).second : 0u;
		VERIFY(file_size >= table_ordinals_offset + dir_export.m_name_count * sizeof(std::uint16_t));
		std::uint16_t const* const table_ordinals = has_names ? reinterpret_cast<std::uint16_t const*>(file_data + table_ordinals_offset) : nullptr;

		std::uint32_t const table_export_address_offset = convert_rva_to_disk_ptr(dir_export.m_functions_address, export_section).second;
		VERIFY(file_size >= table_export_address_offset + dir_export.m_function_count * sizeof(std::uint32_t));
		std::uint32_t const* const table_export_address = reinterpret_cast<std::uint32_t const*>(file_data + table_export_address_offset);

		for(std::uint32_t i = 0; i != dir_export.m_function_count; ++i)
		{
			std::uint16_t const export_ordinal = i + dir_export.m_ordinal_base;
			std::uint32_t const export_address = table_export_address[i];
			char const* export_name = nullptr;
			std::uint32_t export_hint;
			for(std::uint32_t j = 0; j != dir_export.m_name_count; ++j)
			{
				if(table_ordinals[j] == i)
				{
					export_name =  reinterpret_cast<char const*>(file_data + convert_rva_to_disk_ptr(table_name_addresses[j]).second);
					export_hint = j;
					break;
				}
			}
			std::printf("%hu", static_cast<unsigned short int>(export_ordinal));
			if(export_address >= export_table_rva && export_address < export_table_rva + export_table_size)
			{
				std::printf(",%s", reinterpret_cast<char const*>(file_data + export_address));
			}
			else
			{
				std::printf(",%#x", static_cast<unsigned int>(export_address));
			}
			if(export_name)
			{
				std::printf(",%u", static_cast<unsigned int>(export_hint));
			}
			else
			{
				std::printf(",");
			}
			if(export_name)
			{
				std::printf(",%s\n", export_name);
			}
			else
			{
				std::printf(",\n");
			}
		}

		std::printf("export table end\n");

		// end of export table

		// import table

		std::printf("import table begin\n");

		std::uint32_t const import_table_addr = data_directories[static_cast<int>(data_directory_type::eImportTable)].m_address;
		std::uint32_t const import_table_size = data_directories[static_cast<int>(data_directory_type::eImportTable)].m_size;
		VERIFY(import_table_size == 0 || import_table_size >= sizeof(import_directory_entry));
		if(import_table_size != 0)
		{
			auto const import_addr = convert_rva_to_disk_ptr(import_table_addr);
			section_header const* const import_section = import_addr.first;
			std::uint32_t const import_table_file_offset = import_addr.second;
			VERIFY(file_size >= import_table_file_offset + sizeof(import_directory_entry));
			import_directory_entry const* const import_directory_table = reinterpret_cast<import_directory_entry const*>(file_data + import_table_file_offset);
			std::uint32_t import_directory_table_size = 1;
			while
			(
				!(
				import_directory_table[import_directory_table_size].m_import_lookup_table == 0 &&
				import_directory_table[import_directory_table_size].m_date_time == 0 &&
				import_directory_table[import_directory_table_size].m_forwarder_chain == 0 &&
				import_directory_table[import_directory_table_size].m_name == 0 &&
				import_directory_table[import_directory_table_size].m_import_adress_table == 0
				)
			)
			{
				VERIFY(file_size >= import_table_file_offset + import_directory_table_size * sizeof(import_directory_entry));
				++import_directory_table_size;
			};
			VERIFY(import_directory_table_size <= 1024);
			for(std::uint32_t i = 0; i != import_directory_table_size; ++i)
			{
				std::uint32_t const name_file_offset = import_directory_table[i].m_name;
				VERIFY(file_size >= name_file_offset);
				std::uint32_t j = 0;
				char const* const import_dll_name = reinterpret_cast<char const*>(file_data + convert_rva_to_disk_ptr(name_file_offset).second);
				while(import_dll_name[j] != '\0')
				{
					++j;
					VERIFY(file_size >= name_file_offset + j);
				}
				std::uint32_t const name_size = j;
				VERIFY(name_size <= 1024);
				std::printf("importing from %s\n", import_dll_name);

				std::printf("ordinal,hint,name\n");
				std::uint32_t const import_lookup_table_file_offset = import_directory_table[i].m_import_lookup_table;
				if(is_pe32)
				{
					VERIFY(file_size >= import_directory_table_size + sizeof(import_lookup_pe32));
					import_lookup_pe32 const* const import_lookup_table = reinterpret_cast<import_lookup_pe32 const*>(file_data + convert_rva_to_disk_ptr(import_lookup_table_file_offset).second);
					j = 0;
					while(!(import_lookup_table[j].m_value == 0u))
					{
						bool const is_ordinal = (import_lookup_table[j].m_value & 0x80000000u) == 0x80000000u;
						if(is_ordinal)
						{
							VERIFY((import_lookup_table[j].m_value & 0x7FFF0000u) == 0u);
							std::uint16_t const import_ordinal = import_lookup_table[j].m_value & 0x0000FFFFu;
							std::printf("%hu,,\n", static_cast<unsigned short int>(import_ordinal));
						}
						else
						{
							std::uint32_t const hint_name_offset = import_lookup_table[j].m_value &~ 0x80000000ul;
							VERIFY(file_size >= hint_name_offset + sizeof(import_hint_name));
							import_hint_name const* const hint_name = reinterpret_cast<import_hint_name const*>(file_data + convert_rva_to_disk_ptr(hint_name_offset).second);
							char const* const import_name = hint_name->m_import_name;
							std::uint32_t k = 0;
							while(!(import_name[k] == '\0'))
							{
								++k;
								VERIFY(file_size >= hint_name_offset + sizeof(std::uint16_t) + k);
							};
							VERIFY(k <= 1024);
							std::uint32_t const import_name_size = k;
							std::printf(",%hu,%s\n", static_cast<unsigned short int>(hint_name->m_export_name_ptr_idx), import_name);
						}
						++j;
					};
				}
				else
				{
					VERIFY(file_size >= import_directory_table_size + sizeof(import_lookup_pe32_plus));
					import_lookup_pe32_plus const* const import_lookup_table = reinterpret_cast<import_lookup_pe32_plus const*>(file_data + convert_rva_to_disk_ptr(import_lookup_table_file_offset).second);
					j = 0;
					while(!(import_lookup_table[j].m_value == 0ull))
					{
						bool const is_ordinal = (import_lookup_table[j].m_value & 0x8000000000000000ull) == 0x8000000000000000ull;
						if(is_ordinal)
						{
							VERIFY((import_lookup_table[j].m_value & 0x7FFFFFFFFFFF0000ull) == 0ull);
							std::uint16_t const import_ordinal = import_lookup_table[j].m_value & 0x000000000000FFFFull;
							std::printf("%hu,,\n", static_cast<unsigned short int>(import_ordinal));
						}
						else
						{
							VERIFY((import_lookup_table[j].m_value & 0x7FFFFFFF00000000ull) == 0ull);
							std::uint32_t const hint_name_offset = import_lookup_table[j].m_value &~ 0xFFFFFFFF80000000ull;
							VERIFY(file_size >= hint_name_offset + sizeof(import_hint_name));
							import_hint_name const* const hint_name = reinterpret_cast<import_hint_name const*>(file_data + convert_rva_to_disk_ptr(hint_name_offset).second);
							char const* const import_name = hint_name->m_import_name;
							std::uint32_t k = 0;
							while(!(import_name[k] == '\0'))
							{
								++k;
								VERIFY(file_size >= hint_name_offset + sizeof(std::uint16_t) + k);
							};
							VERIFY(k <= 1024);
							std::uint32_t const import_name_size = k;
							std::printf(",%hu,%s\n", static_cast<unsigned short int>(hint_name->m_export_name_ptr_idx), import_name);
						}
						++j;
					};
				}
			}
		}

		std::printf("import table end\n");

		// end of import table
	}
	catch(char const* const ex)
	{
		std::printf("%s\n", ex);
		return EXIT_FAILURE;
	}
	catch(std::exception const& ex)
	{
		std::cout << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch(...)
	{
		std::cout << s_unknown_error << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
