#!/usr/bin/python

import git
import re
from dataclasses import dataclass, field
from typing import List 

@dataclass
class ChangeInfo:
    state: str
    src_path: str
    del_lines: List[int] = field(default_factory=list)
    add_lines: List[int] = field(default_factory=list)
    bc_path: str = ""

def _GetPathPair(difflines):
    paths = [None, None]

    while (difflines):
        toks = difflines[0].split()
        if (toks):
            if (toks[0] == "diff"):
                DIFF_found = True
                paths[0] = toks[2][2:]
                paths[1] = toks[3][2:]
            elif (toks[0] == "deleted"):
                paths[1] = ""
            elif (toks[0] == "new"):
                paths[0] = ""
            elif (toks[0] == "diff" or toks[0] == "@@"):
                break

        difflines.pop(0)

    return tuple(paths)

def _GetChangedLinesPair(difflines):
    cur_lineno = [None, None]
    ch_lines = [[], []]

    while (difflines):
        _line = difflines[0]
        if (_line): 
            if (_line[0] == '@'):
                header_re = re.search("@@ -([0-9]+)(,[0-9]+)? " +
                        "\+([0-9]+)(,[0-9]+)? @@", _line)
                assert(header_re)
                cur_lineno[0] = int(header_re.group(1)) 
                cur_lineno[1] = int(header_re.group(3))
            elif (_line[0] == '-'):
                ch_lines[0] += [cur_lineno[0]]
                cur_lineno[0] += 1
            elif (_line[0] == '+'):
                ch_lines[1] += [cur_lineno[1]]
                cur_lineno[1] += 1
            elif (_line[0] == ' '):
                cur_lineno[0] += 1
                cur_lineno[1] += 1
            elif (_line[0:4] == "diff"):
                break

        difflines.pop(0)

    return tuple(ch_lines)

def GetChangeInfos(repo, commit):
    ret = []

    difflines = repo.git.show("--first-parent", commit.hexsha)
    difflines = difflines.split('\n')

    while (True):
        pre_path, post_path = _GetPathPair(difflines)
        if (not pre_path and not post_path): break
        del_lines, add_lines = _GetChangedLinesPair(difflines)

        if (pre_path == post_path):
            # Modified file: {del,add}_lines follow suit.
            ret += [ChangeInfo("modified", post_path, del_lines, add_lines)]
        elif (not pre_path):
            # Added file: only add_lines.
            ret += [ChangeInfo("added", post_path, [], add_lines)]
        elif (not post_path):
            # Deleted file: only del_lines.
            # If it's fully deleted, the removed functions cannot directly affect
            # the newer version whatsoever. Instead, the indirect impact should be
            # gathered by their callers.
            ret += [ChangeInfo("deleted", pre_path, [], [])]
        else:
            # Renamed (and possibly modified) file: {del,add}_lines only for new.
            # XXX: No renamed modified files from git-diff?
            ret += [ChangeInfo("deleted", pre_path, [], [])]
            ret += [ChangeInfo("added", post_path, del_lines, add_lines)]

    return ret
