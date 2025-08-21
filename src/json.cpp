// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/filesystem.hpp"

STDROMANO_NAMESPACE_BEGIN

/* JSON Load */

bool lex_number(char** s, std::size_t* size, bool* is_float) noexcept
{
    if(is_digit(**s) || **s == '-')
    {
        char* start = *s;
        *size = 1;

        (*s)++;

        *is_float = false;

        while(*s != '\0')
        {
            if(is_digit(**s))
            {
                (*s)++;
                (*size)++;
            }
            else if(**s == '.')
            {
                if(*is_float)
                {
                    return false;
                }

                *is_float = true;

                (*s)++;
                (*size)++;
            }
            else
            {
                break;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

struct StackFrame 
{
    JsonObject* container;
    bool is_dict;
    bool expecting_key;
    StringD current_key;
    
    StackFrame(JsonObject* cont, bool dict) 
        : container(cont), is_dict(dict), expecting_key(dict) {}
};

bool add_value_to_current_container(Vector<StackFrame>& stack,
                                    JsonObject*& current_value,
                                    JsonObject&& value,
                                    bool& root_set) noexcept
{
    if(stack.empty()) 
    {
        if(root_set) 
        {
            log_error("Invalid JSON: Multiple root values");
            return false;
        }

        *current_value = std::move(value);

        root_set = true;
    } 
    else 
    {
        StackFrame& frame = stack.back();

        if(frame.is_dict) 
        {
            if(frame.expecting_key) 
            {
                log_error("Invalid JSON: Expected key in object");
                return false;
            }

            frame.container->get<JsonDict>()[frame.current_key] = std::move(value);
        } 
        else
        {
            frame.container->get<JsonList>().push_back(std::move(value));
        }
    }

    return true;
}

bool json_parse(const char* text, JsonObject& json) noexcept
{
    char* s = const_cast<char*>(text);
    
    Vector<StackFrame> stack;
    JsonObject* current_value = &json;
    bool root_set = false;
    
    while(*s != '\0')
    {
        if(*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') 
        {
            s++;
            continue;
        }
        
        switch (*s)
        {
            case '{':
            {
                JsonDict new_dict;
                current_value->set(std::move(new_dict));
                
                stack.emplace_back(current_value, true);
                root_set = true;
                
                break;
            }
                
            case '[':
            {
                JsonList new_list;
                current_value->set(std::move(new_list));
                
                stack.emplace_back(current_value, false);
                root_set = true;

                break;
            }
                
            case '}':
            {
                if(stack.empty() || !stack.back().is_dict)
                {
                    log_error("Invalid JSON: Unexpected }} at char {}",
                              static_cast<size_t>(s - text));

                    return false;
                }

                stack.pop_back();

                break;
            }
                
            case ']':
            {
                if(stack.empty() || stack.back().is_dict)
                {
                    log_error("Invalid JSON: Unexpected ] at char {}",
                              static_cast<size_t>(s - text));
                    return false;
                }

                stack.pop_back();

                break;
            }
                
            case ':':
            {
                if(stack.empty() || !stack.back().is_dict || !stack.back().expecting_key) 
                {
                    log_error("Invalid JSON: Unexpected : at char {}",
                              static_cast<size_t>(s - text));

                    return false;
                }

                stack.back().expecting_key = false;

                break;
            }
                
            case ',':
            {
                if(stack.empty()) 
                {
                    log_error("Invalid JSON: Unexpected , at char {}",
                              static_cast<size_t>(s - text));

                    return false;
                }

                if(stack.back().is_dict) 
                {
                    stack.back().expecting_key = true;
                }

                break;
            }
                
            case '"':
            {
                s++;
                char* start = s;
                size_t size = 0;
                
                while(*s != '\0' && *s != '"') 
                {
                    if(*s == '\\' && *(s+1) != '\0') 
                    {
                        s++;
                    }

                    size++;
                    s++;
                }
                
                if(*s != '"') 
                {
                    log_error("Invalid JSON: Unterminated string at char {}",
                              static_cast<size_t>(start - text - 1));
                    return false;
                }
                
                StringD str_value(start, size);
                
                if(!stack.empty() && stack.back().is_dict && stack.back().expecting_key) 
                {
                    stack.back().current_key = std::move(str_value);
                }
                else 
                {
                    if(!add_value_to_current_container(stack,
                                                       current_value,
                                                       JsonObject(std::move(str_value)),
                                                       root_set))
                    {
                        return false;
                    }
                }

                break;
            }
                
            case 't':
                if(std::strncmp("true", s, 4) != 0) 
                {
                    log_error("Invalid JSON: Unknown token starting with t at char {}",
                              static_cast<size_t>(s - text));
                    return false;
                }
                
                if(!add_value_to_current_container(stack,
                                                   current_value,
                                                   JsonObject(true),
                                                   root_set)) 
                {
                    return false;
                }

                s += 3;
                
                break;
                
            case 'f':
                if(std::strncmp("false", s, 5) != 0) 
                {
                    log_error("Invalid JSON: Unknown token starting with f at char {}",
                              static_cast<size_t>(s - text));
                    return false;
                }
                
                if(!add_value_to_current_container(stack,
                                                   current_value,
                                                   JsonObject(false),
                                                   root_set)) 
                {
                    return false;
                }

                s += 4;

                break;
                
            case 'n':
                if(std::strncmp("null", s, 4) != 0) 
                {
                    log_error("Invalid JSON: Unknown token starting with n at char {}",
                              static_cast<size_t>(s - text));
                    return false;
                }
                
                if(!add_value_to_current_container(stack, current_value, JsonObject(), root_set)) 
                {
                    return false;
                }

                s += 3;
                break;
                
            case '-':
                if(!is_digit(*(s + 1))) 
                {
                    log_error("Invalid JSON: Invalid number starting with - at char {}",
                              static_cast<size_t>(s - text));
                    return false;
                }
                
            default:
                if(*s == '-' || is_digit(*s)) 
                {
                    char* start = s;
                    size_t size = 0;
                    bool is_float = false;
                    
                    if(!lex_number(&s, &size, &is_float)) 
                    {
                        return false;
                    }
                    
                    JsonObject num_value;

                    if(is_float) 
                    {
                        double val = std::strtod(start, nullptr);
                        num_value.set(val);
                    }
                    else 
                    {
                        int64_t val = std::strtoll(start, nullptr, 10);
                        num_value.set(val);
                    }
                    
                    if(!add_value_to_current_container(stack,
                                                       current_value,
                                                       std::move(num_value),
                                                       root_set)) 
                    {
                        return false;
                    }

                    s--;
                } 
                else 
                {
                    log_error("Invalid JSON: Unknown character \"{}\" ({}) at {}", *s, static_cast<int>(*s), static_cast<size_t>(s - text));
                    return false;
                }
                break;
        }
        
        s++;
    }
    
    if(!stack.empty())
    {
        log_error("Invalid JSON: Unmatched opening bracket");
        return false;
    }
    
    return true;
}

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
    if(do_utf8_validation && !validate_utf8(text.c_str(), text.size()))
    {
        log_error("JSON input contains invalid utf-8");
        return false;
    }

    if(json_parse(text.c_str(), this->_root))
    {
        return true;
    }

    this->_root = std::move(JsonObject());

    return false;
}

/* JSON Dump */

bool dumps_list(StringD& text, JsonObject& list) noexcept;

bool dumps_dict(StringD& text, JsonObject& dict) noexcept;

bool dumps_list(StringD& text, JsonObject& list) noexcept
{
    if(list.type() != JsonObjectType_List)
    {
        return false;
    }

    text.push_back('[');

    for(uint64_t i = 0; i < list.get<Vector<JsonObject>>().size(); i++)
    {
        JsonObject* value = list.get<Vector<JsonObject>>().at(i);

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

bool dumps_dict(StringD& text, JsonObject& dict) noexcept
{
    if(dict.type() != JsonObjectType_Dict)
    {
        return false;
    }

    text.push_back('{');

    for(auto it : dict.get<HashMap<StringD, JsonObject>>())
    {
        const StringD& key = it.first;
        JsonObject& value = it.second;

        text.appendf("\"{}\":", key);

        switch(it.second.type())
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