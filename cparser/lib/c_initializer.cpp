#include "c_initializer.h"
#include <stdexcept>
#include <algorithm>
using namespace std;
using namespace cparser;
using child_t = AggregateUnionObjectMemberTreeArray::child_t;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;
using DesignatorAccesser = AggregateUnionObjectMemberTree::DesignatorAccesser;

static bool is_type(child_t val)
{
    return std::holds_alternative<std::shared_ptr<ASTNodeVariableType>>(val);
}
static shared_ptr<ASTNodeVariableType> get_type(child_t val)
{
    return std::get<std::shared_ptr<ASTNodeVariableType>>(val);
}
static shared_ptr<AggregateUnionObjectMemberTree> get_tree(child_t val)
{
    return std::get<shared_ptr<AggregateUnionObjectMemberTree>>(val);
}


DesignatorAccesser::DesignatorAccesser(weak_ptr<AggregateUnionObjectMemberTree> root)
{
    this->_tree_list.push_back({ .m_node = root, .m_member_index = 0, .m_token = 0 });
}

std::shared_ptr<cparser::ASTNodeVariableType> DesignatorAccesser::type() const 
{
    auto& last = this->_tree_list.back();
    auto node = last.m_node.lock();
    if (last.m_member_index >= node->numOfChildren())
        return nullptr;

    auto child = node->child(last.m_member_index, last.m_token);
    if (is_type(child)) {
        return get_type(child);
    } else {
        auto tree = get_tree(child);
        return tree->type();
    }
}
void DesignatorAccesser::next()
{
    auto& last = this->_tree_list.back();
    auto node = last.m_node.lock();
    assert(last.m_member_index <= node->numOfChildren());
    if (last.m_member_index == node->numOfChildren())
        return;

    last.m_member_index++;
    if (last.m_member_index == node->numOfChildren() &&
        this->_tree_list.size() > 1) 
    {
        this->_tree_list.pop_back();
        this->next();
    }
}
bool DesignatorAccesser::down()
{
    auto& last = this->_tree_list.back();
    auto node = last.m_node.lock();
    assert(last.m_member_index <= node->numOfChildren());
    if (last.m_member_index == node->numOfChildren()) {
        assert(this->_tree_list.size() == 1 || node->numOfChildren() == 0);
        return false;
    }

    auto child = node->child(last.m_member_index, last.m_token);
    if (is_type(child)) {
        return false;
    } else {
        auto tree = get_tree(child);
        if (tree->numOfChildren() == 0) {
            return false;
        } else {
            this->_tree_list.push_back({ .m_node = tree, .m_member_index = 0, .m_token = 0 });
            return true;
        }
    }
}

bool DesignatorAccesser::any_initialized()
{
    auto& last = this->_tree_list.back();
    auto node = last.m_node.lock();
    if (last.m_member_index >= node->numOfChildren())
        return false;

    return node->any_initialized(last.m_member_index, last.m_token);
}
void DesignatorAccesser::initialize_pt()
{
    auto& last = this->_tree_list.back();
    auto node = last.m_node.lock();
    if (last.m_member_index >= node->numOfChildren())
        return;

    node->initialize(last.m_member_index, last.m_token);
}


/* static */
vector<AggregateUnionObjectMemberTree::NodeInfo>&
AggregateUnionObjectMemberTree::get_nodeinfo(DesignatorAccesser& accesser)
{
    return accesser._tree_list;
}

optional<DesignatorAccesser> AggregateUnionObjectMemberTree::access(const DesignatorList& designator_list)
{
    std::shared_ptr<AggregateUnionObjectMemberTree> tree;
    DesignatorAccesser accesser(tree);
    accesser._tree_list.clear();
    if (!this->access__(designator_list, 0, accesser))
        return nullopt;

    return accesser;
}


AggregateUnionObjectMemberTreeArray::AggregateUnionObjectMemberTreeArray(shared_ptr<cparser::ASTNodeVariableTypeArray> type)
{
    this->_type = type;
    if (type->array_size()) {
        auto val = type->array_size()->get_integer_constant();
        assert(val.has_value());
        this->_array_size = val.value();
    } else {
        this->_unknown_max_arraysize = 0;
    }
}

/** static */
shared_ptr<AggregateUnionObjectMemberTreeArray>
AggregateUnionObjectMemberTreeArray::create(shared_ptr<cparser::ASTNodeVariableTypeArray> type)
{
    auto ret = shared_ptr<AggregateUnionObjectMemberTreeArray>(new AggregateUnionObjectMemberTreeArray(type));
    ret->set_self(ret);
    return ret;
}

