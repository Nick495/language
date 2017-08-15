#!/bin/sh

test_string()
{
	STRING="$1"
	echo "==> Testing $STRING"
	TOKENS_OUTPUT=$(echo $STRING | ./print_tokens)
	PARSE_OUTPUT=$(echo $STRING | ./parse)

	if [ "$PARSE_OUTPUT" = "$2" ]; then
		echo "Test passed"
	else
		echo "Test failed. Expected: $2. Got: $PARSE_OUTPUT."
		echo "$TOKENS_OUTPUT"
	fi
}

test_string "12345" "12345"
test_string "2 + 4" "6"
test_string "1 + 2 + 3 + 4" "10"
test_string "( 1 + 2 + 3 + 4 )"	"10"
test_string "2 2 + 2 2" "4 4"
test_string "1 2 3 + 4 5 6" "5 7 9"
test_string "1 2 3 4 5 + 1 2 3 4" "Error: mismatched shapes."
