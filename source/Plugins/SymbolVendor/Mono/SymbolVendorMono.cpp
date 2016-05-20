//===-- SymbolVendorMono.cpp ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SymbolVendorMono.h"

#include <string.h>

#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/Timer.h"
#include "lldb/Core/Log.h"
#include "lldb/Host/Host.h"
#include "lldb/Host/Symbols.h"
#include "lldb/Host/XML.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/SymbolContext.h"
#include "lldb/Symbol/CompileUnit.h"
#include "Plugins/ObjectFile/JIT/ObjectFileMono.h"

using namespace lldb;
using namespace lldb_private;

//
// SymbolVendor implementation for Mono.
//
// It loads symbol implementation from the info in the object file,
// without using an underlying SymbolFile.
// We can't use the default implementation, because it assumes that
// there is a SymbolFile, and it assumes that the number of compile units
// is constant.
//

//----------------------------------------------------------------------
// SymbolVendorMono constructor
//----------------------------------------------------------------------
SymbolVendorMono::SymbolVendorMono(const lldb::ModuleSP &module_sp) :
    SymbolVendor (module_sp)
{
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
SymbolVendorMono::~SymbolVendorMono()
{
}

void
SymbolVendorMono::Initialize()
{
    PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                   GetPluginDescriptionStatic(),
                                   CreateInstance);
}

void
SymbolVendorMono::Terminate()
{
    PluginManager::UnregisterPlugin (CreateInstance);
}


lldb_private::ConstString
SymbolVendorMono::GetPluginNameStatic()
{
    static ConstString g_name("mono");
    return g_name;
}

const char *
SymbolVendorMono::GetPluginDescriptionStatic()
{
    return "Symbol vendor for Mono.";
}

ConstString
SymbolVendorMono::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
SymbolVendorMono::GetPluginVersion()
{
    return 1;
}

SymbolVendor*
SymbolVendorMono::CreateInstance (const lldb::ModuleSP &module_sp, lldb_private::Stream *feedback_strm)
{
    if (!module_sp)
        return NULL;

    ObjectFile * obj_file = module_sp->GetObjectFile();
    if (!obj_file)
        return NULL;

    static ConstString obj_file_jit("mono-jit");
    ConstString obj_name = obj_file->GetPluginName();
    if (obj_name != obj_file_jit)
        return NULL;

    SymbolVendorMono* symbol_vendor = new SymbolVendorMono(module_sp);
	symbol_vendor->AddSymbolFileRepresentation(obj_file->shared_from_this());
	return symbol_vendor;
}

static void
enter_method (const char *name)
{
    Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));

	if (log)
		log->Printf("SymbolVendorMono::%s enter.", name);
}

#define ENTER enter_method(__FUNCTION__)

void
SymbolVendorMono::Dump(Stream *s)
{
	ENTER;
}

lldb::LanguageType
SymbolVendorMono::ParseCompileUnitLanguage (const SymbolContext& sc)
{
	ENTER;
	return eLanguageTypeUnknown;
}

size_t
SymbolVendorMono::ParseCompileUnitFunctions (const SymbolContext& sc)
{
	ENTER;
	return 0;
}

bool
SymbolVendorMono::ParseCompileUnitLineTable (const SymbolContext& sc)
{
	ENTER;
	return false;
}

bool
SymbolVendorMono::ParseCompileUnitDebugMacros (const SymbolContext& sc)
{
	ENTER;
	return false;
}

bool
SymbolVendorMono::ParseCompileUnitSupportFiles (const SymbolContext& sc,
												FileSpecList& support_files)
{
	ENTER;
	return false;
}

bool
SymbolVendorMono::ParseImportedModules (const SymbolContext &sc,
										std::vector<ConstString> &imported_modules)
{
	ENTER;
	return false;
}

size_t
SymbolVendorMono::ParseFunctionBlocks (const SymbolContext& sc)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::ParseTypes (const SymbolContext& sc)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::ParseVariablesForContext (const SymbolContext& sc)
{
	ENTER;
	return 0;
}

Type*
SymbolVendorMono::ResolveTypeUID(lldb::user_id_t type_uid)
{
	ENTER;
	return NULL;
}

//
// This method maps from an address to its symbol info
// FIXME: If this returns 0, the symbol is not found, even if
// ObjectFile has information about it.
//

uint32_t
SymbolVendorMono::ResolveSymbolContext (const Address& so_addr,
										uint32_t resolve_scope,
										SymbolContext& sc)
{
    Log *log(GetLogIfAnyCategoriesSet(LIBLLDB_LOG_JIT_LOADER));

	if (log)
		log->Printf("SymbolVendorMono::%s %p.", __FUNCTION__, so_addr.GetFileAddress ());

	return 0;
}

uint32_t
SymbolVendorMono::ResolveSymbolContext (const FileSpec& file_spec,
										uint32_t line,
										bool check_inlines,
										uint32_t resolve_scope,
										SymbolContextList& sc_list)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindGlobalVariables (const ConstString &name,
									   const CompilerDeclContext *parent_decl_ctx,
									   bool append,
									   size_t max_matches,
									   VariableList& variables)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindGlobalVariables (const RegularExpression& regex,
									   bool append,
									   size_t max_matches,
									   VariableList& variables)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindFunctions (const ConstString &name,
								 const CompilerDeclContext *parent_decl_ctx,
								 uint32_t name_type_mask,
								 bool include_inlines,
								 bool append,
								 SymbolContextList& sc_list)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindFunctions (const RegularExpression& regex,
								 bool include_inlines,
								 bool append,
								 SymbolContextList& sc_list)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindTypes (const SymbolContext& sc,
							 const ConstString &name,
							 const CompilerDeclContext *parent_decl_ctx, 
							 bool append, 
							 size_t max_matches,
							 llvm::DenseSet<lldb_private::SymbolFile *> &searched_symbol_files,
							 TypeMap& types)
{
	ENTER;
	return 0;
}

size_t
SymbolVendorMono::FindTypes (const std::vector<CompilerContext> &context, bool append, TypeMap& types)
{
	ENTER;
	return 0;
}

CompilerDeclContext
SymbolVendorMono::FindNamespace (const SymbolContext& sc, 
								 const ConstString &name,
								 const CompilerDeclContext *parent_decl_ctx)
{
	ENTER;
	return CompilerDeclContext ();
}

size_t
SymbolVendorMono::GetNumCompileUnits()
{
	ENTER;
	return 0;
}

bool
SymbolVendorMono::SetCompileUnitAtIndex (size_t cu_idx,
										 const lldb::CompUnitSP &cu_sp)
{
	ENTER;
	return false;
}

lldb::CompUnitSP
SymbolVendorMono::GetCompileUnitAtIndex(size_t idx)
{
	ENTER;
	return CompUnitSP ();
}

size_t
SymbolVendorMono::GetTypes (SymbolContextScope *sc_scope,
							uint32_t type_mask,
							TypeList &type_list)
{
	ENTER;
	return 0;
}

// Get module unified section list symbol table.
Symtab *
SymbolVendorMono::GetSymtab ()
{
	ENTER;
	return NULL;
}

// Clear module unified section list symbol table.
void
SymbolVendorMono::ClearSymtab ()
{
	ENTER;
}

//------------------------------------------------------------------
/// Notify the SymbolVendor that the file addresses in the Sections
/// for this module have been changed.
//------------------------------------------------------------------
void
SymbolVendorMono::SectionFileAddressesChanged ()
{
	ENTER;
}
