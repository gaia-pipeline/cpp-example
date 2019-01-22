#include "cppsdk/sdk.h"
#include <list>
#include <iostream>
#include <chrono>
#include <thread>

using std::list;
using std::cerr;
using std::endl;
using gaia::argument;
using gaia::job;

void CreateUser(list<argument> args) throw(string) {
    cerr << "CreateUser has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "CreateUser has been finished!" << endl;
}

void MigrateDB(list<argument> args) throw(string) {
    cerr << "MigrateDB has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "MigrateDB has been finished!" << endl;
}

void CreateNamespace(list<argument> args) throw(string) {
    cerr << "CreateNamespace has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "CreateNamespace has been finished!" << endl;
}

void CreateDeployment(list<argument> args) throw(string) {
    cerr << "CreateDeployment has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "CreateDeployment has been finished!" << endl;
}

void CreateService(list<argument> args) throw(string) {
    cerr << "CreateService has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "CreateService has been finished!" << endl;
}

void CreateIngress(list<argument> args) throw(string) {
    cerr << "CreateIngress has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "CreateIngress has been finished!" << endl;
}

void Cleanup(list<argument> args) throw(string) {
    cerr << "Cleanup has been started!" << endl;

    // lets sleep to simulate that we do something
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cerr << "Cleanup has been finished!" << endl;
}

int main() {
    std::list<gaia::job> jobs;
    gaia::job createuser;
    createuser.handler = &CreateUser;
    createuser.title = "Create DB User";
    createuser.description = "Create DB User with least privileged permissions.";

    gaia::job migratedb;
    migratedb.handler = &MigrateDB;
    migratedb.title = "DB Migration";
    migratedb.description = "Imports newest test data dump and migrates to newest version.";
    migratedb.depends_on.push_back("Create DB User");

    gaia::job createnamespace;
    createnamespace.handler = &CreateNamespace;
    createnamespace.title = "Create K8S Namespace";
    createnamespace.description = "Create a new Kubernetes namespace for the new test environment.";
    createnamespace.depends_on.push_back("DB Migration");

    gaia::job createdeployment;
    createdeployment.handler = &CreateDeployment;
    createdeployment.title = "Create K8S Deployment";
    createdeployment.description = "Create a new Kubernetes deployment for the new test environment.";
    createdeployment.depends_on.push_back("Create K8S Namespace");

    gaia::job createservice;
    createservice.handler = &CreateService;
    createservice.title = "Create K8S Service";
    createservice.description = "Create a new Kubernetes service for the new test environment.";
    createservice.depends_on.push_back("Create K8S Namespace");

    gaia::job createingress;
    createingress.handler = &CreateIngress;
    createingress.title = "Create K8S Ingress";
    createingress.description = "Create a new Kubernetes ingress for the new test environment.";
    createingress.depends_on.push_back("Create K8S namespace");

    gaia::job cleanup;
    cleanup.handler = &Cleanup;
    cleanup.title = "Clean up";
    cleanup.description = "Removes all temporary files.";
    cleanup.depends_on.push_back("Create K8S Deployment");
    cleanup.depends_on.push_back("Create K8S Service");
    cleanup.depends_on.push_back("Create K8S Ingress");

    jobs.push_back(createuser);
    jobs.push_back(migratedb);
    jobs.push_back(createnamespace);
    jobs.push_back(createdeployment);
    jobs.push_back(createservice);
    jobs.push_back(createingress);
    jobs.push_back(cleanup);

    // Serve
    try {
        gaia::Serve(jobs);
    } catch (string e) {
        std::cout << "Error: " << e << std::endl;
    }
}

