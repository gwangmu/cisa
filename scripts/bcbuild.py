#!/usr/bin/python

import os
import shutil
import subprocess
import sys

N_PARALLEL_CORES = 4
repo_path = ""
log_file = None

def _Make(*cmds):
    SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
    IRD_CLANG_PATH = SCRIPT_PATH + "/../build/extern/irdump/ird-clang"

    # Execute "make".
    _make = subprocess.Popen(["make",
        "-C" + repo_path, "-k", "-i", "-j" + str(N_PARALLEL_CORES), 
        "CC=" + IRD_CLANG_PATH] + list(cmds),
        stdout=log_file, stderr=log_file)
    _make.communicate()

def SetRepoPath(_repo_path, _log_file):
    global repo_path
    global log_file
    repo_path = _repo_path
    log_file = _log_file

def CleanAndReconfig(reconf_path):
    if (not reconf_path): return
    with open(reconf_path, 'r') as f:
        reconf_lines = f.readlines()
    for reconf_line in reconf_lines:
        subprocess.run("cd " + repo_path + ";" + reconf_line.strip(),
            shell=True, stdout=log_file, stderr=log_file)

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
    if (not os.path.isabs(src_path)):
        src_path = repo_path + '/' + src_path
    if (src_path[-2:] != ".c"):
        return "";
    bc_path = src_path[:-2] + ".bc"
    return bc_path if os.path.exists(bc_path) else ""
