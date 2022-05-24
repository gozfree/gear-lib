#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
#

import argparse
import os, sys, time, re
import subprocess


time_str_header = '''
#define BUILD_TIME_YEAR   {}
#define BUILD_TIME_MONTH  {}
#define BUILD_TIME_DAY    {}
#define BUILD_TIME_HOUR   {}
#define BUILD_TIME_MINUTE {}
#define BUILD_TIME_SECOND {}
#define BUILD_TIME_WEEK_OF_DAY {}
#define BUILD_TIME_YEAR_OF_DAY {}
'''

time_str_cmake = '''
set(BUILD_TIME_YEAR   "{}")
set(BUILD_TIME_MONTH  "{}")
set(BUILD_TIME_DAY    "{}")
set(BUILD_TIME_HOUR   "{}")
set(BUILD_TIME_MINUTE "{}")
set(BUILD_TIME_SECOND "{}")
set(BUILD_TIME_WEEK_OF_DAY "{}")
set(BUILD_TIME_YEAR_OF_DAY "{}")
'''

time_str_makefile = '''
BUILD_TIME_YEAR   ={}
BUILD_TIME_MONTH  ={}
BUILD_TIME_DAY    ={}
BUILD_TIME_HOUR   ={}
BUILD_TIME_MINUTE ={}
BUILD_TIME_SECOND ={}
BUILD_TIME_WEEK_OF_DAY ={}
BUILD_TIME_YEAR_OF_DAY ={}
'''

git_str_header = '''
#define BUILD_VERSION_MAJOR   {}
#define BUILD_VERSION_MINOR   {}
#define BUILD_VERSION_MICRO   {}
#define BUILD_VERSION_DEV     {}
#define BUILD_GIT_COMMIT_ID   "{}"
#define BUILD_GIT_IS_DIRTY    {}
'''

git_str_cmake = '''
set(BUILD_VERSION_MAJOR   "{}")
set(BUILD_VERSION_MINOR   "{}")
set(BUILD_VERSION_MICRO   "{}")
set(BUILD_VERSION_DEV     "{}")
set(BUILD_GIT_COMMIT_ID   "{}")
set(BUILD_GIT_IS_DIRTY    "{}")
'''

git_str_makefile = '''
BUILD_VERSION_MAJOR  ={}
BUILD_VERSION_MINOR  ={}
BUILD_VERSION_MICRO  ={}
BUILD_VERSION_DEV    ={}
BUILD_GIT_COMMIT_ID  ={}
BUILD_GIT_IS_DIRTY   ={}
'''

str_define_start_makefile  = "\n# compile append define start\n"
str_define_end_makefile    = "\n# compile append define end\n"
str_define_start_cmake     = "\n# compile append define start\n"
str_define_end_cmake       = "\n# compile append define end\n"
str_define_start_header    = "\n//compile append define start\n"
str_define_end_header      = "\n//compile append define end\n"

INFO_FORMAT_STR = {"header": [str_define_start_header, str_define_end_header, time_str_header, git_str_header],
                   "cmake":  [str_define_start_cmake,  str_define_end_cmake,  time_str_cmake,  git_str_cmake],
                   "makefile": [str_define_start_makefile, str_define_end_makefile, time_str_makefile, git_str_makefile],
                  }

def remove_old_config_info(start_flag_str, end_flag_str, content):
    match = re.findall(r"{}(.*){}".format(start_flag_str, end_flag_str), content, re.MULTILINE|re.DOTALL)
    if len(match) == 0:
        content += start_flag_str+end_flag_str
    else:
        content = content.replace(match[0], "")
    return content

