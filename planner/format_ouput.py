###############################################################################
# Parses the Siadex output into the format required for pandaPIparser validator
###############################################################################

import sys
import re
from collections import OrderedDict

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def parse_plan(lines, tasklist):
    """ Produces the plan part in the final output """
    output = []
    output.append("==>")

    # Numbering and formatting
    identifier = last_identifier = -1  # Any negative number

    for action in lines:
        line = action.replace(":action ", "")
        line = re.sub('_primitive ', ' ', line)
        identifier_list = get_subtasks_ids([line], tasklist, plan=True)
        
        # If only one id was returned, the task is a primitive from the goal,
        # so no need to consider the previous identifier
        if len(identifier_list) > 1:
            identifier_list = [i for i in identifier_list if int(i) > int(identifier)]
            last_identifier = identifier_list[0]
            
        identifier = identifier_list[0]
        output.append(str(identifier) + " " + line[1:-1])

    # don't output an empty plan if there is none
    if (last_identifier == -1 and identifier == -1):
        output = []

    return output

def get_DT(output_part):
    """ Produces the decomposition part in the final output """
    output_part = output_part.split("Lista de tareas: \n")

    # Get tasks and their ids
    tasks_headers, tasks_ids = get_tasks(output_part[1])

    # Iterate for each tasks, getting the important info from its method
    info, roots = get_method_info(output_part[0])

    # Construct the decomposition tree    
    return parse_DT(tasks_headers, info, roots, tasks_ids), tasks_headers

# ------------------------------------------------------------------------------

# Doesn't store the primitives
def get_tasks(tasks):
    """ Returns two dictionaries: id-task_header and id-task_name """
    lines = list(filter(None, tasks.splitlines()))
    tasks_headers = OrderedDict()
    tasks_names = OrderedDict()

    for line in lines:
        identifier =  re.search(r"\[([0-9_]+)\]", line).group(1)

        header = re.search(r"(?::([^.]+)) (\(.*\))", line).group(2)
        match = re.search(r"(?::([^.]+)) \((.*?)\ ", line)
        if match:
            name = match.group(2)
            tasks_headers.setdefault(identifier,header)
            tasks_names.setdefault(identifier,name)

    return tasks_headers, tasks_names

# ------------------------------------------------------------------------------

def get_method_info(text):
    """ Returns information for each tasks and the root ones """
    # Split by delimiter
    blocks = text.split("===")

    # Get roots ids
    roots = blocks[0].split("Root:")[1].split("-")
    roots.remove('\n')

    info = []
    for block in blocks[1::]:
        if block != '\n':
            inf = parse_block(block)
            info.append(inf)

    return info, roots

# ------------------------------------------------------------------------------

def parse_block(block):
    """ Returns the task id, the name of the method and of its subtasks """
    # Task id
    task =  re.search(r"Tarea:(\d+)", block).group(1)

    # Method name (ignores wrapper methods)
    method = None
    match = re.search(r":method (\w+(?:-\w+)*)", block)
    if match:
        method = match.group(1)

    # If it is not a primitive
    subtasks_names = None
    if method:
        subtasks = block.split(":tasks (")[1]   # Removing non necessary lines
        subtasks = subtasks[1:-5]
        subtasks_names = []

        # Name of the subtasks 
        for subt in list(filter(None,subtasks.splitlines())):
            subtasks_names.append(subt.strip())
        
    return (task, method, subtasks_names)

# ------------------------------------------------------------------------------

def get_subtasks_ids(subtasks, taskslist, plan=False, used_ids=None):
    ids = []
    dic = {}  
    
    for subt in subtasks:
        for ident, name in taskslist.items():
            if name == subt:
                if not plan:    # If we are not looking ids for the plan part
                    if ident not in used_ids: # If id not already used in another task
                        if not dic.get(name, None):
                            ids.append(ident)
                            dic.setdefault(name, ident)                    
                else:
                    ids.append(ident)

    return ids

# ------------------------------------------------------------------------------

def parse_DT(tasks, info, roots, tasks_names):
    output = []

    # Produce root line
    output.append("root " + ' '.join(str(x) for x in roots))

    used_ids = []    # List of already included ids

    for task in info:
        ident = task[0]
        method = task[1]

        if method != None:
            subtasks = get_subtasks_ids(task[2], tasks, used_ids=used_ids)
            used_ids.extend(subtasks)

            # len==0 -> An inline method (empty subtasks)
            # len==1 && !'primitive' -> not a wrapper
            if len(subtasks) > 1 or len(subtasks) == 0 or  (len(subtasks) == 1 and 'primitive' not in tasks[subtasks[0]]):
                output.append(ident + " " + tasks[ident][1:-1] + " -> " + method 
                                    + " " + " ".join(subtasks))

    output.append("<==")

    return output

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def main(argv):
    # --------------------------------------------------------------------------
    # Opening input plan

    with open(argv[1], "r") as f:    
        output = f.read()
        # Remove until 'Root' is seen    
        index = output.find("Root")
        output = output[index:]

        # Separating the plan and the decomposition
        output_parts = output.split("###")
        
    # Removing empty lines
    lines = list(filter(None, output_parts[1].splitlines()))

    decomposition, tasklist = get_DT(output_parts[0])
    plan = parse_plan(lines, tasklist)
    plan.extend(decomposition)

    # ----------------------------------------------------------------------
    # Printing the output

    for line in plan:
        print(line)
    

if __name__ == "__main__":
    main(sys.argv)