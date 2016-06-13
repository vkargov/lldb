//===-- ObjectFileJIT.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"

#include "ObjectFileMono.h"

#include "lldb/Core/ArchSpec.h"
#include "lldb/Core/DataBuffer.h"
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/FileSpecList.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/RangeMap.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/Timer.h"
#include "lldb/Core/UUID.h"
#include "lldb/Core/dwarf.h"
#include "lldb/Host/Host.h"
#include "lldb/Host/FileSpec.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Target/Platform.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/SectionLoadList.h"
#include "lldb/Target/Target.h"

using namespace lldb;
using namespace lldb_private;

//
// Design:
// - One ObjectFileMono instance for each codegen region in the runtime (address range).
// - Dynamically extended when methods are registered by the runtime
// - SymbolVendorMono instances handle symbol info without an underlying SymbolFile.
// FIXME: Locking
//

void
ObjectFileMono::Initialize()
{
    PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                   GetPluginDescriptionStatic(),
                                   CreateInstance,
                                   CreateMemoryInstance,
                                   GetModuleSpecifications);
}

void
ObjectFileMono::Terminate()
{
    PluginManager::UnregisterPlugin (CreateInstance);
}


lldb_private::ConstString
ObjectFileMono::GetPluginNameStatic()
{
    static ConstString g_name("mono-jit");
    return g_name;
}

const char *
ObjectFileMono::GetPluginDescriptionStatic()
{
    return "Mono JIT code object file";
}

ObjectFile *
ObjectFileMono::CreateInstance (const lldb::ModuleSP &module_sp,
                                 DataBufferSP& data_sp,
                                 lldb::offset_t data_offset,
                                 const FileSpec* file,
                                 lldb::offset_t file_offset,
                                 lldb::offset_t length)
{
    return NULL;
}

static const char* MAGIC = { "MONO_JIT_OBJECT_FILE" };

ObjectFile *
ObjectFileMono::CreateMemoryInstance (const lldb::ModuleSP &module_sp,
									  DataBufferSP& data_sp,
									  const ProcessSP &process_sp,
									  lldb::addr_t header_addr)
{
    if (data_sp && data_sp->GetByteSize() > sizeof (MAGIC))
    {
        const uint8_t *magic = data_sp->GetBytes();

		if (memcmp (magic, MAGIC, strlen (MAGIC)) == 0) {
			std::auto_ptr<ObjectFileMono> objfile_ap(new ObjectFileMono(module_sp, data_sp, process_sp, header_addr));

			// Set module architecture to match the target
			ProcessInstanceInfo proc_info;
			bool res = process_sp->GetProcessInfo (proc_info);
			assert (res);
			objfile_ap->SetModulesArchitecture (proc_info.GetArchitecture ());

			return objfile_ap.release();
        }
    }
    return NULL;
}

size_t
ObjectFileMono::GetModuleSpecifications (const lldb_private::FileSpec& file,
                                        lldb::DataBufferSP& data_sp,
                                        lldb::offset_t data_offset,
                                        lldb::offset_t file_offset,
                                        lldb::offset_t length,
                                        lldb_private::ModuleSpecList &specs)
{
    return 0;
}

ObjectFileMono::ObjectFileMono (const lldb::ModuleSP &module_sp,
								lldb::DataBufferSP& header_data_sp,
								const lldb::ProcessSP &process_sp,
								addr_t header_addr) :
    ObjectFile(module_sp, process_sp, header_addr, header_data_sp),
	m_unwinders(),
    m_ranges(),
	m_id(0)
{
}

ObjectFileMono::~ObjectFileMono()
{
}

bool
ObjectFileMono::ParseHeader ()
{
    return false;
}

ByteOrder
ObjectFileMono::GetByteOrder () const
{
    return m_data.GetByteOrder();
}

bool
ObjectFileMono::IsExecutable() const
{
    return false;
}

uint32_t
ObjectFileMono::GetAddressByteSize () const
{
    return m_data.GetAddressByteSize();
}

typedef struct {
	char magic [32];
	uint64_t start;
	int32_t size;
	int id;
} CodeRegionEntry;

typedef struct {
	int op;
	int when;
	int reg;
	int val;
} XUnwindOp;

typedef struct {
	uint64_t code;
	int id;
	int region_id;
	int code_size;
	int nunwind_ops;
	int name_offset;
	int unwind_ops_offset;
} MethodEntry;

int
ObjectFileMono::GetMethodEntryRegion(void *buf, int size)
{
	MethodEntry *entry;

	entry = (MethodEntry*)buf;
	return entry->region_id;
}

int
ObjectFileMono::GetId(void)
{
	return m_id;
}

