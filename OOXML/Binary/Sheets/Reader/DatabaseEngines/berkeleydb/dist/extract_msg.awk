#
# Copyright (c) 2016, 2020 Oracle and/or its affiliates.  All rights reserved.
#
# See the file LICENSE for license information.
#
# $Id$
#

# This script extracts error messages and their IDs from C input files.
# Error messages with IDs start with the DB_STR() or DB_STR_A() macro.
# This script looks for DB_STR() and DB_STR_A() macros, and print the
# message-id pairs.

# Arguments:
#	id_first	if id is printed before message

function extract_string(text) {
	str = "";
	b = index(text, "\"") + 1;
	if (b == 1) {
		return "";
	}
	e = b
	while (e < length(text)) {
		c = substr(text, e, 1);
		if (c == "\"") {
			break;
		} else if (c == "\\") {
			e = e + 2;
		} else {
			e = e + 1;
		}
	}
	return substr(text, b, e - b);
}

/DB_STR\(|DB_STR_A\(/, /\);$/ {
	i = match($0, "DB_STR\\(|DB_STR_A\\(");
	firstline = i != 0 ? 1 : 0;
	if (firstline == 1) {
		finished = 0;
		msg = ""
		id_idx = substr($0, i + 6, 1) == "\(" ? i + 8 : i + 10;
		id = substr($0, id_idx, 4);
		$0 = substr($0, id_idx + 6, length($0) - id_idx - 5);
	}
	if (finished == 0) {
		partial = extract_string($0);
		if (length(partial) != 0) {
			msg = sprintf("%s%s", msg, partial);
			pe = index($0, partial) + length(partial);
			if (pe < length($0)) {
				if (id_first == 1) {
					print id "------" msg;
				} else {
					print msg "------" id;
				}
				finished = 1;
			}
		}
	}
} 