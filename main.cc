#include "cppsdk/sdk.h"
#include <list>
#include <iostream>

void DoSomethingAwesome(std::list<gaia::argument> args) throw(std::string) {
    std::cerr << "This output will be streamed back to gaia and will be displayed in the pipeline logs." << std::endl;

    // An error occured? Return it back so gaia knows that this job failed.
    // throw "Uhh something badly happened!"
}

int main() {
    std::list<gaia::job> jobs;
    gaia::job awesomejob;
    awesomejob.handler = &DoSomethingAwesome;
    awesomejob.title = "DoSomethingAwesome";
    awesomejob.description = "This job does something awesome.";

    try {
        gaia::Serve(jobs);
    } catch (string e) {
        std:cerr << "Error: " << e << std::endl;
    }
}

