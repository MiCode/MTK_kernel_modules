load("@bazel_skylib//lib:collections.bzl", "collections")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

def generate_config_setting_bool(cfg_name, flag_name, enable_value, default_value):
    """
    generates config_setting with associated bool_flag and default value
    Args:
        cfg_name: flag name with cs_ as prefix
        flag_name: flag name by configuration
		enable_value: value to enable the config_setting
        default_value: default value
    """
    if default_value not in [0, 1, True, False]:
        default_value = False

    bool_flag(
        name = flag_name,
        build_setting_default = default_value,
    )

    native.config_setting(
        name = cfg_name,
        flag_values = {":"+flag_name: enable_value}
    )

def generate_config_setting_string(cfg_name, flag_name, flag_value, default_value, available_values):
    string_flag(
        name = flag_name,
        build_setting_default = default_value,
        values = available_values,
    )

    native.config_setting(
        name = cfg_name,
        flag_values = {":"+flag_name: flag_value}
    )


def generate_config_setting_multiple_string(cfg_name, flag_name, default_value, available_values):
    """
    generates multiple config_settings by array of params
    Args:
        cfg_name: flag name with cs_ as prefix for config_settting
        flag_name: string_flag name by configuration
		default_value: value to enable the config_setting
        available_values: list of values. For the corresponding config_setting,
			the value from the list also enables it.

    """
    string_flag(
        name = flag_name,
        build_setting_default = default_value,
        values = available_values,
    )

    for value in available_values:
        if value != "":
            native.config_setting(
                name = cfg_name +"_"+value,
                flag_values = {":"+flag_name: value}
            )

