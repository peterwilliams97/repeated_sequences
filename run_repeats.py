# -*- coding: utf-8 -*-
"""
    Run c++ find repeats programs

    Found 3 exactly repeated strings of length 20
Exactly repeated strings: 3 [""ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"í"cÓ☺", ""ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"í"cÓ☺ÞÑ", "ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"ü"í"cÓ☺Þ", ] 3
0 : [0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0xa1, 0x22, 0x63, 0xe0, 0x01, ]
1 : [0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0xa1, 0x22, 0x63, 0xe0, 0x01, 0xe8, 0xa5, ]
2 : [0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0x81, 0x22, 0xa1, 0x22, 0x63, 0xe0, 0x01, 0xe8, ]
duration = 38.7724
"""
from __future__ import division, print_function
import optparse
import sys
import subprocess
import os
import fnmatch
# Avoid a name clash with glob() declared in this module
import glob as Glob


def _recursive_glob(path_pattern, make_path, include_dirs=False, do_reverse=False):
    """Like glob.glob() except that it sorts directories and
        recurses through subdirectories.
    """
    dir_name, mask = os.path.split(path_pattern)
    if not dir_name:
        dir_name = '.'
    recursive_list = os.walk(dir_name)
    if do_reverse:
        recursive_list = list(os.walk(dir_name))
        recursive_list = recursive_list[::-1]
    for root, dirs, files in recursive_list:
        path_list = sorted(fnmatch.filter(files, mask), key=lambda s: (s.lower(), s))
        if do_reverse:
            path_list = path_list[::-1]
        for filename in path_list:
            full_path = os.path.join(root, filename)
            if include_dirs or not os.path.isdir(full_path):
                yield(make_path(full_path))


def _glob(path_pattern, make_path, include_dirs=False, do_reverse=False):
    """Like glob() except that it sorts directories"""
    path_list = sorted(Glob.glob(path_pattern), key=lambda s: s)
    if do_reverse:
        path_list = path_list[::-1]
    for path in path_list:
        if include_dirs or not os.path.isdir(path):
            yield(make_path(path))


def glob(path_pattern, recursive=False, abs_path=False, include_dirs=False, do_reverse=False):
    """Like glob.glob() except that it sorts directories and
        optionally recurses through subdirectories and
        optionally returns full paths.
            path_pattern: path to match
            recursive: recurse through directories
            abs_path: use abspath to create returned names
    """
    globber = _recursive_glob if recursive else _glob
    make_path = os.path.abspath if abs_path else lambda p: p

    return globber(path_pattern, make_path, include_dirs, do_reverse)


REPEATS_EXE = r'd:\peter.dev\repeats\x64\Release\repeats.exe'
assert os.path.exists(REPEATS_EXE), REPEATS_EXE


def run_repeats(filelist):

    cmd = '%s %s' % (REPEATS_EXE, filelist)

    #stdout_file = open('wm.stdout', 'wt')
    #stderr_file = open('wm.stderr', 'wt')

    p = subprocess.Popen(cmd,
                         #stdout=stdout_file,
                         #stderr=stderr_file,
                         shell=True)
    ret = p.wait()
    #stdout_file.close()
    #stdout_file.close()

    #stdout_value = open('wm.stdout', 'rt').read()
    stderr_value = open('wm.stderr', 'rt').read()

    if ret:
        print('Failed. ret = %d' % ret)
        print('-' * 100)
     #   print stdout_value
        print('-' * 100)
     #   print stderr_value
        print('-' * 100)
        exit()

    print('-' * 100)

    return ret

parser = optparse.OptionParser('python ' + sys.argv[0] + ' [options] <file pattern>')
parser.add_option('-f', '--file-list', action='store_true', dest='is_file_list',
                  default=False,
                  help='argument is file list, not file pattern')
options, args = parser.parse_args()

if len(args) < 1:
    print(parser.usage)
    print('--help for more information')
    exit()

if options.is_file_list:
    filelist = args[0]
else:
    path_pattern = args[0]
    path_list = glob(path_pattern, recursive=True)
    filelist = 'files.list'
    open(filelist, 'wt').write('\n'.join(path_list))


run_repeats(filelist)
