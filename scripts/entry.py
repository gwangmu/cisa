#!/usr/bin/python

# Boilerplate code: load C++ part.
import sys, os
import log

_CXX_PART_PATH = os.path.dirname(os.path.realpath(__file__)) + '/../build/lib'
if (not os.path.exists(_CXX_PART_PATH)):
    log.Fatal('CISA-C++ not found. Try building it first.')
else:
    sys.path.append(_CXX_PART_PATH)

# Main Python part.
import argparse
import git
import os
import shutil
import signal
import sys
from alive_progress import alive_it
from dataclasses import dataclass
from datetime import datetime

import cisa
from bcbuild import libsndfile as bcbuild
import comanal

WORK_REPO_PATH = '/tmp/cisa_repo-' + str(int(datetime.timestamp(datetime.now())))

parser = argparse.ArgumentParser()
parser.add_argument('repo_path', type=str, help='Path to the repo.')
parser.add_argument('-a', type=str, help="Commit after... (date, incl)")
parser.add_argument('-A', type=str, help="Commit after... (hexsha, incl)")
parser.add_argument('-b', type=str, help="Commit before... (date, incl)")
parser.add_argument('-B', type=str, help="Commit before... (hexsha, incl)")
parser.add_argument('-o', type=str, default="cisa_out", help="Path to output.")
parser.add_argument('-m', action='store_true', help="Include merge commits.")
args = parser.parse_args()

def _Cleanup(sig, frame):
    if (os.path.exists(WORK_REPO_PATH)):
        shutil.rmtree(WORK_REPO_PATH)                  # XXX: test (bc precomp)
    sys.exit(0)
signal.signal(signal.SIGINT, _Cleanup)

def _UpdateRepo(repo, commit, changes):
    def _ToAbs(path):
        return WORK_REPO_PATH + '/' + path

    def _RemoveFileRec(path, is_file):
        if (is_file): os.remove(_ToAbs(path))
        else: shutil.rmtree(_ToAbs(path))
        path = os.path.dirname(path)
        if (path and not os.listdir(_ToAbs(path))):
            _RemoveFileRec(path, False)

    for change in changes:
        if (change.state == "deleted"):
            _RemoveFileRec(change.src_path, True)

    for change in changes:
        if (change.state != "deleted"):
            if (change.state == "added"):
                os.makedirs(os.path.dirname(_ToAbs(change.src_path)), exist_ok=True)
            _src = repo.git.show('{}:{}'.format(commit.hexsha, change.src_path))
            with open(_ToAbs(change.src_path), "w") as f:
                f.write(_src)

def main():
    log.Init("log")

    if (os.path.exists(args.o)): shutil.rmtree(args.o)
    os.makedirs(args.o)
    bcbuild.SetRepoPath(WORK_REPO_PATH, sys.stdout) #open(args.o + '/log.bcbuild', 'w'))

    log.Info("prepping git...")
    shutil.copytree(args.repo_path, WORK_REPO_PATH)
    repo = git.Repo(WORK_REPO_PATH)
    if (args.A or args.B):
        after_sha = args.A + "^" if args.A else ""
        before_sha = args.B if args.B else "HEAD"
        commits = list(repo.iter_commits("{}..{}".format(after_sha, before_sha),
            "", ancestry_path=True, reverse=True))
    else:
        commits = list(repo.iter_commits(None, "", after=args.a, before=args.b,
                reverse=True))
    print([c.hexsha[:12] for c in commits])

    if (not commits):
        log.Fatal("no commits in range.")
    else:
        log.Info("total {} commit(s) found.".format(len(commits)))

    log.Info("checking out initial commit...")
    repo.git.checkout(commits[0].hexsha, force=True)   # XXX: test (bc precomp)
    commits = commits[1:]
    bcbuild.CleanAndReconfig()

    log.Info("building all...")
    bcbuild.BuildAll()                                 # XXX: test (bc precomp)
    bc_paths = bcbuild.GetAllBCPaths()

    log.Info("initializing CISA-C++...")
    cisa.Initialize(bc_paths, args.o)

    log.Info("analyzing commits...")
    #for ci in alive_it(commits):
    for idx, ci in enumerate(commits):
        log.Next()
        log.Info("analyzing commit {}/{} (hexsha: {})...".format(idx+1, 
            len(commits), ci.hexsha[:12]))

        # Collect/overwrite changed (i.e., del/add/mod) files.
        # This should include ALL files, including assembly.
        log.Info(" - getting change info...")
        changes = comanal.GetChangeInfos(repo, ci)
        _UpdateRepo(repo, ci, changes)

        # Convert header paths to affected source paths.
        # Let's ignore header functions for now. They'll be analyzed when
        # the functions in source code that use header functions are analyzed. 
        #   > change_srcs = bcbuild.FindDependentSrcs(changed_paths)
        change_srcs = list(filter(lambda i: i.src_path[-2:] == ".c", changes))

        # Partially compile changed files to BCs.
        # One problem: some source code may not be suppose to be compiled
        # (because of the config flags). Here's the workaround: 
        #   1) if the source code (*.c) exists but the object file doesn't, the
        #      previous build must excluded the compilation of this file. Just 
        #      ignore it.
        #   2) if the source code was newly added, build the parent directory.
        log.Info(" - building {} changed file(s)...".format(len(change_srcs)))
        _src_dirs_a = []
        _src_paths_m = []
        for c in change_srcs:
            if (c.state == "added"):
                _src_dirs_a += [os.path.dirname(c.src_path)]
            elif (c.state == "modified"):
                if (bcbuild.GetBCPath(c.src_path)):
                    _src_paths_m += [c.src_path]
        _src_dirs_a = list(set(_src_dirs_a))
        bcbuild.Build(_src_paths_m)
        bcbuild.BuildDir(_src_dirs_a)
        for c in change_srcs:
            c.bc_path = bcbuild.GetBCPath(c.src_path)
        change_srcs = [c for c in change_srcs if c.bc_path]

        # Update/aggregate in CISA-C++.
        # Even if it's a merge commit, the analysis and the call graph should be
        # updated. We can only not aggregate the relevance sets if merge commits
        # are not included.
        log.Info(" - analyzing...".format(len(change_srcs)))
        do_agg = (len(ci.parents) == 1 or args.m)
        cisa.UpdateModules(ci.hexsha[:12],
                [i.bc_path for i in change_srcs], 
                [i.del_lines for i in change_srcs],
                [i.add_lines for i in change_srcs], do_agg)

    cisa.Finalize()
    bcbuild.Finalize()
    shutil.rmtree(WORK_REPO_PATH)
    log.Info("done.")

if (__name__ == "__main__"):
    main()
