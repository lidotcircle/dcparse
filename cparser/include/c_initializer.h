#ifndef _C_PARSER_INITIALIZER_H_
#define _C_PARSER_INITIALIZER_H_

#include <string>
#include <variant>
#include <map>
#include "./c_ast.h"

namespace cparser {


class Designator {
private:
    std::variant<std::string,size_t> _designator;

public:
    inline Designator(std::string name): _designator(name) {}
    inline Designator(size_t name): _designator(name) {}

    inline bool is_named() { return std::holds_alternative<std::string>(_designator); }
    inline bool is_index() { return std::holds_alternative<size_t>(_designator); }

    inline std::string get_name() { return std::get<std::string>(_designator); }
    inline size_t      get_index() { return std::get<size_t>(_designator); }
};
using DesignatorList = std::vector<Designator>;


class AggregateUnionObjectMemberTreeArray;
class AggregateUnionObjectMemberTreeStruct;
class AggregateUnionObjectMemberTreeUnion;
class AggregateUnionObjectMemberTree {
public:
    using child_t = std::variant<std::shared_ptr<AggregateUnionObjectMemberTree>,std::shared_ptr<cparser::ASTNodeVariableType>>;
    class DesignatorAccesser {
    private:
        friend class AggregateUnionObjectMemberTree;
        struct NodeInfo {
            std::weak_ptr<AggregateUnionObjectMemberTree> m_node;
            size_t m_member_index;
            size_t m_token;
        };
        std::vector<NodeInfo> _tree_list;

    public:
        DesignatorAccesser(std::weak_ptr<AggregateUnionObjectMemberTree> root);
        std::shared_ptr<cparser::ASTNodeVariableType> type() const;
        void next();
        bool down();

        bool any_initialized();
        void initialize_pt();
    };

protected:
    using NodeInfo = DesignatorAccesser::NodeInfo;
    static std::vector<NodeInfo>& get_nodeinfo(DesignatorAccesser& accesser);

    friend class DesignatorAccesser;
    friend class AggregateUnionObjectMemberTreeArray;
    friend class AggregateUnionObjectMemberTreeStruct;
    friend class AggregateUnionObjectMemberTreeUnion;
    virtual size_t  numOfChildren() const = 0;
    virtual child_t child(size_t index, size_t token) = 0;
    virtual void    set_self(std::weak_ptr<AggregateUnionObjectMemberTree>) = 0;

    virtual bool    any_initialized(size_t index, size_t token) const = 0;
    virtual void    initialize(size_t index, size_t token) = 0;
    virtual bool    access__(const DesignatorList& designator_list,
                             size_t disignator_index,
                             DesignatorAccesser& accesser) = 0;

public:
    virtual std::shared_ptr<cparser::ASTNodeVariableType> type() const = 0;

    virtual DesignatorAccesser first() = 0;
    std::optional<DesignatorAccesser> access(const DesignatorList&);

    virtual ~AggregateUnionObjectMemberTree() = default;
};

class AggregateUnionObjectMemberTreeArray: public AggregateUnionObjectMemberTree
{
public:
    using child_t = typename AggregateUnionObjectMemberTree::child_t;

protected:
    virtual size_t  numOfChildren() const override;
    virtual child_t child(size_t index, size_t) override;
    virtual void    set_self(std::weak_ptr<AggregateUnionObjectMemberTree>) override;

    virtual bool    any_initialized(size_t index, size_t) const override;
    virtual void    initialize(size_t index, size_t) override;
    virtual bool    access__(const DesignatorList& designator_list,
                             size_t disignator_index,
                             DesignatorAccesser& accesser) override;

private:
    std::shared_ptr<cparser::ASTNodeVariableTypeArray> _type;
    std::optional<size_t> _unknown_max_arraysize;
    std::optional<size_t> _array_size;
    std::map<size_t,child_t> _members;
    std::set<size_t> _any_initialized;
    std::weak_ptr<AggregateUnionObjectMemberTree> _self;
    child_t create_child() const;
    AggregateUnionObjectMemberTreeArray(std::shared_ptr<cparser::ASTNodeVariableTypeArray> type);

public:
    static std::shared_ptr<AggregateUnionObjectMemberTreeArray>
        create(std::shared_ptr<cparser::ASTNodeVariableTypeArray> type);

    std::optional<size_t> get_unknown_array_size() const;
    virtual std::shared_ptr<cparser::ASTNodeVariableType> type() const override;
    virtual DesignatorAccesser first() override;
};

class AggregateUnionObjectMemberTreeUnion: public AggregateUnionObjectMemberTree
{
public:
    using child_t = typename AggregateUnionObjectMemberTree::child_t;

protected:
    virtual size_t  numOfChildren() const override;
    virtual child_t child(size_t index, size_t token) override;
    virtual void    set_self(std::weak_ptr<AggregateUnionObjectMemberTree>) override;

    virtual bool    any_initialized(size_t index, size_t token) const override;
    virtual void    initialize(size_t index, size_t token) override;
    virtual bool    access__(const DesignatorList& designator_list,
                             size_t disignator_index,
                             DesignatorAccesser& accesser) override;

private:
    bool _initialized;
    std::shared_ptr<cparser::ASTNodeVariableTypeUnion> _type;
    std::map<std::string,std::pair<size_t,child_t>> _members;
    std::weak_ptr<AggregateUnionObjectMemberTree> _self;
    AggregateUnionObjectMemberTreeUnion(std::shared_ptr<cparser::ASTNodeVariableTypeUnion> type);

public:
    static std::shared_ptr<AggregateUnionObjectMemberTreeUnion>
        create(std::shared_ptr<cparser::ASTNodeVariableTypeUnion> type);

    virtual std::shared_ptr<cparser::ASTNodeVariableType> type() const override;

    virtual DesignatorAccesser first() override;
};

class AggregateUnionObjectMemberTreeStruct: public AggregateUnionObjectMemberTree
{
public:
    using child_t = typename AggregateUnionObjectMemberTree::child_t;

protected:
    virtual size_t  numOfChildren() const override;
    virtual child_t child(size_t index, size_t token) override;
    virtual void    set_self(std::weak_ptr<AggregateUnionObjectMemberTree>) override;

    virtual bool    any_initialized(size_t index, size_t token) const override;
    virtual void    initialize(size_t index, size_t token) override;
    virtual bool    access__(const DesignatorList& designator_list,
                             size_t disignator_index,
                             DesignatorAccesser& accesser) override;

private:
    std::vector<bool> _initialized;
    std::shared_ptr<cparser::ASTNodeVariableTypeStruct> _type;
    std::map<std::string,std::pair<size_t,child_t>> _members;
    std::weak_ptr<AggregateUnionObjectMemberTree> _self;
    AggregateUnionObjectMemberTreeStruct(std::shared_ptr<cparser::ASTNodeVariableTypeStruct> type);

public:
    static std::shared_ptr<AggregateUnionObjectMemberTreeStruct>
        create(std::shared_ptr<cparser::ASTNodeVariableTypeStruct> type);

    virtual std::shared_ptr<cparser::ASTNodeVariableType> type() const override;

    virtual DesignatorAccesser first() override;
};


std::shared_ptr<AggregateUnionObjectMemberTree> 
create_initializer_tree(std::shared_ptr<cparser::ASTNodeVariableType> type);

}
#endif // _C_PARSER_INITIALIZER_H_
