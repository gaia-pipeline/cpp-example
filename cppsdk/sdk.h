#ifndef SDK_H
#define SDK_H

#include <string>
#include <map>
#include <list>
#include "plugin.grpc.pb.h"

using std::string;
using std::list;
using proto::Job;

namespace gaia {
    struct InputType {
        enum class input_type {
            textfield,
            textarea,
            boolean,
            vault
        };
    };

    inline const string ToString (InputType::input_type es) {
        const std::map<InputType::input_type,const string> EnumStrings {
            { InputType::input_type::textfield, "textfield" },
            { InputType::input_type::textarea, "textarea" },
            { InputType::input_type::boolean, "boolean" },
            { InputType::input_type::vault, "vault" }
        };
        auto it = EnumStrings.find(es);
        return it == EnumStrings.end() ? "Out of range" : it->second;
    }

    struct argument {
        string description;
        InputType::input_type type;
        string key;
        string value;
    };

    struct manual_interaction {
        string description;
        InputType::input_type type;
        string value;
    };

    struct job {
        void (*handler)(list<argument>) throw(string);
        string title;
        string description;
        list<string> depends_on;
        list<argument> args;
        manual_interaction interaction;
    };

    struct job_wrapper {
        void (*handler)(list<argument>) throw(string);
        Job job;
    };

    void Serve(list<job>) throw(string);
}

#endif 
