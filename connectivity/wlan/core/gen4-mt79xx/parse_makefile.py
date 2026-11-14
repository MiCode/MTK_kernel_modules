# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2021 MediaTek Inc.
#
import sys
import re
import subprocess
import os
import json
import datetime

# version 1.1 date:2025/3/24 author:rong.yan
# local test cmd:
# python parse_makefile.py Makefile $PWD compare_opts_str
# target: parse Makefile and defconfig into parsed_config.h
# add function:
# 1  split_var_and_value(line)
#    Split the line into variable part and expected value,
#    handling nested parentheses.
#    Args:
#        line (str): The line to split.
#    Returns:
#        tuple: A tuple containing the variable part and the expected value.
#    Raises:
#        ValueError: If the line has invalid syntax.
# 2 evaluate_findstring(needle, haystack)
#    Evaluate findstring function.
#    Args:
#        needle (str): The string to find.
#        haystack (str): The string in which to search.
#    Returns:
#        str: The needle if found in haystack, otherwise an empty string.
# 3 find_matching_parenthesis(expr, start)
#    Find the position of the matching closing parenthesis.
#    Args:
#        expr (str): The expression containing parentheses.
#        start (int): The starting position of the opening parenthesis.
#    Returns:
#        int: The position of the matching closing parenthesis.
#    Raises:
#        ValueError: If no matching closing parenthesis is found.
# 4 evaluate_condition(command)
#    Evaluate the condition part of the command.
#    Args:
#        command (str): The shell command containing the condition.
#    Returns:
#        bool: True if the condition is satisfied, otherwise False.
# 5 evaluate_shell_command(command, compare_opts)
#    Evaluate shell command with variables from compare_opts.
#
#    Args:
#        command (str): The shell command to evaluate.
#        compare_opts (dict): The dictionary containing variable values.
#
#    Returns:
#        str: 'true' if the condition in the command is satisfied,
#        otherwise an empty string.
# 6 evaluate_expression(expr, compare_opts)
#    Evaluate makefile expressions.
#    Args:
#        expr (str): The expression to evaluate.
#        compare_opts (dict): The dictionary containing variable values.
#    Returns:
#        str: The evaluated expression.
#    Raises:
#        ValueError: If an unsupported function is encountered.
# 7 parse_makefile(makefile_path, compare_opts)
#    Parse the makefile and evaluate its contents.
#    Args:
#        makefile_path (str): The path to the makefile.
#        compare_opts (dict): The dictionary containing variable values.
#    Returns:
#        None
# version 1.1 end
driver_build_date_added = False


def split_var_and_value(line):
    depth = 0
    split_index = -1
    for i in range(len(line) - 1, -1, -1):  # Iterate from the end to the start
        char = line[i]
        if char == ')':
            depth += 1
        elif char == '(':
            depth -= 1
        elif char == ',' and depth == 0:
            split_index = i
            break
    if split_index == -1:
        raise ValueError("Invalid syntax: {}".format(line))
    var_part = line[:split_index].strip()
    expected_value = line[split_index + 1:].strip()
    return var_part, expected_value


def evaluate_findstring(needle, haystack):
    return needle if needle in haystack else ''


def find_matching_parenthesis(expr, start):
    depth = 1
    for i in range(start + 2, len(expr)):
        if expr[i] == '(':
            depth += 1
        elif expr[i] == ')':
            depth -= 1
            if depth == 0:
                return i
    raise ValueError("No matching closing parenthesis found")


def evaluate_condition(command):
    # Extract the condition part (e.g., [ 16 -ge 15 ])
    condition_start = command.find('[')
    condition_end = command.find(']')
    if condition_start != -1 and condition_end != -1:
        condition = command[condition_start+1:condition_end].strip()
        # Split the condition into parts
        parts = condition.split()
        if len(parts) == 3:
            left_operand = int(parts[0])
            operator = parts[1]
            right_operand = int(parts[2])
            # Evaluate the condition
            if operator == '-ge':
                return left_operand >= right_operand
            elif operator == '-le':
                return left_operand <= right_operand
            elif operator == '-eq':
                return left_operand == right_operand
            elif operator == '-ne':
                return left_operand != right_operand
            elif operator == '-gt':
                return left_operand > right_operand
            elif operator == '-lt':
                return left_operand < right_operand
    return False