child_t AggregateUnionObjectMemberTreeArray::create_child() const
{
    auto elemtype = this->_type->elemtype();
    const auto bt = elemtype->basic_type();
    if (elemtype->is_scalar_type() || bt == variable_basic_type::ENUM)
    {
        return child_t(elemtype);
    } else if (bt == variable_basic_type::ARRAY) {
        auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(elemtype);
        return AggregateUnionObjectMemberTreeArray::create(arrtype);
    } else if (bt == variable_basic_type::STRUCT) {
        auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(elemtype);
        return AggregateUnionObjectMemberTreeStruct::create(structtype);
    } else if (bt == variable_basic_type::UNION) {
        auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(elemtype);
        return AggregateUnionObjectMemberTreeUnion::create(uniontype);
    } else {
        throw runtime_error("unsupported type");
    }
}

size_t  AggregateUnionObjectMemberTreeArray::numOfChildren() const
{
    if (this->_array_size.has_value()) {
        return this->_array_size.value();
    } else {
        assert(this->_unknown_max_arraysize.has_value());
        return 0xffffffff;
    }
}
child_t AggregateUnionObjectMemberTreeArray::child(size_t index, size_t)
{
    assert(index < this->numOfChildren());
    if (this->_members.find(index) == this->_members.end())
        this->_members[index] = this->create_child();

    if (this->_unknown_max_arraysize.has_value()) {
        const auto oldval = this->_unknown_max_arraysize.value();
        this->_unknown_max_arraysize = std::max(oldval, index + 1);
    }

    return this->_members[index];
}

void AggregateUnionObjectMemberTreeArray::set_self(std::weak_ptr<AggregateUnionObjectMemberTree> self)
{
    assert(this->_self.expired());
    this->_self = self;
    assert(this->_self.lock() && this->_self.lock().get() == this);
}

bool AggregateUnionObjectMemberTreeArray::any_initialized(size_t index, size_t) const
{
    auto _this = const_cast<AggregateUnionObjectMemberTreeArray*>(this);
    auto child = _this->child(index, 0);

    if (this->_any_initialized.find(index) != this->_any_initialized.end()) 
        return true;;

    if (is_type(child)) {
        return false;
    } else {
        auto tree = get_tree(child);
        auto treesize = tree->numOfChildren();
        for (size_t i=0;i<treesize;i++) {
            if (tree->any_initialized(i, 0)) {
                _this->_any_initialized.insert(i);
                return true;
            }
        }

        return false;
    }
}
void AggregateUnionObjectMemberTreeArray::initialize(size_t index, size_t)
{
    assert(index < this->numOfChildren());
    this->_any_initialized.insert(index);
    auto child = this->child(index, 0);

    if (!is_type(child)) {
        auto childtree = get_tree(child);
        for (size_t i=0;i<childtree->numOfChildren();i++)
            childtree->initialize(i, 0);
    }
}

bool AggregateUnionObjectMemberTreeArray::access__(const DesignatorList& designators, size_t index, DesignatorAccesser& accesser)
{
    assert(designators.size() > index);
    auto designator = designators[index];
    if (designator.is_named()) return false;

    auto idx = designator.get_index();
    if (idx >= this->numOfChildren()) return false;

    auto tree = AggregateUnionObjectMemberTree::get_nodeinfo(accesser);
    tree.push_back({ .m_node = this->_self, .m_member_index = idx, .m_token = 0 });
    auto child = this->child(idx, 0);
    if (index + 1 < designators.size()) {
        if (is_type(child)) return false;
        auto childtree = get_tree(child);
        return childtree->access__(designators, index + 1, accesser);
    } else {
        return true;
    }
}

optional<size_t> AggregateUnionObjectMemberTreeArray::get_unknown_array_size() const
{
    return this->_unknown_max_arraysize;
}

std::shared_ptr<cparser::ASTNodeVariableType> AggregateUnionObjectMemberTreeArray::type() const
{
    return this->_type;
}

DesignatorAccesser AggregateUnionObjectMemberTreeArray::first()
{
    assert(this->_self.lock());
    DesignatorAccesser accesser(this->_self);
    return accesser;
}


size_t  AggregateUnionObjectMemberTreeUnion::numOfChildren() const
{
    return this->_members.size() > 0 ? 1 : 0;
}

child_t AggregateUnionObjectMemberTreeUnion::child(size_t index, size_t token)
{
    assert(index == 0 && this->_members.size() > 0);

    for (auto& m : this->_members) {
        if (m.second.first == token)
            return m.second.second;
    }

    assert(false && "should not reach here, bad union access token");
}

