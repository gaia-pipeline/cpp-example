#include <string>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "plugin.grpc.pb.h"
#include "sdk.h"

using std::string;
using std::unique_ptr;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using proto::Plugin;
using proto::Empty;
using proto::Job;
using proto::JobResult;
using proto::Argument;
using proto::ManualInteraction;

// General constants
static const string SERVER_CERT_ENV = "GAIA_PLUGIN_CERT";
static const string SERVER_KEY_ENV = "GAIA_PLUGIN_KEY";
static const string ROOT_CA_CERT_ENV = "GAIA_PLUGIN_CA_CERT";
static const string LISTEN_ADDRESS = "127.0.0.1";
static const int CORE_PROTOCOL_VERSION = 1;
static const int PROTOCOL_VERSION = 2;
static const string PROTOCOL_TYPE = "grpc";
static const string PROTOCOL_NETWORK = "tcp";

// FNV hash constants
static const unsigned int FNV_PRIME = 16777619u;
static const unsigned int OFFSET_BASIS = 2166136261u;

// Error messages
static const string ERR_JOB_NOT_FOUND = "job not found in plugin";
static const string ERR_EXIT_PIPELINE = "pipeline exit requested by job";
static const string ERR_DUPLICATE_JOB = "duplicate job found (two jobs with the same title)";

class GRPCPluginImpl final : public Plugin::Service {
    public:
        Status GetJobs(ServerContext* context, const Empty* request, ServerWriter<Job>* writer) {
            // Iterate over all jobs and send every job to client (e.g. Gaia).
            for (auto const& job : cached_jobs) {
                writer->Write(job.job);
            }
            return Status::OK;
        }

        Status ExecuteJob(ServerContext* context, const Job* request, JobResult* response) {
            gaia::job_wrapper * job = GetJob((*request));
            if (job == nullptr) {
                return Status(grpc::StatusCode::CANCELLED, ERR_JOB_NOT_FOUND);
            }

            // Transform arguments.
            list<gaia::argument> args;
            for (int i = 0; i < (*request).args_size(); ++i) {
                gaia::argument arg = {};
                arg.key = (*request).args(i).key();
                arg.value = (*request).args(i).value();
                args.push_back(arg);
            }

            // Execute job function.
            try {
                (*job).handler(args);
            } catch (string e) {
                // Check if job wants to force exit pipeline.
                // We will exit the pipeline but not mark as 'failed'.
                if (e.compare(ERR_EXIT_PIPELINE) != 0) {
                   response->set_failed(true);
                }

                // Set log message and job id.
                response->set_exit_pipeline(true);
                response->set_message(e);
                response->set_unique_id((*job).job.unique_id());
            }

            return Status::OK;
        }

        void PushCachedJobs(gaia::job_wrapper* job) {
            cached_jobs.push_back(*job);
        }

        static bool compare(gaia::job_wrapper a, gaia::job_wrapper b) {
            return (a.job.unique_id() == b.job.unique_id());
        }

        void ApplyUnique() throw(string) {
            int before_unique = cached_jobs.size();
            cached_jobs.unique(compare);
            if (before_unique > cached_jobs.size()) {
                throw ERR_DUPLICATE_JOB;
            }
        }

    private:
        list<gaia::job_wrapper> cached_jobs;
        
        // GetJob finds the right job in the cache and returns it.
        gaia::job_wrapper * GetJob(const Job job) {
            for (auto & cached_job : cached_jobs) {
                if (cached_job.job.unique_id() == job.unique_id()) {
                    return &cached_job;
                }
            }
            return nullptr;
        }
};

static unsigned int fnvHash(const char* str) {
    const size_t length = strlen(str) + 1;
    unsigned int hash = OFFSET_BASIS;
    for (size_t i = 0; i < length; ++i) {
        hash ^= *str++;
        hash *= FNV_PRIME;
    }
    return hash;
};

static bool read_file(const string& filename, string& data) {
    std::ifstream file(filename.c_str(), std::ios::in);
	if (file.is_open()) {
		std::stringstream ss;
		ss << file.rdbuf ();
		file.close ();
		data = ss.str ();
        return true;
	}
	return false;
}

namespace gaia {