def append_time_info(time_info_filename, version_info_filename, file_type):
    str_time_define_start = INFO_FORMAT_STR[file_type][0]
    str_time_define_end   = INFO_FORMAT_STR[file_type][1]
    append_format_time_str= INFO_FORMAT_STR[file_type][2]
    append_format_git_str = INFO_FORMAT_STR[file_type][3]
    content = ""
    content2 = ""
    content2_old = content2
    try:
        f = open(time_info_filename)
        content = f.read()
        f.close()
    except Exception:
        pass
    if version_info_filename:
        try:
            f = open(version_info_filename)
            content2 = f.read()
            content2_old = content2
            f.close()
        except Exception:
            pass
    time_now = time.localtime(time.time())
    # remove old config info
    content = remove_old_config_info(str_time_define_start, str_time_define_end, content)
    content2 = remove_old_config_info(str_time_define_start, str_time_define_end, content2)
    
    # time info
    time_define = append_format_time_str.format(time_now.tm_year,
                                        time_now.tm_mon,
                                        time_now.tm_mday,
                                        time_now.tm_hour,
                                        time_now.tm_min,
                                        time_now.tm_sec,
                                        time_now.tm_wday,
                                        time_now.tm_yday)
    # git info
    # add tag by command;
    #               git tag -a v0.1.1 -m "release v0.1.1 describe....."
    #               git push origin --tags 
    git_tag_name = ""
    version_major = 0
    version_minor = 0
    version_micro   = 0
    version_dev  = 0
    git_hash      = ""
    git_dirty     = ""
    try:
        git_tag = subprocess.check_output(["git", "describe", "--long", "--tag", "--dirty", "--always"], stderr=subprocess.STDOUT, universal_newlines=True).strip()
    except subprocess.CalledProcessError as er:
        if er.returncode == 128:
            # git exit code of 128 means no repository found
            print("== WARNING: NOT a git repository !!!")
        git_tag = ""
    except OSError:
        git_tag = ""
    # git_tag = "v0.3.2-39-gbeae86483-dirty"
    git_tag = git_tag.split("-")
    if len(git_tag) == 0:
        print("== WARNING: git get info fail")
    if len(git_tag) == 1:       # bdc1dcf
        git_hash = git_tag[0]
    elif len(git_tag) == 2:     # bdc1dcf-dirty or v0.1.1-bdc1dcf
        if git_tag[1] == "dirty":
            git_hash = git_tag[0]
            git_dirty = git_tag[1]
        else:
            git_tag_name = git_tag[0]
            git_hash = git_tag[1]
    elif len(git_tag) == 3:     # v0.1.1-10-bdc1dcf or v0.1.1-bdc1dcf-dirty
        if git_tag[2] == "dirty":
            git_tag_name = git_tag[0]
            git_hash = git_tag[1]
            git_dirty = git_tag[2]
        else:
            git_tag_name = git_tag[0]+"."+git_tag[1]
            git_hash = git_tag[2]
    else:                       # v0.1.1-10-bdc1dcf-dirty
        git_tag_name = git_tag[0]+"."+git_tag[1]
        git_hash = git_tag[2]
        git_dirty = git_tag[3]

    if git_tag_name.lower().startswith("v"):
        version = git_tag_name[1:].split(".")
        # convert to int from str
        for i,v in enumerate(version):
            try:
                version[i] = int(v)
            except Exception:
                version[i] = 0
        if len(version) >= 1:
            version_major = version[0]
        if len(version) >= 2:
            version_minor = version[1]
        if  len(version) >= 3:
            version_micro = version[2]
        if  len(version) >= 4:
            version_dev = version[3]
    if file_type == "header":
        dirty_value = 1 if git_dirty=="dirty" else 0
    elif file_type == "cmake":
        dirty_value = "y" if git_dirty=="dirty" else ""
    else:
        if git_dirty=="dirty":
            dirty_value = "y"
        else:
            append_format_git_str = append_format_git_str.replace("BUILD_GIT_IS_DIRTY", "# BUILD_GIT_IS_DIRTY")
            dirty_value = " is not set"
    git_define = append_format_git_str.format(version_major,
                                              version_minor,
                                              version_micro,
                                              version_dev,
                                              git_hash,
                                              dirty_value)
    # append time and git info to content
    content = content.split(str_time_define_end)
    if not version_info_filename:
        content = (time_define+git_define+str_time_define_end).join(content)
    else:
        content2 = content2.split(str_time_define_end)
        content = (time_define+str_time_define_end).join(content)
        content2 = (git_define+str_time_define_end).join(content2)
    # update config file
    with open(time_info_filename, "w") as f:
        f.write(content)
    if version_info_filename and content2 != content2_old:
        with open(version_info_filename, "w") as f:
            f.write(content2)

def write_config(filename):
    print("-- Update build time and version info to makefile config at: " + str(filename))
    time_info_filename = None
    version_info_filename = None
    if filename[0] != None and filename[0].lower() != "none" and os.path.exists(filename[0]):
        time_info_filename = filename[0]
    if filename[1] != None and filename[1].lower() != "none" and os.path.exists(filename[1]):
        version_info_filename = filename[1]
    if time_info_filename == None:
        raise Exception("param error")
    append_time_info(time_info_filename, version_info_filename, "makefile")

def write_cmake(filename):
    print("-- Update build time and version info to cmake  config  at: " + str(filename))
    time_info_filename = None
    version_info_filename = None
    if filename[0] != None and filename[0].lower() != "none" and os.path.exists(filename[0]):
        time_info_filename = filename[0]
    if filename[1] != None and filename[1].lower() != "none" and os.path.exists(filename[1]):
        version_info_filename = filename[1]
    if time_info_filename == None:
        raise Exception("param error")
    append_time_info(time_info_filename, version_info_filename, "cmake")

def write_header(filename):
    print("-- Update build time and version info to header  config  at: " + str(filename))
    time_info_filename = None
    version_info_filename = None
    if filename[0] != None and filename[0].lower() != "none":
        time_info_filename = filename[0]
    if filename[1] != None and filename[1].lower() != "none":
        version_info_filename = filename[1]
    if time_info_filename == None:
        raise Exception("param error")
    append_time_info(time_info_filename, version_info_filename, "header")

parser = argparse.ArgumentParser(description='generate time info for', prog=os.path.basename(sys.argv[0]))

OUTPUT_FORMATS = {"makefile": write_config,
                  "header": write_header,
                  "cmake": write_cmake
                  }


parser.add_argument('--configfile', nargs=3, action='append',
                        help='Write config file (format and output filename), version_filename can be None so all version info will append to time_filename',
                        metavar=('FORMAT', 'TIME_FILENAME', "VERSION_FILENAME"),
                        default=[])

args = parser.parse_args()

out_format = {}
for fmt, filename, version_filename in args.configfile:
    if fmt not in OUTPUT_FORMATS.keys():
        print("Format %s not supported! Known formats:%s" %(fmt, OUTPUT_FORMATS.keys()))
        sys.exit(1)
    out_format[fmt] = (filename, version_filename)

for fmt, filename in out_format.items():
    # if not os.path.exists(filename):
    #     print("File not found:%s" %(filename))
    # not check always create
    func = OUTPUT_FORMATS[fmt]
    func(filename)