void AggregateUnionObjectMemberTreeUnion::set_self(std::weak_ptr<AggregateUnionObjectMemberTree> self)
{
    assert(this->_self.expired());
    this->_self = self;
    assert(this->_self.lock() && this->_self.lock().get() == this);
}

bool AggregateUnionObjectMemberTreeUnion::any_initialized(size_t index, size_t) const
{
    assert(index == 0);
    if (this->_initialized)
        return true;

    for (auto& m : this->_members) {
        auto mval = m.second.second;
        if (!is_type(mval)) {
            auto childtree = get_tree(mval);
            for (size_t i=0;i<childtree->numOfChildren();i++) {
                if (childtree->any_initialized(i, 0)) {
                    auto _this = const_cast<AggregateUnionObjectMemberTreeUnion*>(this);
                    _this->initialize(0, 0);
                    return true;
                }
            }
        }
    }

    return false;
}

void AggregateUnionObjectMemberTreeUnion::initialize(size_t index, size_t)
{
    assert(index == 0);
    this->_initialized = true;

    for (auto& m : this->_members) {
        auto mval = m.second.second;
        if (!is_type(mval)) {
            auto childtree = get_tree(mval);
            for (size_t i=0;i<childtree->numOfChildren();i++) {
                childtree->initialize(i, 0);
            }
        }
    }
}

bool AggregateUnionObjectMemberTreeUnion::access__(
        const DesignatorList& designator_list, size_t designator_index, DesignatorAccesser& accesser)
{
    assert(designator_list.size() > designator_index);
    auto designator = designator_list[designator_index];
    if (designator.is_index()) return false;

    auto name = designator.get_name();
    if (this->_members.find(name) == this->_members.end())
        return false;

    auto& member = this->_members[name];
    auto& tree = AggregateUnionObjectMemberTree::get_nodeinfo(accesser);
    tree.push_back({ .m_node = this->_self, .m_member_index = 0, .m_token = member.first });

    if (designator_index + 1 < designator_list.size()) {
        if (is_type(member.second)) return false;
        auto childtree = get_tree(member.second);
        return childtree->access__(designator_list, designator_index + 1, accesser);
    } else {
        return true;
    }
}


AggregateUnionObjectMemberTreeUnion::AggregateUnionObjectMemberTreeUnion(
        shared_ptr<cparser::ASTNodeVariableTypeUnion> type)
{
    this->_type = type;
    auto defs = type->get_definition();
    for (auto& member: *defs) {
        auto id = member->get_id();
        if (!id) continue;
        auto member_type = member->get_type();
        if (member_type->is_incomplete_type()) continue;
        child_t child = member_type;
        const auto bt = member_type->basic_type();
        if (bt == variable_basic_type::STRUCT) {
            auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(member_type);
            child = AggregateUnionObjectMemberTreeStruct::create(structtype);
        } else if (bt == variable_basic_type::UNION) {
            auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(member_type);
            child = AggregateUnionObjectMemberTreeUnion::create(uniontype);
        } else if (bt == variable_basic_type::ARRAY) {
            auto arraytype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(member_type);
            child = AggregateUnionObjectMemberTreeArray::create(arraytype);
        }
        if (this->_members.find(id->id) != this->_members.end())
            continue;

        this->_members[id->id] = { this->_members.size(), child };
    }
}

/** static */
shared_ptr<AggregateUnionObjectMemberTreeUnion>
AggregateUnionObjectMemberTreeUnion::create(shared_ptr<ASTNodeVariableTypeUnion> type)
{
    auto ret = shared_ptr<AggregateUnionObjectMemberTreeUnion>(
            new AggregateUnionObjectMemberTreeUnion(type));
    ret->set_self(ret);
    return ret;
}

shared_ptr<ASTNodeVariableType> AggregateUnionObjectMemberTreeUnion::type() const
{
    return this->_type;
}

DesignatorAccesser AggregateUnionObjectMemberTreeUnion::first()
{
    assert(this->_self.lock());
    DesignatorAccesser accesser(this->_self);
    return accesser;
}


size_t  AggregateUnionObjectMemberTreeStruct::numOfChildren() const
{
    return this->_members.size();
}

child_t AggregateUnionObjectMemberTreeStruct::child(size_t index, size_t)
{
    assert(this->_members.size() > index);

    for (auto& m : this->_members) {
        if (m.second.first == index)
            return m.second.second;
    }

    assert(false && "should not reach here, bad union access token");
}