    void Serve(list<gaia::job> jobs) throw(string) {
        // Allocate space for objects.
        GRPCPluginImpl service;
        ServerBuilder builder;

        // Transform all given jobs to proto objects.
        for (auto const& job : jobs) {
            Job proto_job;
            
            // Transform manual interaction.
            ManualInteraction* ma = new ManualInteraction();
            ma->set_description(job.interaction.description);
            ma->set_type(ToString(job.interaction.type));
            ma->set_value(job.interaction.value);
            proto_job.set_allocated_interaction(ma);

            // Transform arguments.
            for (auto const& a : job.args) {
                Argument* arg = proto_job.add_args();
                arg->set_description(a.description);
                arg->set_type(ToString(a.type));
                arg->set_key(a.key);
                arg->set_value(a.value);
            }

            // Set other data to proto object.
            proto_job.set_unique_id(fnvHash(job.title.c_str()));
            proto_job.set_title(job.title);
            proto_job.set_description(job.description);

            // Resolve dependencies.
            for (auto const& dependency : job.depends_on) {
                bool dependency_found;
                for (auto const& current_job : jobs) {
                    // Transform titles to lower case for higher matching.
                    string current_job_title = current_job.title;
                    string depends_on_title = dependency;
                    std::transform(current_job_title.begin(), current_job_title.end(), current_job_title.begin(), ::tolower);
                    std::transform(depends_on_title.begin(), depends_on_title.end(), depends_on_title.begin(), ::tolower);
                    
                    // Check if this is the specified dependency.
                    if (current_job_title.compare(depends_on_title) == 0) {
                        dependency_found = true;
                        proto_job.add_dependson(fnvHash(current_job.title.c_str()));
                        break;
                    }
                }

                // Check if dependency has been found.
                if (!dependency_found) {
                    throw "job '" + job.title + "' has dependency '" + dependency + "' which is not declared";
                }
            }

            // Create the jobs wrapper object.
            gaia::job_wrapper w = {
                job.handler,
                proto_job,
            };
            service.PushCachedJobs(&w);
        }

        // ApplyUnique checks if given jobs includes a duplicate.
        // If so it will throw an error.
        service.ApplyUnique();

        // Get certificates path from env variables.
        char* cert_path_p = std::getenv(SERVER_CERT_ENV.c_str());
        char* key_path_p = std::getenv(SERVER_KEY_ENV.c_str());
        char* ca_cert_path_p = std::getenv(ROOT_CA_CERT_ENV.c_str());

        // if the env variable was not found it returns a pullptr.
        if (cert_path_p == nullptr) {
            throw "certificate env variable was not set: " + SERVER_CERT_ENV;
        } else if (key_path_p == nullptr) {
            throw "key env variable was not set: " + SERVER_KEY_ENV;
        } else if (ca_cert_path_p == nullptr) {
            throw "root certificate env variable was not set: " + ROOT_CA_CERT_ENV; 
        }
        string cert_path(cert_path_p);
        string key_path(key_path_p);
        string ca_cert_path(ca_cert_path_p);

        // Load all certificates into memory.
        string cert_raw;
        string key_raw;
        string ca_cert_raw;
        if (!read_file(cert_path, cert_raw)) {
            throw "certificate is not a file or does not exist: " + cert_path; 
        } else if (!read_file(key_path, key_raw)) {
            throw "key is not a file or does not exist: " + key_path;
        } else if (!read_file(ca_cert_path, ca_cert_raw)) {
            throw "root certificate is not a file or does not exist: " + ca_cert_path;
        }

        // Load and setup mTLS.
        grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = {
            key_raw,
            cert_raw,
        };
        grpc::SslServerCredentialsOptions ssl_ops;
        ssl_ops.pem_root_certs = ca_cert_raw;
        ssl_ops.pem_key_cert_pairs.push_back(keycert);

        // Allocate memory for the automatic selected port.
        int * selectedPort = new int(0);

        // Enable health check service and start grpc server.
        grpc::EnableDefaultHealthCheckService(true);
        builder.AddListeningPort(LISTEN_ADDRESS + string(":0"), grpc::SslServerCredentials(ssl_ops), selectedPort);
        builder.RegisterService(&service);
        unique_ptr<Server> server(builder.BuildAndStart());
             
        // Define health service.
        grpc::HealthCheckServiceInterface* health_svc = server->GetHealthCheckService();
        health_svc->SetServingStatus("plugin", true);

        // Output the address and service name to stdout.
        // hashicorp go-plugin will use that to establish connection.
        std::cout << CORE_PROTOCOL_VERSION <<
            "|" << PROTOCOL_VERSION <<
            "|" << PROTOCOL_NETWORK <<
            "|" << LISTEN_ADDRESS + ":" << *selectedPort <<
            "|" << PROTOCOL_TYPE << std::endl << std::flush;

        // clean up a bit and wait until server receives exit signal.
        delete selectedPort;
        server->Wait();
    };
}
