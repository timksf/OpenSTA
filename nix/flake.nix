{
    description = "C/C++ env for OpenSTA dev";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/23.11";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, flake-utils, nixpkgs } :
        flake-utils.lib.eachSystem ["x86_64-linux"] (system: let
            pkgs = import nixpkgs {
                inherit system;
            };
            in { 
                devShell = with pkgs; clangStdenv.mkDerivation rec {
                    name = "opensta-dev";
                    cmakeFlags = [
                        "-DTCL_LIBRARY=${tcl}/lib/libtcl${clangStdenv.hostPlatform.extensions.sharedLibrary}"
                        "-DTCL_HEADER=${tcl}/include/tcl.h"
                    ];
                    buildInputs = [
                        tcl
                        zlib
                        eigen
                    ];
                    nativeBuildInputs = [
                        clang-tools
                        lldb_15 #lldb_16 seems to be broken
                        gdb
                        swig
                        pkg-config
                        cmake
                        gnumake
                        flex
                        bison
                    ];
                };                
            }
        );
}