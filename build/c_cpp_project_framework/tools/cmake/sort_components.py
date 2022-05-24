#
# sort components according to priority.conf file
#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
# @license Apache 2.0
#
# @usage: python sort_components.py priority.conf component_dir1 component_dir2 ... component_dir4
#

import sys, os

conf_file =  sys.argv[1]
components = sys.argv[2:]

if not os.path.exists(conf_file):
    exit(2)

try:
    conf = ""
    f = open(conf_file)
    while True:
        line = f.readline()
        if not line:
            break
        line = line.strip()
        if line.startswith("#") or line == "":
            continue
        conf += line +" "
    f.close()
except Exception as e:
    print("[ERROR] "+str(e))
    exit(1)

components_ordered = conf.split()
dict_order = {}
for i,component in enumerate(components_ordered):
    dict_order[component] = i

final_components = []
components_not_ordered = []
for component in components: # all components
    name = os.path.basename(component)
    if name in dict_order.keys(): # have priority in config file
        find_pos = False
        if len(final_components) == 0:
            find_pos = True
            final_components.append(component)
        else:
            for j,tmp in enumerate(final_components):
                tmp_name = os.path.basename(tmp)
                if dict_order[name] < dict_order[tmp_name]:
                    find_pos = True
                    final_components.insert(j, component)
                    break
        if not find_pos:
            final_components.append(component)
    else: # no priority in config file
        components_not_ordered.append(component)
final_components += components_not_ordered

for i in final_components:
    print(i, end = ";")


