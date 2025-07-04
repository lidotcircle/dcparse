#ifndef _C_PARSER_TRANSLATION_UNIT_CONTEXT_H_
#define _C_PARSER_TRANSLATION_UNIT_CONTEXT_H_

#include "c_ast.h"
#include "c_reporter.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>


namespace cparser {

class CTranslationUnitContext
{
  private:
    class Scope
    {
      private:
        std::map<std::string, std::shared_ptr<ASTNodeVariableType>> m_decls;
        std::map<std::string, std::shared_ptr<ASTNodeVariableType>> m_typedefs;
        std::map<std::string, std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
            m_structs;
        std::map<std::string, std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
            m_unions;
        std::map<std::string, std::pair<size_t, std::shared_ptr<ASTNodeEnumeratorList>>> m_enums;
        std::shared_ptr<SemanticReporter> m_reporter;
        std::weak_ptr<CParserContext> m_pctx;
        inline std::shared_ptr<CParserContext> pcontext() const
        {
            return m_pctx.lock();
        }

      public:
        Scope(std::shared_ptr<SemanticReporter> reporter, std::weak_ptr<CParserContext> pctx);

        std::shared_ptr<ASTNodeVariableType> lookup_variable(const std::string& varname);
        std::shared_ptr<ASTNodeVariableType> lookup_typedef(const std::string& typedef_name);
        std::optional<std::pair<size_t, std::shared_ptr<ASTNodeEnumeratorList>>>
        lookup_enum(const std::string& enum_name);
        std::optional<std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
        lookup_struct(const std::string& struct_name);
        std::optional<std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
        lookup_union(const std::string& union_name);

        void declare_variable(const std::string& varname,
                              std::shared_ptr<ASTNodeVariableType> type);
        void declare_typedef(const std::string& typedef_name,
                             std::shared_ptr<ASTNodeVariableType> type);
        void declare_enum(const std::string& enum_name);
        void declare_struct(const std::string& struct_name);
        void declare_union(const std::string& struct_name);

        void define_enum(const std::string& enum_name,
                         std::shared_ptr<ASTNodeEnumeratorList> enum_node);
        void define_struct(const std::string& struct_name,
                           std::shared_ptr<ASTNodeStructUnionDeclarationList> struct_node);
        void define_union(const std::string& union_name,
                          std::shared_ptr<ASTNodeStructUnionDeclarationList> union_node);

        std::optional<int> resolve_enum_constant(const std::string& enumtag, const std::string& id);
    };
    std::shared_ptr<SemanticReporter> m_reporter;
    std::weak_ptr<CParserContext> m_pctx;
    std::optional<std::shared_ptr<ASTNodeVariableType>> m_return_type;
    bool m_function_fake_scope;
    std::vector<Scope> m_scopes;
    std::set<std::string> m_defined_functions;
    std::set<std::string> m_idlabels;
    size_t m_loop_level_counter;
    struct SwitchStatInfo
    {
        std::set<long> m_case_values;
        bool m_has_default;
    };
    std::vector<SwitchStatInfo> m_switch_info;
    size_t m_struct_level_counter;
    inline std::shared_ptr<CParserContext> pcontext() const
    {
        return m_pctx.lock();
    }

  public:
    CTranslationUnitContext(std::shared_ptr<SemanticReporter> reporter,
                            std::shared_ptr<CParserContext> pctx);

    std::shared_ptr<ASTNodeVariableType> lookup_variable(const std::string& varname);
    std::shared_ptr<ASTNodeVariableType> lookup_typedef(const std::string& typedef_name);
    std::optional<std::pair<size_t, std::shared_ptr<ASTNodeEnumeratorList>>>
    lookup_enum(const std::string& enum_name);
    std::optional<std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
    lookup_struct(const std::string& struct_name);
    std::optional<std::pair<size_t, std::shared_ptr<ASTNodeStructUnionDeclarationList>>>
    lookup_union(const std::string& union_name);

    void function_begin(const std::string& funcname,
                        std::shared_ptr<ASTNodeVariableType> ret_type,
                        std::shared_ptr<ASTNodeParameterDeclarationList> parameter_list);
    std::shared_ptr<ASTNodeVariableType> return_type();
    bool in_function_definition();
    void function_end();

    void enter_loop();
    void leave_loop();
    void enter_switch();
    void leave_switch();
    bool breakable();
    bool continueable();

    bool in_switch_statement() const;
    bool add_caselabel(long long val);
    bool add_defaultlabel();

    void enter_scope();
    void leave_scope();

    void enter_struct_union_definition();
    void leave_struct_union_definition();
    bool in_struct_union_definition() const;

    void add_idlabel(const std::string& label);
    bool is_valid_idlabel(const std::string& label);

    void declare_variable(const std::string& varname, std::shared_ptr<ASTNodeVariableType> type);
    void declare_typedef(const std::string& typedef_name,
                         std::shared_ptr<ASTNodeVariableType> type);
    void declare_enum(const std::string& enum_name);
    void declare_struct(const std::string& struct_name);
    void declare_union(const std::string& struct_name);

    void define_enum(const std::string& enum_name,
                     std::shared_ptr<ASTNodeEnumeratorList> enum_node);
    void define_struct(const std::string& struct_name,
                       std::shared_ptr<ASTNodeStructUnionDeclarationList> struct_node);
    void define_union(const std::string& union_name,
                      std::shared_ptr<ASTNodeStructUnionDeclarationList> union_node);

    std::optional<int> resolve_enum_constant(const std::string& enumtag, const std::string& id);
};

} // namespace cparser

#endif // _C_PARSER_TRANSLATION_UNIT_CONTEXT_H_
