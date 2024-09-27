# !/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import argparse


class SetupError(Exception):
    """Functions in this script can raise this error which will cause the
    application to abort displaying the provided error message, but
    without a stack trace.
    """
    pass


"""
*@name get_struct_variable_name
*@breaf get struct variable name
*@param [in] single_line - a string from source file line by line
*@return [out] struct_name - type:string 
*                       description: structure name
"""


def get_struct_variable_name(single_line):
    single_line = single_line.lstrip()  # delete all space character in left
    pos_struct = single_line.find('struct')
    struct_name = single_line[pos_struct + 6:-1]
    if '{' in struct_name:
        pos_left_parenthesis = struct_name.find('{')
        struct_name = struct_name[0:pos_left_parenthesis]
    struct_name = struct_name.lstrip()
    struct_name = struct_name.rstrip()
    return struct_name


"""
*@name get_members_in_list
*@breaf get all variable member name in struct
*@param [in] member_content - The contents of all member variables in the struct
*@return [out] 
"""


def get_members_in_list(member_content):
    temp_list = []
    pos_left_parenthesis_in_content = member_content.find('{')
    pos_right_parenthesis_in_content = member_content.find('};')
    content = member_content[pos_left_parenthesis_in_content + 1:pos_right_parenthesis_in_content]
    temp_list = content.split(';')
    if '\n' in temp_list:
        temp_list.remove('\n')
    if '' in temp_list:
        temp_list.remove('')
    if ' ' in temp_list:
        temp_list.remove(' ')
    return temp_list


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", help="input file path")
    parser.add_argument("-o", "--output", help="output file path, if there is no output option,the script will "
                                               "overwrite input "
                                               "file.")
    args = parser.parse_args()
    fspath = args.input
    if args.output is None:
        output_fs = fspath
    else:
        output_fs = args.output

    struct_process_flag = False
    content = ""
    full_content = ""
    with open(fspath, "rU") as f:
        for line in f.readlines():
            full_content += line
            if 'struct' in line:
                struct_process_flag = True
                struct_name = get_struct_variable_name(line)
            if struct_process_flag is True:
                content += line
            if '};' in line:  # in this circumstance, content contains all structs member
                struct_process_flag = False
                # get str
                member_list = get_members_in_list(content)
                content = ""
                # get YLT_REFL macro string
                reflection_str = "YLT_REFL(" + struct_name + ','
                length_of_members = len(member_list)
                count = 0
                for member in member_list:
                    pos_last_space = member.rfind(' ')
                    member = member[pos_last_space:]
                    count = count + 1
                    if count != length_of_members:
                        reflection_str += member + ','
                    else:
                        reflection_str += member + ');'
                if line[-1] == '\n':
                    reflection_str = reflection_str + '\n'
                else:
                    reflection_str = '\n' + reflection_str + '\n'
                full_content += reflection_str
    with open(output_fs, 'w') as fp:
        fp.write(full_content)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except SetupError as err:
        print("errors: %s" % (err))
        sys.exit()
