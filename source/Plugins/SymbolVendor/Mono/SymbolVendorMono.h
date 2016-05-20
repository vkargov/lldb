//===-- SymbolVendorMono.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_SymbolVendorMono_h_
#define liblldb_SymbolVendorMono_h_

#include "lldb/lldb-private.h"
#include "lldb/Symbol/SymbolVendor.h"

namespace lldb_private {

class SymbolVendorMono : public lldb_private::SymbolVendor
{
public:
    //------------------------------------------------------------------
    // Static Functions
    //------------------------------------------------------------------
    static void
    Initialize();

    static void
    Terminate();

    static lldb_private::ConstString
    GetPluginNameStatic();

    static const char *
    GetPluginDescriptionStatic();

    static lldb_private::SymbolVendor*
    CreateInstance (const lldb::ModuleSP &module_sp, lldb_private::Stream *feedback_strm);

    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    SymbolVendorMono(const lldb::ModuleSP &module_sp);

    ~SymbolVendorMono() override;

    void
    Dump(Stream *s) override;

    lldb::LanguageType
    ParseCompileUnitLanguage (const SymbolContext& sc) override;
    
    size_t
    ParseCompileUnitFunctions (const SymbolContext& sc) override;

    bool
    ParseCompileUnitLineTable (const SymbolContext& sc) override;

    bool
    ParseCompileUnitDebugMacros (const SymbolContext& sc) override;

    bool
    ParseCompileUnitSupportFiles (const SymbolContext& sc,
                                  FileSpecList& support_files) override;
    
    bool
    ParseImportedModules (const SymbolContext &sc,
                          std::vector<ConstString> &imported_modules) override;

    size_t
    ParseFunctionBlocks (const SymbolContext& sc) override;

    size_t
    ParseTypes (const SymbolContext& sc) override;

    size_t
    ParseVariablesForContext (const SymbolContext& sc) override;

    Type*
    ResolveTypeUID(lldb::user_id_t type_uid) override;

    uint32_t
    ResolveSymbolContext (const Address& so_addr,
                          uint32_t resolve_scope,
                          SymbolContext& sc) override;

    uint32_t
    ResolveSymbolContext (const FileSpec& file_spec,
                          uint32_t line,
                          bool check_inlines,
                          uint32_t resolve_scope,
                          SymbolContextList& sc_list) override;

    size_t
    FindGlobalVariables (const ConstString &name,
                         const CompilerDeclContext *parent_decl_ctx,
                         bool append,
                         size_t max_matches,
                         VariableList& variables) override;

    size_t
    FindGlobalVariables (const RegularExpression& regex,
                         bool append,
                         size_t max_matches,
                         VariableList& variables) override;

    size_t
    FindFunctions (const ConstString &name,
                   const CompilerDeclContext *parent_decl_ctx,
                   uint32_t name_type_mask,
                   bool include_inlines,
                   bool append,
                   SymbolContextList& sc_list) override;

    size_t
    FindFunctions (const RegularExpression& regex,
                   bool include_inlines,
                   bool append,
                   SymbolContextList& sc_list) override;

    size_t
    FindTypes (const SymbolContext& sc, 
               const ConstString &name,
               const CompilerDeclContext *parent_decl_ctx, 
               bool append, 
               size_t max_matches,
               llvm::DenseSet<lldb_private::SymbolFile *> &searched_symbol_files,
               TypeMap& types) override;

    size_t
    FindTypes (const std::vector<CompilerContext> &context, bool append, TypeMap& types) override;

    CompilerDeclContext
    FindNamespace (const SymbolContext& sc, 
                   const ConstString &name,
                   const CompilerDeclContext *parent_decl_ctx) override;
    
    size_t
    GetNumCompileUnits() override;

    bool
    SetCompileUnitAtIndex (size_t cu_idx,
                           const lldb::CompUnitSP &cu_sp) override;

    lldb::CompUnitSP
    GetCompileUnitAtIndex(size_t idx) override;

    size_t
    GetTypes (SymbolContextScope *sc_scope,
              uint32_t type_mask,
              TypeList &type_list) override;

    // Get module unified section list symbol table.
    Symtab *
    GetSymtab () override;

    // Clear module unified section list symbol table.
    void
    ClearSymtab () override;

    //------------------------------------------------------------------
    /// Notify the SymbolVendor that the file addresses in the Sections
    /// for this module have been changed.
    //------------------------------------------------------------------
    void
    SectionFileAddressesChanged () override;

    //------------------------------------------------------------------
    // PluginInterface protocol
    //------------------------------------------------------------------
    ConstString
    GetPluginName() override;

    uint32_t
    GetPluginVersion() override;

private:
    DISALLOW_COPY_AND_ASSIGN (SymbolVendorMono);
};

}

#endif  // liblldb_SymbolVendorMono_h_
