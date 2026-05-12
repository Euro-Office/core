#!/usr/bin/env python

import sys
import os
sys.path.append('../../../../build_tools/scripts')
import base

# suppresses status output when set via --quiet/-q; errors are still shown
QUIET = False

def log(msg):
  if not QUIET:
    print(msg)
  return

def log_info(msg):
  if not QUIET:
    base.print_info(msg)
  return

# ---------------------------------------------------------------------------
# Per-target defaults (mirror LIBXML2_WITH_* options from CMakeLists.txt
# and the platform branches inside configure.ac)
# ---------------------------------------------------------------------------
COMMON_FEATURES = {
  "WITH_C14N":         0,
  "WITH_CATALOG":      0,
  "WITH_DEBUG":        0,
  "WITH_HTML":         1,
  "WITH_HTTP":         0,
  "WITH_ICONV":        0,
  "WITH_ICU":          0,
  "WITH_ISO8859X":     0,
  "WITH_MODULES":      0,
  "WITH_OUTPUT":       1,
  "WITH_PATTERN":      0,
  "WITH_PUSH":         1,
  "WITH_READER":       1,
  "WITH_REGEXPS":      0,
  "WITH_RELAXNG":      0,
  "WITH_SAX1":         1,
  "WITH_SCHEMAS":      0,
  "WITH_SCHEMATRON":   0,
  "WITH_THREADS":      0,
  "WITH_THREAD_ALLOC": 0,
  "WITH_VALID":        0,
  "WITH_WRITER":       0,
  "WITH_XINCLUDE":     0,
  "WITH_XPATH":        0,
  "WITH_XPTR":         1,
  "WITH_ZLIB":         0
}

