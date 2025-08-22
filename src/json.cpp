// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/filesystem.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

#include <stack>

STDROMANO_NAMESPACE_BEGIN

/* JSON Load */

void _parse_number(const char* s,
                   const char* end,
                   std::int64_t* i_val,
                   double* d_val,
                   bool* is_double,
                   std::size_t* parse_size) noexcept 
{
    const char* p = s;

    bool neg = false;

    if(p < end && *p == '-') 
    {
        neg = true;
        ++p;
    }

    uint64_t int_val = 0;
    double float_val = 0.0;
    int frac_exp = 0;
    bool is_float = false;

    while(p < end && *p >= '0' && *p <= '9') 
    {
        std::uint64_t digit = static_cast<std::uint64_t>(*p - '0');

        if(!is_float &&
           (int_val > (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - digit) / 10))
        {
            is_float = true;
            float_val = static_cast<double>(int_val);
            frac_exp = 0;
        }

        if(!is_float) 
        {
            int_val = int_val * 10 + digit;
        } 
        else 
        {
            float_val = float_val * 10.0 + digit;
            frac_exp--;
        }

        ++p;
    }

    if(p < end && *p == '.') 
    {
        is_float = true;
        if (float_val == 0.0) float_val = static_cast<double>(int_val);

        ++p;

        while(p < end && *p >= '0' && *p <= '9') 
        {
            float_val = float_val * 10.0 + (*p - '0');
            frac_exp--;
            ++p;
        }
    }

    int exp_val = 0;
    bool exp_neg = false;

    if(p < end && (*p == 'e' || *p == 'E')) 
    {
        is_float = true;
        if (float_val == 0.0 && !is_float) float_val = static_cast<double>(int_val);

        ++p;

        if(p < end && (*p == '+' || *p == '-')) 
        {
            exp_neg = (*p == '-');
            ++p;
        }

        while(p < end && *p >= '0' && *p <= '9') 
        {
            exp_val = exp_val * 10 + (*p - '0');
            ++p;
        }
    }

    if(exp_neg)
        exp_val = -exp_val;

    if(!is_float) 
    {
        std::int64_t result = neg ? -static_cast<std::int64_t>(int_val) :
                                     static_cast<std::int64_t>(int_val);
        *i_val = result;
    } 
    else 
    {
        double val = float_val * std::pow(10.0, frac_exp + exp_val);

        if(neg)
            val = -val;

        *d_val = val;
    }

    *parse_size = p - s;
    *is_double = is_float;
}

class JsonParser
{
private:
    const char* text;
    const char* current;
    const char* end;
    
    enum ParseContext
    {
        Context_Root,
        Context_Dict,
        Context_List
    };
    
    struct StackFrame
    {
        ParseContext context;
        JsonObject* object;
        StringD key;
        bool expecting_value;
        
        StackFrame(ParseContext ctx, JsonObject* obj) : context(ctx),
                                                        object(obj),
                                                        expecting_value(false) {}
    };
    
    std::stack<StackFrame> parse_stack;

public:
    JsonParser(const char* json_text) : text(json_text),
                                        current(json_text),
                                        end(json_text + std::strlen(json_text)) {}
    
    JsonParser(const char* json_text, std::size_t length) : text(json_text),
                                                            current(json_text),
                                                            end(json_text + length) {}
    
    bool parse(JsonObject& root) noexcept
    {
        this->current = text;

        while(!this->parse_stack.empty()) 
        {
            this->parse_stack.pop();
        }
        
        this->skip_whitespace();

        if(this->current >= this->end) 
        {
            return false;
        }
        
        return this->parse_value(root);
    }

private:
    STDROMANO_FORCE_INLINE void skip_whitespace() noexcept
    {
        while(this->current < this->end && 
              (*this->current == ' ' ||
               *this->current == '\t' || 
               *this->current == '\n' || 
               *this->current == '\r')) 
        {
            this->current++;
        }
    }
    
    bool parse_value(JsonObject& obj) noexcept
    {
        this->skip_whitespace();

        if(this->current >= this->end) 
        {
            return false;
        }
        
        switch(*this->current)
        {
            case '"':
                return this->parse_string(obj);
            case '{':
                return this->parse_dict(obj);
            case '[':
                return this->parse_list(obj);
            case 't':
                return this->parse_literal(obj, "true", 4, true);
            case 'f':
                return this->parse_literal(obj, "false", 5, false);
            case 'n':
                return this->parse_null(obj);
            case '-':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return this->parse_number(obj);
            default:
                return false;
        }
    }
    
    bool parse_string(JsonObject& obj) noexcept
    {
        if(this->current >= this->end || *this->current != '"') 
        {
            return false;
        }
        
        this->current++;
        const char* start = this->current;
        
        while(this->current < this->end && *this->current != '"') 
        {
            if(*this->current == '\\') 
            {
                this->current++;

                if(this->current >= this->end) 
                {
                    return false;
                }
            }

            this->current++;
        }
        
        if(this->current >= this->end || *this->current != '"')
        {
            return false;
        }
        
        std::size_t length = this->current - start;
        StringD str = StringD::make_from_c_str(start, length);
        obj.set(std::move(str));
        
        this->current++;
        return true;
    }
    
    bool parse_number(JsonObject& obj) noexcept
    {
        std::int64_t int_val = 0;
        double double_val = 0.0;
        bool is_double = false;
        std::size_t parsed_size = 0;

        _parse_number(this->current, this->end, &int_val, &double_val, &is_double, &parsed_size);
        
        if(is_double) 
        {
            obj.set(double_val);
        }
        else 
        {
            obj.set(int_val);
        }

        this->current += parsed_size;
        
        return true;
    }
    
