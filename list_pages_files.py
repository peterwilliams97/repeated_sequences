# -*- coding: latin-1 -*-
"""
    Build a file list matching a specified pattern
"""
from __future__ import division, print_function
import re
import os
import sys


def recursive_glob(dir_name, matcher):
    """Like glob.glob() except that it sorts directories and
        recurses through subdirectories.
    """
    #print(dir_name)
    if not dir_name:
        dir_name = '.'
    recursive_list = os.walk(dir_name)
    for root, dirs, files in recursive_list:
        files2 = [fn for fn in files if matcher(fn)]
        path_list = sorted(files2, key=lambda s: (s.lower(), s))
        #print(root, len(files), len(files2), len(path_list))
        for filename in path_list:
            full_path = os.path.abspath(os.path.join(root, filename))
            if not os.path.isdir(full_path):
                yield(full_path)

RE_PAGES = re.compile(r'pages=(\d+)')


def num_pages(name):
    m = RE_PAGES.search(name)
    return int(m.group(1)) if m else -1


def match_pages(name_ext):
    #dir, name_ext = os.path.split(path)
    name, ext = os.path.splitext(name_ext)
    #print(name, ext)
    return ext.lower() in {'.spl', '.prn'} and RE_PAGES.search(name)


if __name__ == '__main__':
    import optparse
    parser = optparse.OptionParser('python ' + sys.argv[0] + ' <path> [options]')
    parser.add_option('-x', '--exclude', dest='exclude', default=None,
                      help='Exclude files matching this pattern')

    options, args = parser.parse_args()
    if len(args) < 1:
        print(parser.usage)
        print('--help for more information')
        exit(1)

    dir_name = args[0]
    re_exclude = re.compile(options.exclude) if options.exclude else None

    path_list = list(recursive_glob(dir_name, match_pages))
    path_list.sort(key=lambda s: (-num_pages(s), s.lower(), s))
    if re_exclude:
        path_list = [path for path in path_list if not re_exclude.search(path)]

    for path in path_list:
        print(path)
