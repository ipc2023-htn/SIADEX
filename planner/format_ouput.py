###############################################################################
# Parses the Siadex output into the format required for pandaPIparser validator
# 
# UNFINISHED, only produces first part of the output, no decomposition
###############################################################################

import sys
import re

def parse_plan(lines):
    output = []
    output.append("==>")

    # Numbering and formatting
    identifier = 0
    for action in lines:
        line = action.replace(":action ", "")
        line = re.sub('_primitive ', ' ', line)
        output.append(str(identifier) + " " + line)
        identifier += 1

    output.append("root -1")
    output.append("<==")

    return output

def main(argv):
    # --------------------------------------------------------------------------
    # Opening input plan

    input_file = open(argv[1], "r")
    lines = [line.rstrip("\n") for line in input_file]
    input_file.close()

    output = parse_plan(lines)

    # ----------------------------------------------------------------------
    # Printing the output

    for line in output:
        print(line)
    

if __name__ == "__main__":
    main(sys.argv)
