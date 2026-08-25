#!/bin/bash

# set -x

SEARCH_ROOT="$1"
PARSER_BIN="$2"
EXIT_CODE=0

if [[ ! -e "$SEARCH_ROOT" ]] || [[ ! -f "$PARSER_BIN" ]]
then
  echo "USAGE: $0 <search_root> <parser_bin>"
  echo "    search_root - where to start looking for lib files"
  echo "    parser_bin - path to parser binary"
  exit 1
fi

LIB_FILES="$(find "$SEARCH_ROOT" -name '*.lib*' -type f | grep -Ei "asap7|sky130|nangate45")"

total=0
failed=0

for file in $LIB_FILES
do
  total="$((total+1))"
  if file "$file" | grep -q "ASCII"
  then
    if ! "$PARSER_BIN" --input_path "$file" > /dev/null
    then
      echo "Parsing \"$file\" Failed"
      failed="$((failed+1))"
      EXIT_CODE=1
    fi
  elif file "$file" | grep -q "gzip"
  then
    uncompressed_file="$(mktemp)"
    gzip -dc "$file" > "$uncompressed_file"
    if ! "$PARSER_BIN" --input_path "$uncompressed_file" > /dev/null
    then
      failed="$((failed+1))"
      echo "Parsing \"$file\" Failed"
      EXIT_CODE=1
    fi
    rm -f "$uncompressed_file"
  elif file "$file" | grep -qi "7-zip"
  then
    uncompressed_file="$(mktemp)"
    7z e -so "$file" > "$uncompressed_file"
    if ! "$PARSER_BIN" --input_path "$uncompressed_file" > /dev/null
    then
      failed="$((failed+1))"
      echo "Parsing \"$file\" Failed"
      EXIT_CODE=1
    fi
    rm -f "$uncompressed_file"
  else
    echo "unknown file type: $file"
    EXIT_CODE=1
  fi
done

echo "Success rate: [$((total-failed))/$total]"
exit "$EXIT_CODE"