def evaluate_shell_command(command, compare_opts):
    # Replace variables in the command with their values from compare_opts
    while '$(' in command:
        start = command.find('$(')
        end = command.find(')', start)
        if start != -1 and end != -1:
            var_name = command[start+2:end]
            var_value = compare_opts.get(var_name, '')
            command = command[:start] + str(var_value) + command[end+1:]
        else:
            break
    # print command
    # Execute the shell command
    if evaluate_condition(command):
        return 'true'
    return ''


def evaluate_expression(expr, compare_opts):
    while '$(' in expr:
        start = expr.find('$(')
        # end = expr.find(')', start)
        end = find_matching_parenthesis(expr, start)
        inner_expr = expr[start+2:end]
        # print inner_expr
        if ',' in inner_expr:
            func, args = inner_expr.split(' ', 1)
            # args = args[:-1]  # Remove the last ')'
            if func == 'filter':
                pattern, text = args.split(',', 1)
                pattern = pattern.strip()
                text = text.strip()
                text_value = evaluate_expression(text, compare_opts)
                pattern_value = evaluate_expression(pattern, compare_opts)
                pattern_value = pattern_value.replace('%', '')
                text_list = text_value.split()
                pattern_list = pattern_value.split()
                # print pattern_list
                # print text_list
                for word in pattern_list:
                    for text_word in text_list:
                        if evaluate_findstring(word, text_word):
                            return word
                expr = ''
                # print expr
            elif func == 'findstring':
                needle, haystack = args.split(',', 1)
                needle = needle.strip()
                needle_value = evaluate_expression(needle, compare_opts)
                haystack = haystack.strip()
                haystack_value = evaluate_expression(haystack, compare_opts)
                found = evaluate_findstring(needle_value, haystack_value)
                expr = found
            elif func == 'strip':
                value = args.strip()
                expr = expr.replace('$(%s)' % inner_expr, value)
            elif func == 'call':
                if "kver_ge" in args:
                    expr = "1"
            else:
                raise ValueError("Unsupported function: {}".format(func))
        else:
            if "shell" in inner_expr:
                expr = evaluate_shell_command(inner_expr, compare_opts)
            elif 'wildcard' in inner_expr:
                expr = "1"
            else:
                var_name = inner_expr
                expr = expr.replace(
                  '$(%s)' % var_name,
                  compare_opts.get(var_name, '')
                )
    return expr


