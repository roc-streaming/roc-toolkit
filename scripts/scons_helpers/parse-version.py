from __future__ import print_function

import os
import os.path
import re

os.chdir(os.path.dirname(os.path.realpath(__file__)))

fp = open('../../src/public_api/include/roc/version.h')

data = fp.read()

m = re.search(r"""^#define ROC_VERSION_MAJOR (\d+)$""", data, re.MULTILINE)
major = m.group(1)

m = re.search(r"""^#define ROC_VERSION_MINOR (\d+)$""", data, re.MULTILINE)
minor = m.group(1)

m = re.search(r"""^#define ROC_VERSION_PATCH (\d+)$""", data, re.MULTILINE)
patch = m.group(1)

print('.'.join([major, minor, patch]))
