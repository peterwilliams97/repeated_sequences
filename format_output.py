"""
INNER_LOOP=4
# size = 10.000 Mbyte
# min_repeats = 10
# num_documents = 2
# method = 4
# num_unique = 1000
# directory = "C:\dev\suffix\make_repeats"
# REPEATED_STRING = "the long long long repeated string that keeps on going"
# --------------------------------------------------------------------------------
# 10 repeats
# 11 repeats

--------------------------------------------------------------------------
get_all_repeats: num repeated strings=74659, len= 5, time= 61.3747
repeated_strings: 74659 [     ,     ,     ,     ,     ,     	,     ,     2,     ?,     c,     t,     w,     {,     �,     �,     �,     �,     �,     �,     �,     �,     �,     ,    ,    ,    ,    ),    J,    M,    [,    b,    d,    q,    s,    y,    z,    �,    �,    �,     , ] 74659
74659 strings * 255 bytes = 19038045 vs 350468 valid_strings, 20971520 total offsets
--------------------------------------------------------------------------
get_all_repeats: num repeated strings=73674, len= 6, time= 80.4507
repeated_strings: 73674 [      ,      ,      ,      ,      ,     	 ,      ,     2 ,     ? ,     c ,     t ,     w ,     { ,     � ,     � ,     � ,     � ,     � ,     � ,     � ,     � ,     � ,      ,     ,     ,     ,    ) ,    J ,    M ,    [ ,    b ,    d ,    q ,    s ,    y ,    z ,    � ,    � ,    � ,      , ] 73674
73674 strings * 255 bytes = 18786870 vs 73337 valid_strings, 20971520 total offsets

--------------------------------------------------------------------------
Found 2 repeated strings of length 59
Repeated strings: 2 [PARFAYRBBNUUUZVRWTQEBJFYNHHSDWWTLMLVRRUQJPBFDOWMMAFJRDTESAJ, VLMFAWITVCMCQXSOSGHAFQSZAHANRQQXTIQHQPEPXZUKXDSTJSOOEZAAZFY, ] 2
--------------------------------------------------------------------------
Found 1 exactly repeated strings of length 41
Exactly repeated strings: 1 [ long repeated string that keeps on going, ] 1
duration = 95.1637
"""

import re
import sys

output = sys.argv[1]

f = open(output, 'wt')

size = 0.0
num_documents = 0
target_string = ''

patterns = {
    # size = 10.000 Mbyte
    'size': r'size\s*=\s*(\d*\.\d*)\s*Mbyte',

    # num_documents = 2
    'num_documents' : r'num_documents\s*=\s*(\d+)',

    # REPEATED_STRING = "the long long long repeated string that keeps on going"
    'REPEATED_STRING' : r'REPEATED_STRING\s*=\s*"([^"]+)"',

    # get_all_repeats: num repeated strings=73674, len= 6, time= 80.4507
    'get_all_repeats': r'get_all_repeats: num repeated strings\s*=\s*(\d+), len\s*=\s*(\d+), time\s*=\s*(\d*\.\d*)',

    # Found 1 exactly repeated strings of length 41
    'Found' : r'Found\s*(\d+)\s*exactly repeated strings of length\s*(\d+)',

    # Exactly repeated strings: 1 [ "long repeated string that keeps on going", ] 1
    'Exactly' : r'Exactly repeated strings:Found\s*(\d+)\s*\[ "(.+)",\s*\]',

    # duration = 95.1637
    'duration': r'duration\s*=\s*(\d*\.\d*)'
}

regexes = {k: re.compile(v) for k, v in patterns.items()}


def match(line):
    for k, v in regexes.items():
        m = re.search(line)
        if m:
            return k, m
    return None, None

last_time = 0.0
for line in output:
    k, m = match(line)
    if k == 'size':
        size = float(m.group(1))
    elif k == 'num_documents':
        num_documents = int(m.group(1))
    elif k == 'REPEATED_STRING':
        target_string = m.group(1)
    elif k == 'get_all_repeats':
        num_substrings = int(m.group(1))
        substring_len = int(m.group(2))
        time = float(m.group(3))
        round_time = time - last_time
        last_time = time
    elif k == 'duration':
        duration = float(m.group(1))


# size = 10.000 Mbyte
# min_repeats = 10
# num_documents = 2