void
ObjectFileMono::AddMethod(void *buf, int size)
{
	uint8_t *p = (uint8_t*)buf;	
	MethodEntry *entry;
	char *name;
	XUnwindOp *unwind_ops;
	int i, ret_reg;

	// FIXME: 32/64 bit

    Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));

	entry = (MethodEntry*)p;
	name = (char*)(p + entry->name_offset);
	ret_reg = *(p + entry->unwind_ops_offset);
	unwind_ops = (XUnwindOp*)(p + entry->unwind_ops_offset + 1);

	if (log)
		log->Printf("ObjectFileMono::%s %s [%p-%p]", __FUNCTION__, name, (char*)entry->code, (char*)entry->code + entry->code_size);

	static int symbol_id = m_symtab_ap->GetNumSymbols ();

	auto section = GetSectionList (true)->GetSectionAtIndex (0);
	int offset = (addr_t)entry->code - (addr_t)section->GetFileAddress ();

	Symbol symbol(
            symbol_id,    // Symbol table index
            name,     // symbol name.
            false,      // is the symbol name mangled?
            eSymbolTypeCode, // Type of this symbol
            false,           // Is this globally visible?
            false,           // Is this symbol debug info?
            true,            // Is this symbol a trampoline?
            true,            // Is this symbol artificial?
			section, // Section in which this symbol is defined or null.
			offset,       // Offset in section or symbol value.
            entry->code_size,     // Size in bytes of this symbol.
            true,            // Size is valid
            false,           // Contains linker annotations?
            0);              // Symbol flags.
	int symbol_idx = m_symtab_ap->AddSymbol(symbol);
	m_symtab_ap->SectionFileAddressesChanged ();

	UnwindPlanSP plan (new UnwindPlan (lldb::eRegisterKindDWARF));
	plan->SetSourceName ("Mono JIT");
	//plan->SetSourcedFromCompiler (LazyBool (true));
	//plan->SetUnwindPlanValidAtAllInstructions (LazyBool (true));
	plan->SetReturnAddressRegister (ret_reg);

	UnwindPlan::Row *row = new UnwindPlan::Row ();

	int cfa_reg = -1;
	int cfa_offset = -1;
	int last_when = -1;
	bool skip_rest = false;
	for (i = 0; i < entry->nunwind_ops; ++i) {
		XUnwindOp *op = unwind_ops + i;

		if (op->when > last_when) {
			UnwindPlan::RowSP row_sp (row);
			plan->AppendRow (row_sp);
			row = new UnwindPlan::Row (*row);
		}

		row->SetOffset (op->when);
		switch (op->op) {
		case DW_CFA_def_cfa:
			row->GetCFAValue ().SetIsRegisterPlusOffset (op->reg, op->val);
			cfa_reg = op->reg;
			break;
		case DW_CFA_offset:
			row->SetRegisterLocationToAtCFAPlusOffset (op->reg, op->val, true);
			break;
		case DW_CFA_def_cfa_offset:
			row->GetCFAValue ().SetIsRegisterPlusOffset (cfa_reg, op->val);
			cfa_offset = op->val;
			break;
		case DW_CFA_def_cfa_register:
			row->GetCFAValue ().SetIsRegisterPlusOffset (op->reg, cfa_offset);
			cfa_reg = op->reg;
			break;
		case DW_CFA_lo_user:
		case DW_CFA_remember_state:
		case DW_CFA_restore_state:
			// FIXME:
			skip_rest = true;
			break;
		default:
			fprintf (stderr, "MISS: %x, reg=0x%x, val=0x%x, when=0x%x\n", op->op, op->reg, op->val, op->when);
			assert (0);
			break;
		}
		if (skip_rest)
			break;
		last_when = op->when;
	}

	UnwindPlan::RowSP row_sp (row);
	plan->AppendRow (row_sp);

	/*
	StreamFile s(stderr, false);
	plan->Dump (s, nullptr, 0);
	*/

	if (entry->nunwind_ops > 0)
		m_unwinders [symbol.GetAddressRef().GetFileAddress()] = plan;

	int id = m_ranges.GetSize ();
	MonoMethodInfo *method = new MonoMethodInfo (id, name, AddressRange (section, offset, entry->code_size), m_symtab_ap->SymbolAtIndex (symbol_idx));

	m_ranges.Append(RangeToMethod::Entry ((addr_t)entry->code, entry->code_size, method));
}

MonoMethodInfo*
ObjectFileMono::FindMethodByAddr (lldb::addr_t addr)
{
    const RangeToMethod::Entry *entry = m_ranges.FindEntryThatContains (addr);

	if (entry)
		return entry->data;
	else
		return NULL;
}