def parse_makefile(makefile_path, compare_opts):
    global driver_build_date_added
    with open(makefile_path, 'r') as f:
        lines = f.readlines()

    output = []
    # compare_opts = {}
    current_condition = True
    condition_stack = []  # Used to track nested conditions
    skiping_stack = []
    condition_stack.append(current_condition)
    skiping = False
    skiping_stack.append(skiping)
    skiping_r = False

    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            continue

        # Handle assignment statements
        # print line
        # print current_condition
        # print condition_stack
        # print skiping_stack
        if line.startswith('ifneq ($(wildcard') and current_condition:
            skiping_r = False

        if line.startswith('endif'):
            if skiping_stack:
                skiping_stack.pop()
                if not skiping_stack:
                    skiping_stack.append(False)
            if condition_stack:
                condition_stack.pop()
                if not condition_stack:
                    condition_stack.append(True)
                current_condition = condition_stack[-1]
            continue
        if skiping_stack[-1]:
            continue
        if skiping_r:
            continue
        if line.startswith('src:=') and current_condition:
            skiping_r = True
            continue
        assign_match = re.match(r'(\S+)\s*([?]?=|:=)\s*(.*)', line)
        if assign_match and current_condition:
            var_name = assign_match.group(1)
            operator = assign_match.group(2)
            value = assign_match.group(3).strip()
            while '$(' in value:
                start = value.find('$(')
                end = value.find(')', start)
                if start != -1 and end != -1:
                    var_name_temp = value[start+2:end]
                    var_value = compare_opts.get(var_name_temp, '')
                    value = value[:start] + str(var_value) + value[end+1:]
                else:
                    break
            if operator in ['=']:
                compare_opts[var_name] = value
                # print(var_name, compare_opts[var_name])
            elif operator in ['?='] and var_name not in compare_opts:
                compare_opts[var_name] = value
                # print(var_name, compare_opts[var_name])

            continue

        # Handle wildcard and include statements
        if line.startswith('include') and current_condition:
            include_path = line[8:].strip()
            # print include_path
            while '$(' in include_path:
                start = include_path.find('$(')
                end = include_path.find(')', start)
                if start != -1 and end != -1:
                    var_name = include_path[start+2:end]
                    var_value = compare_opts.get(var_name, '')
                    include_path = include_path[:start] + \
                        str(var_value) + include_path[end+1:]
                else:
                    break
            # print include_path
            # parse_makefile(include_path, compare_opts)
            if os.path.exists(include_path):
                parse_makefile(include_path, compare_opts)
            continue

        # Handle conditional statements

        if line.startswith('ifndef'):
            func, args = line.split(' ', 1)
            if not compare_opts.get(args, ''):
                current_condition = True
            else:
                current_condition = False
            condition_stack.append(current_condition)

        if line.startswith('ifdef'):
            func, args = line.split(' ', 1)
            if compare_opts.get(args, '') == '':
                current_condition = False
            else:
                current_condition = True
            condition_stack.append(current_condition)

        if line.startswith('ifeq'):
            # Parse ifeq statement
            line = line[5:].strip()  # Remove 'ifeq '
            line = line[1:-1]  # Remove outer parentheses
            var_part, expected_value = split_var_and_value(line)  # Split
            var_part = var_part.strip()  # Remove whitespace
            expected_value = expected_value.strip()  # Remove whitespace

            # Evaluate expression
            combined_value = evaluate_expression(var_part, compare_opts)
            expected_value = evaluate_expression(expected_value, compare_opts)
            # print(combined_value)  # Print combined value
            # print(expected_value)  # Print expected value
            current_condition = (combined_value == expected_value)
            if condition_stack and not condition_stack[-1]:
                current_condition = False
            condition_stack.append(current_condition)
            # print(current_condition)  # Print current condition
            continue

        if line.startswith('ifneq'):
            # Parse ifneq statement
            line = line[6:].strip()  # Remove 'ifneq '
            line = line[1:-1]  # Remove outer parentheses
            # var_part, expected_value = line.split(',', 1)  # Split
            var_part, expected_value = split_var_and_value(line)
            var_part = var_part.strip()  # Remove whitespace
            expected_value = expected_value.strip()  # Remove whitespace

            # Evaluate expression
            combined_value = evaluate_expression(var_part, compare_opts)
            expected_value = evaluate_expression(expected_value, compare_opts)
            # print(combined_value)  # Print combined value
            # print(expected_value)  # Print expected value
            current_condition = (combined_value != expected_value)
            if condition_stack and not condition_stack[-1]:
                current_condition = False
            condition_stack.append(current_condition)
            continue

        if line.startswith('else ifeq'):
            if condition_stack[-1]:
                skiping_stack.append(True)
                continue
            condition_stack.pop()  # Remove the last condition
            # Parse else ifeq statement
            line = line[9:].strip()  # Remove 'else ifeq '
            line = line[1:-1]  # Remove outer parentheses
            # var_part, expected_value = line.split(',', 1)  # Split
            var_part, expected_value = split_var_and_value(line)
            var_part = var_part.strip()  # Remove whitespace

            # Evaluate expression
            combined_value = evaluate_expression(var_part, compare_opts)
            expected_value = expected_value.strip()  # Remove whitespace
            # print(combined_value)  # Print combined value
            # print(expected_value)  # Print expected value
            current_condition = (combined_value == expected_value)
            if condition_stack and not condition_stack[-1]:
                current_condition = False
            condition_stack.append(current_condition)
            # print(current_condition)  # Print current condition
            continue

        if line.startswith('else ifneq'):
            if condition_stack[-1]:
                skiping_stack.append(True)
                continue
            condition_stack.pop()  # Remove the last condition
            # Parse else ifeq statement
            line = line[10:].strip()  # Remove 'else ifneq '
            line = line[1:-1]  # Remove outer parentheses
            # var_part, expected_value = line.split(',', 1)  # Split
            var_part, expected_value = split_var_and_value(line)
            var_part = var_part.strip()  # Remove whitespace

            # Evaluate expression
            combined_value = evaluate_expression(var_part, compare_opts)
            expected_value = expected_value.strip()  # Remove whitespace
            # print(combined_value)  # Print combined value
            # print(expected_value)  # Print expected value
            current_condition = (combined_value != expected_value)
            if condition_stack and not condition_stack[-1]:
                current_condition = False
            condition_stack.append(current_condition)
            # print(current_condition)  # Print current condition
            continue

        if line.startswith('else'):
            if condition_stack[-1]:
                skiping_stack.append(True)
                continue
            if condition_stack:
                current_conditionTop = condition_stack[-1]
                condition_stack.pop()
                current_conditionTop2 = condition_stack[-1] \
                    if condition_stack else True
                if current_conditionTop2 and not current_conditionTop:
                    current_condition = True
                else:
                    current_condition = False
                condition_stack.append(current_condition)
            continue

        # Handle ccflags-y appending
        ccflags_match = re.match(r'ccflags-y\s*\+=\s*(.*)', line)
        if ccflags_match and current_condition:
            ccflags_value = ccflags_match.group(1).strip()
            # print ccflags_value
            while '$(' in ccflags_value:
                start = ccflags_value.find('$(')
                end = ccflags_value.find(')', start)
                var_name = ccflags_value[start+2:end].strip()
                ccflags_value = ccflags_value.replace(
                  '$(%s)' % var_name,
                  compare_opts.get(var_name, '')
                )
            d_define_matches = re.findall(r'-D(\w+)(?:=(\S+))?', ccflags_value)
            for define_name, define_value in d_define_matches:
                if define_value:  # If there is a value after '='
                    if define_value.startswith("'") \
                            and define_value.endswith("'"):
                        define_value = define_value[1:-1]
                    define_statement = "#define {} {}" \
                        .format(define_name, define_value)
                else:  # If there is no value, just define the name
                    define_statement = "#define {}".format(define_name)
                output.append(define_statement)

    # Handle variable substitution in output
    for i in range(len(output)):
        statement = output[i]
        for var_name in compare_opts:
            statement = statement.replace(
              "$(%s)" % var_name,
              compare_opts[var_name] if var_name in compare_opts else ""
            )
        output[i] = statement

    # Print output
    if not driver_build_date_added:
        current_timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
        header_content = (
            '#define DRIVER_BUILD_DATE "{}"\n'.format(current_timestamp)
        )
        output.append(header_content)
        driver_build_date_added = True

    for statement in output:
        print(statement)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: parse_makefile.py <Makefile_path>")
        sys.exit(1)
    compare_opts_str = sys.argv[3]
    compare_opts = json.loads(compare_opts_str)
    # print sys.argv[2]
    # compare_opts["src"] = sys.argv[2]
    path = os.getcwd()
    out_index = path.find("/out")
    if out_index != -1:
        processed_path = path[:out_index] + sys.argv[2]
    else:
        processed_path = path
    compare_opts["src"] = processed_path
    compare_opts["CFG_DIR"] = compare_opts["src"] + "/configs"
    # print compare_opts
    parse_makefile(sys.argv[1], compare_opts)
