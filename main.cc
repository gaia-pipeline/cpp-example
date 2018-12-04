#include "cppsdk/sdk.h"
#include <list>
#include <iostream>

using std::list;
using std::cerr;
using std::endl;
using gaia::argument;
using gaia::job;

void PrintParam(list<argument> args) throw(string) {
    for (auto const& arg : args) {
        cerr << "Key: " << arg.key << "; Value: " << arg.value << ";" << endl;
    }
}

int main() {
    std::list<job> jobs;
    job printparam;
    printparam.handler = &PrintParam;
    printparam.title = "Print Parameters";
    printparam.description = "This job prints out all given params.";
    printparam.args.push_back(argument{
        "Username for the database schema:",
        // textfield displays a text field in the UI.
        // You can also use textarea for text area, boolean
        // for boolean input.
        gaia::InputType::input_type::textfield,
        "username",
    });
    printparam.args.push_back(argument{
        "Description for username:",
        // textfield displays a text field in the UI.
        // You can also use textarea for text area, boolean
        // for boolean input.
        gaia::InputType::input_type::textarea,
        "usernamedesc",
    });

    jobs.push_back(printparam);

    // Serve
    try {
        gaia::Serve(jobs);
    } catch (string e) {
        std::cout << "Error: " << e << std::endl;
    }
}

