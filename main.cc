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
    list<job> jobs;
    job printparam;
    printparam.handler = &PrintParam;
    printparam.title = "Print Parameters";
    printparam.description = "This job prints out all given params which includes one from the vault.";
    printparam.args.push_back(argument{
        "",
        gaia::InputType::input_type::vault,
        "dbpassword",
    });

    jobs.push_back(printparam);

    // Serve
    try {
        gaia::Serve(jobs);
    } catch (string e) {
        std::cout << "Error: " << e << std::endl;
    }
}