TARGETS = {
  "windows": {
    "WITH_WINPATH":                   1,
    "HAVE_DECL_GETENTROPY":           0,
    "HAVE_DECL_GLOB":                 0,
    "HAVE_DECL_MMAP":                 0,
    "HAVE_FUNC_ATTRIBUTE_DESTRUCTOR": 0,
    "HAVE_DLOPEN":                    0,
    "HAVE_SHLLOAD":                   0,
    "HAVE_LIBHISTORY":                0,
    "HAVE_LIBREADLINE":               0,
    "HAVE_STDINT_H":                  1,
    "XML_SYSCONFDIR":                 "/etc",
    "MODULE_EXTENSION":               ".dll",
    "XML_THREAD_LOCAL":               "__declspec(thread)"
  },
  "linux": {
    "WITH_WINPATH":                   0,
    "HAVE_DECL_GETENTROPY":           0,
    "HAVE_DECL_GLOB":                 1,
    "HAVE_DECL_MMAP":                 1,
    "HAVE_FUNC_ATTRIBUTE_DESTRUCTOR": 1,
    "HAVE_DLOPEN":                    1,
    "HAVE_SHLLOAD":                   0,
    "HAVE_LIBHISTORY":                0,
    "HAVE_LIBREADLINE":               0,
    "HAVE_STDINT_H":                  1,
    "XML_SYSCONFDIR":                 "/etc",
    "MODULE_EXTENSION":               ".so",
    "XML_THREAD_LOCAL":               "__thread"
  },
  "mac": {
    "WITH_WINPATH":                   0,
    "HAVE_DECL_GETENTROPY":           0,
    "HAVE_DECL_GLOB":                 1,
    "HAVE_DECL_MMAP":                 1,
    "HAVE_FUNC_ATTRIBUTE_DESTRUCTOR": 1,
    "HAVE_DLOPEN":                    1,
    "HAVE_SHLLOAD":                   0,
    "HAVE_LIBHISTORY":                0,
    "HAVE_LIBREADLINE":               0,
    "HAVE_STDINT_H":                  1,
    "XML_SYSCONFDIR":                 "/etc",
    "MODULE_EXTENSION":               ".dylib",
    "XML_THREAD_LOCAL":               "__thread"
  }
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def build_defaults(target):
  if target not in TARGETS:
    base.print_error("unknown target: " + target)
    sys.exit(1)
  cfg = {}
  for k in COMMON_FEATURES:
    cfg[k] = COMMON_FEATURES[k]
  for k in TARGETS[target]:
    cfg[k] = TARGETS[target][k]
  return cfg

def derive(cfg):
  parts = cfg["VERSION"].split(".")
  major = int(parts[0])
  minor = int(parts[1])
  micro = int(parts[2])
  cfg["LIBXML_MAJOR_VERSION"]  = major
  cfg["LIBXML_MINOR_VERSION"]  = minor
  cfg["LIBXML_MICRO_VERSION"]  = micro
  cfg["LIBXML_VERSION_NUMBER"] = major * 10000 + minor * 100 + micro
  if "LIBXML_VERSION_EXTRA" not in cfg:
    cfg["LIBXML_VERSION_EXTRA"] = ""
  return

def subst_atvars(text, cfg):
  for key in cfg:
    text = text.replace("@" + key + "@", str(cfg[key]))
  # warn about leftover placeholders (helps catch typos in xmlversion.h.in)
  i = text.find("@")
  while i != -1:
    j = text.find("@", i + 1)
    if j == -1:
      break
    token = text[i + 1:j]
    is_name = (len(token) > 0)
    for ch in token:
      if not (ch.isalnum() or ch == "_"):
        is_name = False
        break
    if is_name:
      base.print_error("undefined template variable: @" + token + "@")
    i = text.find("@", j + 1)
  return text

def process_cmakedefine_line(line, cfg):
  stripped = line.lstrip()
  if not stripped.startswith("#"):
    return None
  indent = line[:len(line) - len(stripped)]
  body = stripped[1:].lstrip()    # skip '#' and any whitespace after it

  if body.startswith("cmakedefine01"):
    rest = body[len("cmakedefine01"):].strip()
    if rest == "":
      return None
    name = rest.split()[0]
    value = 1 if int(cfg.get(name, 0)) else 0
    return indent + "#define " + name + " " + str(value)

  if body.startswith("cmakedefine"):
    rest = body[len("cmakedefine"):]
    if rest == "" or not rest[0].isspace():
      return None
    rest = rest.lstrip()
    sp = -1
    for k in range(len(rest)):
      if rest[k].isspace():
        sp = k
        break
    if sp == -1:
      name = rest
      tail = ""
    else:
      name = rest[:sp]
      tail = rest[sp:]
    if cfg.get(name):
      return indent + "#define " + name + subst_atvars(tail, cfg)
    return indent + "/* #undef " + name + " */"

  return None

def process_template(text, cfg, cmake_style):
  out_lines = []
  for line in text.splitlines():
    if cmake_style:
      replaced = process_cmakedefine_line(line, cfg)
      if replaced is not None:
        out_lines.append(replaced)
        continue
    out_lines.append(subst_atvars(line, cfg))
  return "\n".join(out_lines) + "\n"

def configure_file(src, dst, cfg, cmake_style):
  if not base.is_file(src):
    base.print_error("template not found: " + src)
    sys.exit(1)
  text = base.readFile(src)
  dst_dir = os.path.dirname(base.get_path(dst))
  if dst_dir != "":
    base.create_dir(dst_dir)
  base.writeFile(dst, process_template(text, cfg, cmake_style))
  log("  wrote " + dst)
  return

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def print_help():
  print("Usage: python configure_libxml2.py [options]")
  print("")
  print("Generates config.h and include/libxml/xmlversion.h from the")
  print("libxml2 *.in templates, mirroring the logic of configure.ac /")
  print("CMakeLists.txt without invoking autoconf or cmake.")
  print("")
  print("Paths:")
  print("  --source DIR           libxml2 source root (default: cwd)")
  print("  --config-h PATH        explicit path for config.h")
  print("                         (default: <script-dir>/config.h)")
  print("  --xmlversion-h PATH    explicit path for xmlversion.h")
  print("                         (default: <script-dir>/xmlversion.h)")
  print("")
  print("Build options:")
  print("  --target NAME          windows | linux | mac (default: host)")
  print("  --with-FEATURE 0|1     toggle any LIBXML2_WITH_* feature")
  print("                         e.g. --with-zlib 1 --with-iconv 1")
  print("  --have-NAME 0|1        set any HAVE_* probe value")
  print("                         e.g. --have-dlopen 1")
  print("  --xml-sysconfdir DIR   override XML_SYSCONFDIR")
  print("  --xml-thread-local S   override TLS specifier")
  print("  --module-extension S   override dynamic-module suffix")
  print("  --version-extra S      set LIBXML_VERSION_EXTRA")
  print("")
  print("  -q, --quiet            suppress status output (errors still shown)")
  print("  -h, --help             show this message")
  return

def parse_args(argv):
  args = {}
  i = 1
  while i < len(argv):
    arg = argv[i]
    if arg == "-h" or arg == "--help":
      args["help"] = "1"
      i += 1
      continue
    if arg == "-q" or arg == "--quiet":
      args["quiet"] = "1"
      i += 1
      continue
    if not arg.startswith("--"):
      base.print_error("unexpected positional argument: " + arg)
      sys.exit(1)
    key = arg[2:]
    if i + 1 >= len(argv):
      base.print_error("missing value for --" + key)
      sys.exit(1)
    args[key] = argv[i + 1]
    i += 2
  return args

def apply_overrides(cfg, args):
  reserved = ("source", "target", "config-h", "xmlversion-h",
              "version-extra", "help", "quiet")
  for key in args:
    if key in reserved:
      continue
    cfg_key = key.upper().replace("-", "_")
    value = args[key]
    if cfg_key not in cfg:
      base.print_error("unknown option: --" + key + "  "
                       "(maps to " + cfg_key + ")")
      sys.exit(1)
    # keep type stable: numeric keys stay numeric
    if isinstance(cfg[cfg_key], int):
      try:
        cfg[cfg_key] = int(value)
      except ValueError:
        base.print_error("--" + key + " expects 0 or 1, got: " + value)
        sys.exit(1)
    else:
      cfg[cfg_key] = value
  return

def main(argv):
  global QUIET
  args = parse_args(argv)
  if "help" in args:
    print_help()
    return 0
  QUIET = ("quiet" in args)

  source     = args.get("source", os.getcwd())
  script_dir = os.path.dirname(os.path.realpath(__file__))
  target     = args.get("target", base.host_platform())

  cfg = build_defaults(target)

  version_file = source + "/VERSION"
  if not base.is_file(version_file):
    base.print_error("VERSION file not found in: " + source)
    return 1
  cfg["VERSION"] = base.readFile(version_file).strip()

  apply_overrides(cfg, args)
  if "version-extra" in args:
    cfg["LIBXML_VERSION_EXTRA"] = args["version-extra"]

  derive(cfg)

  config_h_out     = args.get("config-h",     script_dir + "/config.h")
  xmlversion_h_out = args.get("xmlversion-h", script_dir + "/xmlversion.h")

  log_info("libxml2 " + cfg["VERSION"] + "  target=" + target)
  log("  source = " + source)

  configure_file(source + "/config.h.cmake.in",
                 config_h_out, cfg, True)
  configure_file(source + "/include/libxml/xmlversion.h.in",
                 xmlversion_h_out, cfg, False)
  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv))
