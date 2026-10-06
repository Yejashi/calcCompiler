{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    llvmPackages.llvm
    clang
  ];

  # Ensure llvm-config is on PATH and LLVM_DIR is set for CMake
  shellHook = ''
    export PATH="${pkgs.llvmPackages.llvm}/bin:$PATH"
    export LLVM_DIR="${pkgs.llvmPackages.llvm}/lib/cmake/llvm"
    export CXX="${pkgs.clang}/bin/clang++"
  '';
}
