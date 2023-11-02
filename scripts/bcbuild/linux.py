#!/usr/bin/python
#
# Users can "supposedly" replace this script if they want to plug in projects
# other than the Linux kernel. Three methods should be defined;
#  > SetRepoPath(repo_path): set the persistent repo path to "repo_path".
#  > BuildAll(): build the entire repo.
#  > GetAllBCPaths(): return a list of all BC paths.
#  > Build(src_paths): build selected srcs.
#  > GetBCPath(src_path): return the BC path of "src_path".
#
# Actually I was kidding. You should implement all functions not starting with
# an underscore. Maybe with an underscore too because you need to "make" it for
# real in one way or another. (shrug)

import os
import shutil
import subprocess
import sys

N_PARALLEL_CORES = 4
repo_path = ""
log_file = None

def _Make(*cmds):
    SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
    IRDUMPER_PATH = SCRIPT_PATH + "/../../tmpext/IRDumper/build/lib/libDumper.so"
    LLVM_LIB_PATH = SCRIPT_PATH + "/../../../llvm/install/lib"
    CLANG_PATH = SCRIPT_PATH + "/../../../llvm/install/bin/clang"
    MF_NAME = "Makefile.cisa"

    # Generate alternative Makefile.
    mf_path = repo_path + '/' + MF_NAME 
    shutil.copy(repo_path + '/Makefile', mf_path)
    with open(mf_path, 'a') as f:
        f.write("\n# CISA build script addition.\n"\
                "KBUILD_USERCFLAGS += -Wno-error -g -Xclang -no-opaque-pointers "\
                "-Xclang -flegacy-pass-manager -Xclang -load -Xclang {0}\n"\
                "KBUILD_CFLAGS += -Wno-error -g -Xclang -no-opaque-pointers "\
                "-Xclang -flegacy-pass-manager -Xclang -load -Xclang {0}"\
                .format(IRDUMPER_PATH))

    # Execute "make".
    _make = subprocess.Popen(["make", "-C" + repo_path, "-f" + MF_NAME, "-k", "-i",
        "-j" + str(N_PARALLEL_CORES), "CC=" + CLANG_PATH] + list(cmds),
        stdout=log_file, stderr=log_file)
    _make.communicate()

def SetRepoPath(_repo_path, _log_file):
    global repo_path
    global log_file
    repo_path = _repo_path
    log_file = _log_file

def BuildAll():
    log_file.write("info: building all...\n")
    _Make()
    pass

def GetAllBCPaths():
    bc_paths = []
    for root, _, files in os.walk(repo_path):
        for f in files:
            if (f[-3:] == ".bc"):
                bc_paths += [os.path.join(root, f)]
    return bc_paths

def Build(src_paths):
    if (not src_paths): return
    o_paths = []
    for src_path in src_paths:
        assert(src_path[-2:] == ".c")
        o_paths += [src_path[:-2] + ".o"]
    log_file.write("info: building objects... ({})\n".format(', '.join(o_paths)))
    _Make(*o_paths)

def BuildDir(src_dirs):
    if (not src_dirs): return
    log_file.write("info: building directories... ({})\n".format(','.join(src_dirs)))
    _Make(*src_dirs)

def Finalize():
    if (log_file != sys.stdout):
        log_file.close()

def GetBCPath(src_path):
    assert(src_path[-2:] == ".c")
    bc_path = src_path[:-2] + ".bc"
    return bc_path if os.path.exists(repo_path + '/' + bc_path) else ""