Symtab *
ObjectFileMono::GetSymtab()
{
	if (m_symtab_ap.get() != NULL)
		return m_symtab_ap.get ();

	DataExtractor reader(m_data);
	CodeRegionEntry entry;
	reader.ExtractBytes (0, sizeof (CodeRegionEntry), eByteOrderLittle, &entry);

	m_id = entry.id;

	Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));

	if (log)
		log->Printf("ObjectFileMono::%s added JIT object file %d for range [%p-%p]", __FUNCTION__, m_id, (uint8_t*)entry.start, (uint8_t*)entry.start + entry.size);

	m_sections_ap.reset(new SectionList());
	m_symtab_ap.reset(new Symtab(this));

	SectionSP section_sp (new Section(GetModule(),        // Module to which this section belongs.
									  this,               // ObjectFile to which this section belongs and should read section data from.
									  0,    // Section ID.
									  ConstString("jitted_code"),               // Section name.
									  eSectionTypeCode,          // Section type.
									  (offset_t)entry.start,     // VM address.
									  entry.size,            // VM size in bytes of this section.
									  0,   // Offset of this section in the file.
									  entry.size,          // Size of the section as found in the file.
									  1,          // Alignment of the section
									  0,    // Flags for this section.
									  0));// Number of host bytes per target byte
	m_sections_ap->AddSection (section_sp);

	return m_symtab_ap.get ();
}

bool
ObjectFileMono::IsStripped ()
{
    return false;
}

void
ObjectFileMono::CreateSections (SectionList &unified_section_list)
{
	unified_section_list = *m_sections_ap;
}

void
ObjectFileMono::Dump (Stream *s)
{
    ModuleSP module_sp(GetModule());
    if (module_sp)
    {
        lldb_private::Mutex::Locker locker(module_sp->GetMutex());
        s->Printf("%p: ", static_cast<void*>(this));
        s->Indent();
        s->PutCString("ObjectFileMono");

        ArchSpec arch;
        if (GetArchitecture(arch))
            *s << ", arch = " << arch.GetArchitectureName();

        s->EOL();
    }
}

bool
ObjectFileMono::GetUUID (lldb_private::UUID* uuid)
{
    return false;
}

uint32_t
ObjectFileMono::GetDependentModules (FileSpecList& files)
{
    files.Clear();
    return 0;
}

lldb_private::Address
ObjectFileMono::GetEntryPointAddress ()
{
    return Address();
}

lldb_private::Address
ObjectFileMono::GetHeaderAddress ()
{
    return Address();
}

ObjectFile::Type
ObjectFileMono::CalculateType()
{
    return eTypeJIT;
}

ObjectFile::Strata
ObjectFileMono::CalculateStrata()
{
    return eStrataJIT;
}

bool
ObjectFileMono::GetArchitecture (ArchSpec &arch)
{
    return false;
}

bool
ObjectFileMono::SetLoadAddress (Target &target,
                               lldb::addr_t value,
                               bool value_is_offset)
{
    size_t num_loaded_sections = 0;
    SectionList *section_list = GetSectionList ();
    if (section_list)
    {
        const size_t num_sections = section_list->GetSize();
        // "value" is an offset to apply to each top level segment
        for (size_t sect_idx = 0; sect_idx < num_sections; ++sect_idx)
        {
            SectionSP section_sp (section_list->GetSectionAtIndex (sect_idx));
            if (section_sp &&
                section_sp->GetFileSize() > 0 &&
                section_sp->IsThreadSpecific() == false)
            {
                if (target.GetSectionLoadList().SetSectionLoadAddress (section_sp, section_sp->GetFileAddress() + value), true)
                    ++num_loaded_sections;
            }
        }
    }
    return num_loaded_sections > 0;
}

    
size_t
ObjectFileMono::ReadSectionData (const lldb_private::Section *section,
                                lldb::offset_t section_offset,
                                void *dst,
                                size_t dst_len) const
{
    return 0;
}

size_t
ObjectFileMono::ReadSectionData (const lldb_private::Section *section,
                                lldb_private::DataExtractor& section_data) const
{
    return 0;
}

lldb::UnwindPlanSP
ObjectFileMono::GetUnwindPlan(lldb_private::AddressRange range, lldb::offset_t offset)
{
	Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));

	//fprintf (stderr, "GetUnwindPlan: %p\n", (uint8_t*)range.GetBaseAddress ().GetFileAddress ());
	auto iter = m_unwinders.find (range.GetBaseAddress ().GetFileAddress ());
	if (iter != m_unwinders.end ()) {
		lldb::UnwindPlanSP plan (iter->second);

		if (log) {
			log->Printf("ObjectFileMono::%s found unwind plan for: %p", __FUNCTION__, (uint8_t*)range.GetBaseAddress ().GetFileAddress ());

			StreamFile s(stderr, false);
			plan->Dump (s, nullptr, 0);
		}

		return plan;
	} else {
		return NULL;
	}
}

//------------------------------------------------------------------
// PluginInterface protocol
//------------------------------------------------------------------
lldb_private::ConstString
ObjectFileMono::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
ObjectFileMono::GetPluginVersion()
{
    return 1;
}
