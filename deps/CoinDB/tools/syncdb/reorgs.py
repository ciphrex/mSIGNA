# Run this utility on the sync.log file to find reorganizations

import sys, re

if len(sys.argv) < 2:
    print "# Usage: " + sys.argv[0] + " <filename>"
    sys.exit()

filename = sys.argv[1]

prevheight = ''

with open(filename) as fp:
    for line in fp:
        m = re.search('Sync height: [0-9]+$', line)
        if m:
            height = m.group(0)
            #print height
            if height <= prevheight:
                print "reorg at " + height
            prevheight = height
