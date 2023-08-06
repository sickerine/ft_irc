from erk import *

_ERK_PLUGIN_ = "Generated Plugin"

class DumpPlugin(Plugin):
    NAME = "Blank Plugin"
    VERSION = "1.0"
    DESCRIPTION = "This is a blank plugin"

    def line_in(self,line):
        print(line)

    def line_out(self,line):
        print(line)
