#!/bin/sh -
#	$Id$
#
# This script lists messages with identical text but different ids, or
# messages with the same id but different texts.
#
# If the first argument equals "id", list messages with the same id but
# different texts. Otherwise, list messages with the same text but
# different ids.

# NOTE: Please update the MSG_DIRS if there is any new source dir.
MSG_DIRS="../src/ ../util/ ../lang/dbm/"

AWK=${AWK:-awk}

if [ -e msg_id_pairs ] ; then
	rm -rf msg_id_pairs msg_id_pairs_sorted
fi

if [ "$1" == "id" ] ; then
	id_first="1"
else
	id_first="0"
fi

for f in `find $MSG_DIRS -name "*.[ch]"` ; do
	$AWK -f ./extract_msg.awk -v id_first="$id_first" $f >> msg_id_pairs
done

sort < msg_id_pairs > msg_id_pairs_sorted

$AWK -F "------" -v lastline="" -v lastkey="" -v first="1" \
    '{
    	if (lastkey == $1 && lastline != $0 && lastkey != "3675") {
    		if (first == 1) {
    			print lastline;
    			first = 0;
    		}
    		print;
    	} else {
    		first = 1;
    	}
    	lastline = $0;
    	lastkey = $1;
    }' \
    msg_id_pairs_sorted
