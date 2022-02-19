#ifndef _SIMPLE_CALCULATOR_CONTEXT_H_
#define _SIMPLE_CALCULATOR_CONTEXT_H_

#include <memory>
#include <map>
#include <vector>
#include <optional>

class ScopeContext;
class SCalcLoopStatState;

class SCalcFunction
{
public:
    virtual double call(std::vector<double> parameters) const = 0;
    virtual ~SCalcFunction() = default;
};

class SCalcContext
{
private:
    std::vector<std::shared_ptr<ScopeContext>> _stack;
    std::map<std::string,std::shared_ptr<SCalcFunction>> _funcs;

public:
    SCalcContext();
    virtual ~SCalcContext();

    double get_id(const std::string& id) const;
    void   set_id(const std::string& id, double val);

    void set_argument(const std::string& argname, double val);

    void push_scope();
    void pop_scope();

    void   add_function (const std::string& id, std::shared_ptr<SCalcFunction> func);
    double call_function(const std::string& id, std::vector<double> parameters);

    std::optional<double> return_value() const;
    void returnit(double val);
    void continueit();
    void breakit();

    void executingLoopStat();
    bool executingLoopStatFinished(); // => breaked

    bool stoped() const;
};

#endif // _SIMPLE_CALCULATOR_CONTEXT_H_
