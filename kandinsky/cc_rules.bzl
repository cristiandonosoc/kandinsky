load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

_ADDITIONAL_CXXOPTS = [
    "/W4",  # Enable a lot of warnings.
    "/WX",  # Warnings are errors.
    "/we4061",  # All switch cases must be handled.
    "/we5262",  # No implicit switch fallthrough.
]

def kdk_cc_library(**kwargs):
    kwargs["cxxopts"] = kwargs.get("cxxopts", []) + _ADDITIONAL_CXXOPTS
    cc_library(**kwargs)

def kdk_cc_binary(**kwargs):
    kwargs["cxxopts"] = kwargs.get("cxxopts", []) + _ADDITIONAL_CXXOPTS
    cc_binary(**kwargs)
