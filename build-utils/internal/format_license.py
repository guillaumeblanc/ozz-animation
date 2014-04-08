#!/usr/bin/python
#*******************************************************************************
#                                                                              #
#  ozz-animation, 3d skeletal animation libraries and tools.                   #
#  https://code.google.com/p/ozz-animation/                                    #
#                                                                              #
#------------------------------------------------------------------------------#
#                                                                              #
#  Copyright (c) 2012-2014 Guillaume Blanc                                     #
#                                                                              #
#  This software is provided 'as-is', without any express or implied           #
#  warranty. In no event will the authors be held liable for any damages       #
#  arising from the use of this software.                                      #
#                                                                              #
#  Permission is granted to anyone to use this software for any purpose,       #
#  including commercial applications, and to alter it and redistribute it      #
#  freely, subject to the following restrictions:                              #
#                                                                              #
#  1. The origin of this software must not be misrepresented; you must not     #
#  claim that you wrote the original software. If you use this software        #
#  in a product, an acknowledgment in the product documentation would be       #
#  appreciated but is not required.                                            #
#                                                                              #
#  2. Altered source versions must be plainly marked as such, and must not be  #
#  misrepresented as being the original software.                              #
#                                                                              #
#  3. This notice may not be removed or altered from any source                #
#  distribution.                                                               #
#                                                                              #
#******************************************************************************#

import os, glob
import sys
import itertools
import string
import re
import time

def recurse_files(_folder, _filter):
    # Iterate files.
    for i in glob.iglob(os.path.join(_folder, _filter)):
        yield i
    # Iterate folders...
    for i in glob.iglob(os.path.join(_folder, '*/')):
        #... and recurse them.
        for j in recurse_files(i, _filter):
            if j.find('\\extern\\') == -1 and j.find('\\build\\') == -1:
                yield j

license_text = "\
//============================================================================//\n\
//                                                                            //\n\
// ozz-animation, 3d skeletal animation libraries and tools.                  //\n\
// https://code.google.com/p/ozz-animation/                                   //\n\
//                                                                            //\n\
//----------------------------------------------------------------------------//\n\
//                                                                            //\n\
// Copyright (c) 2012-2014 Guillaume Blanc                                    //\n\
//                                                                            //\n\
// This software is provided 'as-is', without any express or implied          //\n\
// warranty. In no event will the authors be held liable for any damages      //\n\
// arising from the use of this software.                                     //\n\
//                                                                            //\n\
// Permission is granted to anyone to use this software for any purpose,      //\n\
// including commercial applications, and to alter it and redistribute it     //\n\
// freely, subject to the following restrictions:                             //\n\
//                                                                            //\n\
// 1. The origin of this software must not be misrepresented; you must not    //\n\
// claim that you wrote the original software. If you use this software       //\n\
// in a product, an acknowledgment in the product documentation would be      //\n\
// appreciated but is not required.                                           //\n\
//                                                                            //\n\
// 2. Altered source versions must be plainly marked as such, and must not be //\n\
// misrepresented as being the original software.                             //\n\
//                                                                            //\n\
// 3. This notice may not be removed or altered from any source               //\n\
// distribution.                                                              //\n\
//                                                                            //\n\
//============================================================================//\n\
\n\
#"

def process_file(_file, _is_h):
    # Read
    fr = open(_file, 'rt')
    if fr == None:
        print "Failed to read file " + _file
        return False
    
    text = fr.read();
    fr.close()

    # Prepares output
    output = ""
    modified = False;

    # Test for a valid license
    find_license = string.find(text, license_text)
    if find_license == -1:
        # Replaces license up to the first '#'
        first_define = string.find(text, '#')
        if first_define != -1:
            output = license_text + text[first_define + 1:]
        else:
            output = license_text + text
        modified = True
    else:
        output = text
        
    # '#' must be found as it is part of the license
    first_define = string.find(output, '#')

    if _is_h:
        # Test for a valid #define GUARD

        # Removes pragma once
        pragma_once = '#pragma once'
        pragma_found = string.find(output[first_define:], pragma_once)
        if  pragma_found == 0:
            output = output[:first_define] + output[first_define + len(pragma_once):]
            modified = True

        # Prepares #define GUARD
        guard = string.upper(_file)
        to_replace = ['/', '\\', '.', '-']
        for i in to_replace:
            guard = string.replace(guard, i, '_') 
        to_remove = ['INCLUDE_', 'SRC_']
        for i in to_remove:
            guard = string.replace(guard, i, '') 
        guard = 'OZZ_' + guard + '_'
        guard = re.sub('_+', '_', guard)

        header = '#ifndef ' + guard + '\n' + '#define ' + guard + '\n'
        footer = '#endif  // ' + guard + '\n'

        # Test for a valid header/footer
        found_match = re.search('^#ifndef (?P<found_guard>.+)\n^#define (?P=found_guard)\n(.*\n)*^#endif  // (?P=found_guard)',
                                output[first_define:],
                                re.MULTILINE);
        if found_match == None:
            output = output[:first_define] + header + output[first_define:]
            if not output.endswith('\n'):
                output += '\n'
            output += footer;
            modified = True
        elif found_match.group('found_guard') != guard:
            output = string.replace(output, found_match.group('found_guard'), guard)
            modified = True

        # Needs at least 1 \n
        if not output.endswith('\n'):
            output += '\n'

        # remove \n after guard begin   
        guard_pos = output.find(header) + len(header) - 1
        if not output.startswith('\n\n', guard_pos):
            output = output[:guard_pos] + '\n' + output[guard_pos:]
            modified = True
        while output.startswith('\n\n\n', guard_pos):
            output = output[:guard_pos] + output[guard_pos+1:]
            modified = True

        # remove \n before guard end
        guard_pos = output.find(footer)
        while output.endswith('\n\n', 0, guard_pos):
            output = output[:guard_pos-1] + output[guard_pos:]
            guard_pos = output.find(footer)
            modified = True

    # must end with a single \n
    if not output.endswith('\n'):
        output += '\n'
        modified = True
    else:
        while output.endswith('\n\n'):
            output = output[:len(output) - 1]
            modified = True

    # Write
    if modified:
        fw = open(_file, 'wt')
        if fw == None:
            print "Failed to write file " + _file
            modified = False
        fw.write(output)
        fw.close()
        print _file + ' modified'

    return modified

def process_h(_file):
    return process_file(_file, True)

def process_cc(_file):
    return process_file(_file, False)
    
def main():
    # Process .h
    for i in recurse_files('../../', '*.h'):
        process_h(i)
    # Process .cc
    for i in recurse_files('../../', '*.cc'):
        process_cc(i)
    #
    print 'Terminated'

    while 1:
        time.sleep(10)

def main():
    # Process .h
    for i in recurse_files('../../', '*.h'):
        process_h(i)
    # Process .cc
    for i in recurse_files('../../', '*.cc'):
        process_cc(i)
    #
    print 'Terminated'

    while 1:
        time.sleep(10)
main()
