#!/usr/bin/python
import os
import argparse
import time

default_online_debug_ops = "/sys/devices/platform/11cc0000.i2c/i2c-8/8-0010/sensor_setting_online_debug_ops"
default_init_setting_file = "./init_setting.txt"
default_mode_setting_file = "./mode_setting.txt"
default_streamon_setting_file = "./streamon_setting.txt"
default_streamoff_setting_file = "./streamoff_setting.txt"
online_debug_ops = default_online_debug_ops;

def example():
    print("--------------------------------------------------");
    cmd = "./sensor_setting_debug.py --debug_enable=1 --sensor_reg_id='0x1641' --sensor_mode=0 --init_setting_file='%s' --mode_setting_file='%s'  | tee log.txt" \
        % (default_init_setting_file, default_mode_setting_file)
    print("example 1 : \n" + cmd)
    print("--------------------------------------------------");
    cmd = "./sensor_setting_debug.py --debug_enable=1 --sensor_reg_id='0x1641' --sensor_mode=0 --init_setting_file='%s' --mode_setting_file='%s' --streamon_setting_file='%s' --streamoff_setting_file='%s' | tee log.txt" \
        % (default_init_setting_file, default_mode_setting_file, default_streamon_setting_file, default_streamoff_setting_file)
    print("example 2 : \n" + cmd)
    print("--------------------------------------------------");
    pass

def read_setting(setting_file):
    ret = ""
    fd = open(setting_file)
    line = fd.readline()
    while line:
        ret += " ".join(line.replace(',', '').split())
        ret += " "
        line = fd.readline()
    # print(ret)
    return ret.strip().split(' ')

def write_setting(key_word, content):
    setting_str = key_word
    for data in content:
        setting_str += " " + str(data)
    # print(setting_str)
    cmd = "adb shell \" echo %s > %s \"" % (setting_str, online_debug_ops)
    fd = os.popen(cmd)
    time.sleep(0.1)
    print(cmd)
    pass

def construct_header(en, id, mode):
    header_info = [''] * 4
    header_info[0] = str(en)
    header_info[1] = str(id)
    header_info[2] = str(mode)
    header_info[3] = '0'
    return header_info

def setting_split(setting_in):
    threshold = 500
    return [setting_in[i:i+threshold] for i in range(0, len(setting_in), threshold)]

def main(debug_enable, sensor_reg_id, sensor_mode, init_setting, mode_setting, streamon_setting, streamoff_setting):
    header_info = construct_header(debug_enable, sensor_reg_id, sensor_mode)
    write_setting("HEADER_INFO", header_info)
    if not debug_enable:
        print("reset online debug");
    else:
        if len(init_setting):
            # print(init_setting)
            settings = setting_split(init_setting)
            for setting in settings:
                print("-----------\nlen = %d" % (len(setting)))
                write_setting("INIT_SETTING", setting)
        if len(mode_setting):
            settings = setting_split(mode_setting)
            for setting in settings:
                print("-----------\nlen = %d" % (len(setting)))
                write_setting("MODE_SETTING", setting)
        if len(streamon_setting):
            write_setting("STREAMON_SETTING", streamon_setting)
        if len(streamoff_setting):
            write_setting("STREAMOFF_SETTING", streamoff_setting)
        print("debug_enable =", debug_enable, "sensor_reg_id =", sensor_reg_id, "sensor mode =", sensor_mode, "header_info =", header_info)
    pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Sensor Setting Debug Tool', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--debug_enable', type = int, default = 0, help="1 use online setting, 0 use offline setting")
    parser.add_argument('--sensor_reg_id', type = str, default = '0x00', help="sensor reg id")
    parser.add_argument('--sensor_mode', type = int, default = 0, help="sensor mode 0, 1, 2...")
    parser.add_argument('--init_setting_file', type = str, default = '', help="path/of/init_setting_file")
    parser.add_argument('--mode_setting_file', type = str, default = '', help="path/of/mode_setting_file")
    parser.add_argument('--streamon_setting_file', type = str, default = '', help="path/of/streamon_setting_file")
    parser.add_argument('--streamoff_setting_file', type = str, default = '', help="path/of/streamoff_setting_file")
    parser.add_argument('--online_debug_ops', type = str, default = default_online_debug_ops, help="debug ops path")
    args = parser.parse_args()
    online_debug_ops = args.online_debug_ops;

    if args.debug_enable != 0 and args.debug_enable != 1:
        print("error debug_enable")
        parser.print_help()
        example()
        exit
    # elif args.sensor_reg_id < 0 or args.sensor_reg_id > 4:
    #     print("error sensor_reg_id")
    #     parser.print_help()
    #     example()
    #     exit
    elif args.sensor_mode < 0 or args.sensor_mode > 10:
        print("error sensor_mode")
        parser.print_help()
        example()
        exit
    else:
        init_setting = []
        mode_setting = []
        streamon_setting = []
        streamoff_setting = []
        if len(args.init_setting_file):
            init_setting = read_setting(args.init_setting_file)
        if len(args.mode_setting_file):
            mode_setting = read_setting(args.mode_setting_file)
        if len(args.streamon_setting_file):
            streamon_setting = read_setting(args.streamon_setting_file)
        if len(args.streamoff_setting_file):
            streamoff_setting = read_setting(args.streamoff_setting_file)

        if len(init_setting) or len(mode_setting) or len(streamon_setting) or len(streamoff_setting):
            main(args.debug_enable, args.sensor_reg_id, args.sensor_mode, init_setting, mode_setting, streamon_setting, streamoff_setting)
        else:
            print("no input setting");
            parser.print_help()
            example()