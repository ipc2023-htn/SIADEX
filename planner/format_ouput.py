###############################################################################
# Parses the Siadex output into the format required for pandaPIparser validator
# 
# UNFINISHED, only produces first part of the output, no decomposition
###############################################################################

import sys

def parse_plan(lines):
    output = []
    output.append("==>")

    # Numbering and formatting
    identifier = 0
    for action in lines:
        line = action.replace(":action ", "")
        output.append(str(identifier) + " " + line)
        identifier += 1

    output.append("root")
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