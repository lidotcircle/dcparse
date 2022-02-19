#include "scalc/context.h"
#include "scalc/ast.h"
#include "scalc/scalc_error.h"
#include "scalc/defer.hpp"
#include <assert.h>
#include <optional>
using namespace std;


class SCalcFunctionState {
    bool   _returned;
    double _return_val;

public:
    SCalcFunctionState(): _returned(false), _return_val(0) {}

    bool returned() const {
        return this->_returned;
    }

    double value() const {
        assert(this->returned());
        return this->_return_val;
    }

    void returnit(double val) {
        assert(!this->_returned);
        this->_returned = true;
        this->_return_val = val;
    }
};

class SCalcLoopStatState {
    bool _continued;;
    bool _breaked;

public:
    SCalcLoopStatState(): _continued(false), _breaked(false) {}

    void continueit() {
        assert(!this->_continued && !this->_breaked);
        this->_continued = true;
    }
    void breakit() {
        assert(!this->_continued && !this->_breaked);
        this->_breaked = true;
    }

    bool continued() const {
        return this->_continued;
    }
    bool breaked() const {
        return this->_breaked;
    }
    bool stoped() const {
        return this->continued() || this->breaked();
    }
};

class ScopeContext {
    shared_ptr<SCalcLoopStatState> _loop;
    shared_ptr<SCalcFunctionState> _function;

    map<string,double> _variables;

public:
    ScopeContext() = delete;
    ScopeContext(
            shared_ptr<SCalcFunctionState> func,
            shared_ptr<SCalcLoopStatState> loop):
        _function(func), _loop(loop)
    {
    }

    shared_ptr<SCalcFunctionState>       function()       { return this->_function; }
    const shared_ptr<SCalcFunctionState> function() const { return this->_function; }
    void returnit(double);


    shared_ptr<SCalcLoopStatState>       loop()       { return this->_loop; }
    const shared_ptr<SCalcLoopStatState> loop() const { return this->_loop; }
    void continueit()
    {
        if (this->_loop == nullptr)
            throw SCalcBadContinueStatement();

        assert(!this->stoped());
        this->_loop->continueit();
    }
    void breakit()
    {
        if (this->_loop == nullptr)
            throw SCalcBadBreakStatement();

        assert(!this->_loop->stoped());
        this->_loop->breakit();
    }
    void executingLoopStat()
    {
        this->_loop = make_shared<SCalcLoopStatState>();
    }
    bool executingLoopStatFinished()
    {
        assert(this->_loop);
        const auto ret = this->_loop->breaked();
        this->_loop = nullptr;
        return ret;
    }

    bool stoped() const
    {
        if (this->_function && this->_function->returned())
            return true;

        if (this->_loop && this->_loop->stoped())
            return true;

        return false;
    }

    optional<double> get_id(const string& id) const
    {
        if (this->_variables.find(id) == this->_variables.end())
            return nullopt;

        return this->_variables.at(id);
    }
    void set_id(const string& id, double val)
    {
        this->_variables[id] = val;
    }
};

SCalcContext::SCalcContext(): 
    _stack({ make_shared<ScopeContext>(nullptr, nullptr) })
{
}

SCalcContext::~SCalcContext() {}

double SCalcContext::get_id(const string& id) const
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    auto func = back->function();

    for (auto scope=this->_stack.rbegin();scope!=this->_stack.rend();scope++) {
        auto& ss = *scope;
        if (ss->function() != func)
            break;

        auto val = ss->get_id(id);
        if (val.has_value())
            return val.value();
    }

    this->_stack.back()->set_id(id, 0);
    return 0;
}
void SCalcContext::set_id(const string& id, double _val)
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    auto func = back->function();

    for (auto scope=this->_stack.rbegin();scope!=this->_stack.rend();scope++) {
        auto& ss = *scope;
        if (ss->function() != func)
            break;

        auto val = ss->get_id(id);
        if (val.has_value()){
            ss->set_id(id, _val);
            return;
        }
    }

    this->_stack.back()->set_id(id, _val);
}

void SCalcContext::set_argument(const string& argname, double val)
{
    assert(!this->_stack.empty());
    assert(this->_stack.back()->function());

    this->_stack.back()->set_id(argname, val);
}

void SCalcContext::push_scope()
{
    auto back = this->_stack.back();

    auto func = back->function();
    auto loop = back->loop();

    this->_stack.push_back(make_shared<ScopeContext>(func, loop));
}
void SCalcContext::pop_scope()
{
    assert(this->_stack.size() > 1);
    this->_stack.pop_back();
}

void SCalcContext::add_function(const string& id, shared_ptr<SCalcFunction> func)
{
    this->_funcs[id] = func;
}
double SCalcContext::call_function(const string& id, vector<double> parameters)
{
    if (this->_funcs.find(id) == this->_funcs.end())
        throw SCalcUndefinedFunction(id);

    this->_stack.push_back(make_shared<ScopeContext>(make_shared<SCalcFunctionState>(), nullptr));
    auto b = this->_stack.back();
    auto d1 = defer([&]() {
        auto back = this->_stack.back();
        assert(b == back);
        this->_stack.pop_back();
        assert(b != this->_stack.back());
    });

    return this->_funcs[id]->call(parameters);
}

optional<double> SCalcContext::return_value() const
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();

    auto func = back->function();
    assert(func);

    if (func->returned()) {
        return func->value();
    } else {
        return nullopt;
    }
}

void SCalcContext::returnit(double val)
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();

    auto func = back->function();
    if (func == nullptr)
        throw SCalcBadReturnStatement();

    func->returnit(val);
}
void SCalcContext::continueit()
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    back->continueit();
}
void SCalcContext::breakit()
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    back->breakit();
}

void SCalcContext::executingLoopStat()
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    back->executingLoopStat();
}
bool SCalcContext::executingLoopStatFinished()
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    return back->executingLoopStatFinished();
}

bool SCalcContext::stoped() const
{
    assert(!this->_stack.empty());
    auto back = this->_stack.back();
    return back->stoped();
}