    bool parse_literal(JsonObject& obj, const char* literal, std::size_t len, bool value) noexcept
    {
        if(this->current + len > end || std::strncmp(current, literal, len) != 0) 
        {
            return false;
        }
        
        obj.set(value);
        this->current += len;
        return true;
    }
    
    bool parse_null(JsonObject& obj)
    {
        if(this->current + 4 > this->end || std::strncmp(this->current, "null", 4) != 0) 
        {
            return false;
        }
        
        this->current += 4;

        return true;
    }
    
    bool parse_list(JsonObject& obj) noexcept
    {
        if(this->current >= this->end || *this->current != '[') 
        {
            return false;
        }
        
        this->current++;
        this->skip_whitespace();
        
        JsonList list;
        obj.set(std::move(list));
        
        if(this->current < this->end && *this->current == ']') 
        {
            this->current++;
            return true;
        }
        
        while(true)
        {
            JsonObject element;

            if(!this->parse_value(element)) 
            {
                return false;
            }
            
            obj.get<JsonList>().push_back(std::move(element));
            
            this->skip_whitespace();

            if(this->current >= this->end) 
            {
                return false;
            }
            
            if(*this->current == ']') 
            {
                this->current++;
                break;
            }
            else if(*this->current == ',') 
            {
                this->current++;
                this->skip_whitespace();
                
                if(this->current < this->end && *this->current == ']') 
                {
                    return false;
                }
            } 
            else 
            {
                return false;
            }
        }
        
        return true;
    }
    
    bool parse_dict(JsonObject& obj) noexcept
    {
        if(this->current >= this->end || *this->current != '{') 
        {
            return false;
        }
        
        this->current++;
        this->skip_whitespace();
        
        JsonDict dict;
        obj.set(std::move(dict));
        
        if(this->current < this->end && *this->current == '}') 
        {
            this->current++;
            return true;
        }
        
        while (true)
        {
            this->skip_whitespace();

            if(this->current >= this->end || *this->current != '"') 
            {
                return false;
            }
            
            JsonObject key_obj;

            if(!this->parse_string(key_obj)) 
            {
                return false;
            }

            StringD key = key_obj.get<StringD>();
            
            this->skip_whitespace();

            if(this->current >= this->end || *this->current != ':') 
            {
                return false;
            }

            this->current++;
            
            JsonObject value;

            if(!this->parse_value(value)) 
            {
                return false;
            }
            
            obj.get<JsonDict>()[std::move(key)] = std::move(value);
            
            this->skip_whitespace();
            if(current >= end) 
            {
                return false;
            }
            
            if(*this->current == '}') 
            {
                this->current++;
                break;
            } 
            else if(*this->current == ',') 
            {
                this->current++;
                this->skip_whitespace();
                
                if(current < end && *current == '}') 
                {
                    return false;
                }
            } 
            else 
            {
                return false;
            }
        }
        
        return true;
    }
};

bool Json::loadf(const StringD& file_path,
                 bool do_utf8_validation) noexcept
{
    const StringD content = load_file_content(file_path, "rb");

    if(content.empty())
    {
        return false;
    }

    return this->loads(content, do_utf8_validation);
}

bool Json::loads(const StringD& text,
                 bool do_utf8_validation) noexcept
{
    SCOPED_PROFILE_START(ProfileUnit::MilliSeconds, json_loads);

    if(do_utf8_validation && !validate_utf8(text.c_str(), text.size()))
    {
        log_error("JSON input contains invalid utf-8");
        return false;
    }

    JsonParser parser(text.c_str(), text.size());

    return parser.parse(this->_root);
}

/* JSON Dump */

bool dumps_list(StringD& text, const JsonObject& list) noexcept;

bool dumps_dict(StringD& text, const JsonObject& dict) noexcept;

bool dumps_list(StringD& text, const JsonObject& list) noexcept
{
    if(list.type() != JsonObjectType_List)
    {
        return false;
    }

    text.push_back('[');

    const JsonList& l = list.get<JsonList>();

    for(uint64_t i = 0; i < l.size(); i++)
    {
        const JsonObject* value = l.at(i);

        switch(value->type())
        {
            case JsonObjectType_List:
                if(!dumps_list(text, *value))
                {
                    return false;
                }

                break;
            case JsonObjectType_Dict:
                if(!dumps_dict(text, *value))
                {
                    return false;
                }

                break;
            default:
                text.appends(value->get_as_string());
                break;
        }

        text.push_back(',');
    }

    text.rstrip(',');

    text.push_back(']');

    return true;
}

bool dumps_dict(StringD& text, const JsonObject& dict) noexcept
{
    if(dict.type() != JsonObjectType_Dict)
    {
        return false;
    }

    text.push_back('{');

    const JsonDict& d = dict.get<JsonDict>();

    for(const auto& [key, value] : dict.get<JsonDict>())
    {
        text.appendf("\"{}\":", key);

        switch(value.type())
        {
            case JsonObjectType_List:
                if(!dumps_list(text, value))
                {
                    return false;
                }

                break;
            case JsonObjectType_Dict:
                if(!dumps_dict(text, value))
                {
                    return false;
                }

                break;
            default:
                text.appends(value.get_as_string());
                break;
        }

        text.push_back(',');
    }

    text.rstrip(',');

    text.push_back('}');

    return true;
}

bool Json::dumpf(const StringD& file_path) noexcept
{
    return true;
}

bool Json::dumps(StringD& text) noexcept
{
    switch(this->_root.type())
    {
        case JsonObjectType_Dict:
            if(!dumps_dict(text, this->_root))
            {
                return false;
            }

            break;

        case JsonObjectType_List:
            if(!dumps_list(text, this->_root))
            {
                return false;
            }

            break;

        default:
            return false;
    }

    text.push_back('\0');

    return true;
}

STDROMANO_NAMESPACE_END