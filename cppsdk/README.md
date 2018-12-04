# cppsdk
C++ SDK for Gaia

# How to generate gRPC server interfaces
If the `plugin.proto` file has been changed, it's sometimes useful to regenerate the gRPC server interfaces.
You can use the command `make plugin.grpc.pb.cc plugin.pb.cc` to regenerate them.
