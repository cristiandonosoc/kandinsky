load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

_ADDITIONAL_COPTS = [
    "/W4", # Enable a lot of warnings.
    "/WX", # Warnings are errors.
    "/we5262", # No implicit switch fallthrough.
]

def kdk_cc_library(**kwargs):
    kwargs["cxxopts"] = kwargs.get("cxxopts", []) + _ADDITIONAL_COPTS
    cc_library(**kwargs)

def kdk_cc_binary(**kwargs):
    kwargs["cxxopts"] = kwargs.get("cxxopts", []) + _ADDITIONAL_COPTS
    cc_binary(**kwargs)