void AggregateUnionObjectMemberTreeStruct::set_self(std::weak_ptr<AggregateUnionObjectMemberTree> self)
{
    assert(this->_self.expired());
    this->_self = self;
    assert(this->_self.lock() && this->_self.lock().get() == this);
}

bool AggregateUnionObjectMemberTreeStruct::any_initialized(size_t index, size_t) const
{
    assert(index < this->_members.size());
    if (this->_initialized[index])
        return true;

    auto _this = const_cast<AggregateUnionObjectMemberTreeStruct*>(this);
    auto child = _this->child(index, 0);
    if (is_type(child))
        return false;

    auto childtree = get_tree(child);
    for (size_t i=0;i<childtree->numOfChildren();i++) {
        if (childtree->any_initialized(i, 0))
            return true;
    }

    return false;
}

void AggregateUnionObjectMemberTreeStruct::initialize(size_t index, size_t)
{
    assert(index < this->_members.size());
    this->_initialized[index] = true;

    auto child = this->child(index, 0);
    if (is_type(child)) return;

    auto child_tree = get_tree(child);
    for (size_t i=0;i<child_tree->numOfChildren();i++)
        child_tree->initialize(i, 0);
}

bool AggregateUnionObjectMemberTreeStruct::access__(
        const DesignatorList& designator_list, size_t designator_index, DesignatorAccesser& accesser)
{
    assert(designator_list.size() > designator_index);
    auto designator = designator_list[designator_index];
    if (designator.is_index()) return false;

    auto name = designator.get_name();
    if (this->_members.find(name) == this->_members.end())
        return false;

    auto& member = this->_members[name];
    auto& tree = AggregateUnionObjectMemberTree::get_nodeinfo(accesser);
    tree.push_back({ .m_node = this->_self, .m_member_index = member.first, .m_token = 0 });

    if (designator_index + 1 < designator_list.size()) {
        if (is_type(member.second)) return false;
        auto childtree = get_tree(member.second);
        return childtree->access__(designator_list, designator_index + 1, accesser);
    } else {
        return true;
    }
}


AggregateUnionObjectMemberTreeStruct::AggregateUnionObjectMemberTreeStruct(
        shared_ptr<ASTNodeVariableTypeStruct> type)
{
    this->_type = type;
    auto defs = type->get_definition();
    for (auto& member: *defs) {
        auto id = member->get_id();
        if (!id) continue;
        auto member_type = member->get_type();
        if (member_type->is_incomplete_type()) continue;
        child_t child = member_type;
        const auto bt = member_type->basic_type();
        if (bt == variable_basic_type::STRUCT) {
            auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(member_type);
            child = AggregateUnionObjectMemberTreeStruct::create(structtype);
        } else if (bt == variable_basic_type::UNION) {
            auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(member_type);
            child = AggregateUnionObjectMemberTreeUnion::create(uniontype);
        } else if (bt == variable_basic_type::ARRAY) {
            auto arraytype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(member_type);
            child = AggregateUnionObjectMemberTreeArray::create(arraytype);
        }
        if (this->_members.find(id->id) != this->_members.end())
            continue;

        this->_members[id->id] = { this->_members.size(), child };
    }
    this->_initialized = vector<bool>(this->_members.size(), false);
}

/** statc */
shared_ptr<AggregateUnionObjectMemberTreeStruct>
AggregateUnionObjectMemberTreeStruct::create(shared_ptr<ASTNodeVariableTypeStruct> type)
{
    auto ret = shared_ptr<AggregateUnionObjectMemberTreeStruct>(
            new AggregateUnionObjectMemberTreeStruct(type));
    ret->set_self(ret);
    return ret;
}

shared_ptr<ASTNodeVariableType> AggregateUnionObjectMemberTreeStruct::type() const
{
    return this->_type;
}

DesignatorAccesser AggregateUnionObjectMemberTreeStruct::first()
{
    assert(this->_self.lock());
    DesignatorAccesser accesser(this->_self);
    return accesser;
}


std::shared_ptr<AggregateUnionObjectMemberTree> 
cparser::create_initializer_tree(std::shared_ptr<cparser::ASTNodeVariableType> type)
{
    assert(type);
    const auto bt = type->basic_type();
    if (bt == variable_basic_type::ARRAY) {
        auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        return AggregateUnionObjectMemberTreeArray::create(arrtype);
    } else if (bt == variable_basic_type::STRUCT) {
        auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(type);
        return AggregateUnionObjectMemberTreeStruct::create(structtype);
    } else if (bt == variable_basic_type::UNION) {
        auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(type);
        return AggregateUnionObjectMemberTreeUnion::create(uniontype);
    } else {
        assert(false && "should not reach here");
    }
}

