#!/usr/bin/env python

import sys
import os
sys.path.append("../../../../build_tools/scripts")
import base

def clear_module():
  if base.is_dir("brotli"):
    base.delete_dir_with_access_error("brotli")
  return

base.check_module_version("1", clear_module)

# fetch harfbuzz
if not base.is_dir("brotli"):
  base.cmd("git", ["clone", "https://github.com/google/brotli.git"])
  os.chdir("brotli")
  base.cmd("git", ["checkout", "db24a98aed38a16de78cc05ca15cdd8dc2a0d92d"])
  os.chdir("../")
