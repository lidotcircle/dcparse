#ifndef _DC_PARSER_REGEX_UTF8_HPP_
#define _DC_PARSER_REGEX_UTF8_HPP_

#include "../dcutf8.h"
#include "./regex_internal.hpp"
#include <assert.h>
#include <memory>
#include <string>
#include <vector>


class RegExpUTF8 : public SimpleRegExp<int>
{
  private:
    UTF8Decoder m_decoder;

  public:
    RegExpUTF8(const std::string& pattern);

    virtual void feed(char c);
    virtual void feed(int c) override;

    template<
        typename Iterator,
        typename = typename std::enable_if<is_forward_iterator<Iterator>::value &&
                                               iterator_value_type_is_same<Iterator, char>::value,
                                           void>::type>
    bool test(Iterator begin, Iterator end)
    {
        this->reset();
        for (auto it = begin; it != end; ++it)
            feed(*it);
        return match();
    }

    bool test(const std::string& str);

    static RegExpUTF8 compiled(const std::string& pattern);
};

#endif // _DC_PARSER_REGEX_UTF8_HPP_
