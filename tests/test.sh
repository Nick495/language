#!/bin/sh

test_string()
{
	STRING="$1"
	echo "==> Testing $STRING"
	TOKENS_OUTPUT=$(echo $STRING | ./print_tokens)
	echo "$TOKENS_OUTPUT"
	PARSE_OUTPUT=$(echo $STRING | ./parse)
	echo "$PARSE_OUTPUT"
	echo ""
}

test_string "12345"
test_string "( 1 + 2 + 3 + 4 )"
test_string "2 + 4"
test_string "1 + 2 + 3 + 4"
