import sys
import cComment

cddlBlock = '''\
CDDL HEADER START

This file and its contents are supplied under the terms of the
Common Development and Distribution License ("CDDL"), version 1.0.
You may only use this file in accordance with the terms of version
1.0 of the CDDL.

A full copy of the text of the CDDL should have accompanied this
source.  A copy of the CDDL is also available via the Internet at
http://www.illumos.org/license/CDDL.

When distributing Covered Code, include this CDDL HEADER in each
file and include the License file at usr/src/SYSTEMXVI.LICENSE.
If applicable, add the following below this CDDL HEADER, with the
fields enclosed by brackets "[]" replaced with your own identifying
information: Portions Copyright [yyyy] [name of copyright owner]

CDDL HEADER END
'''

if len(sys.argv) != 2:
    sys.exit("Must provide 1 argument only (path to the file)")

print(cComment.toCComment(cddlBlock)) 

txt = open(sys.argv[1], "r").read()

print("Checking for CDDL header: ", end = '')

if cComment.toCComment(cddlBlock) in txt:
    print("yes")
else:
    print("no")
