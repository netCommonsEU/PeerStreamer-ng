#!/bin/bash

URI="http://127.0.0.1:3000/"

info() {
	echo -e "\E[33m$@\033[0m"
}

error() {
	echo -e "\E[31m$@\033[0m"
}

success() {
	echo -e "\E[32m$@\033[0m"
}

assert_equal() {
    # assert_equal <line> <val1> <val2>
	line=$1
    val1=$2
	val2="$3"
    if [[ "$val1" == "$val2" ]]; then
        echo -n .
        return
	else
		error "\nLine $1: Failure, \"$val1\" mismatches \"$val2\""
		exit 1
	fi
}

assert_not_equal() {
    # assert_not_equal <line> <val1> <val2>
	line=$1
    val1=$2
	val2="$3"
    if [[ "$val1" != "$val2" ]]; then
        echo -n .
        return
	else
		error "\nLine $1: Failure, \"$val1\" matches \"$val2\""
		exit 1
	fi
}

function test_url() {
    # test_url <METHOD> <URL> <DATA>
	METHOD=$1
	URL=$2
	if [ -z $3 ]; then
		curl -X $METHOD $URI/$URL 2>/dev/null
	else
		DATA="$3"
		curl -X $METHOD --data "$DATA" $URI/$URL 2>/dev/null
	fi
	return $?
}

which curl > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
	error "Executable curl not found"
	exit 1
fi

res=$(test_url GET )
if [[ $? -ne 0 ]]; then
	error "PeerStreamer-ng is not responding at $URI.\nIs it running?"
	exit 1
fi

## Try the GET
res=$(test_url GET channels)
assert_equal $LINENO "$res" "[]"

res=$(test_url GET sources)
assert_equal $LINENO "$res" "[]"

res=$(test_url GET nonexisting)
assert_equal $LINENO "$res" "Not Found"

## Source creation
res=$(test_url POST sources key=value)
assert_equal $LINENO "$res" "Not Found"

res=$(test_url UPDATE sources)
assert_equal $LINENO "$res" "Not Found"

res=$(test_url UPDATE sources/mych)
assert_equal $LINENO "$res" "Not Found"

res=$(test_url POST sources/mych key=value)
assert_equal $LINENO "$res" "{\"id\":\"mych\",\"source_ip\":\"127.0.0.1\",\"source_port\":\"0\",\"janus_streaming_id\":\"0\"}"

res=$(test_url UPDATE sources/mych)
assert_equal $LINENO "$res" "{\"id\":\"mych\",\"source_ip\":\"127.0.0.1\",\"source_port\":\"0\",\"janus_streaming_id\":\"0\"}"

res=$(test_url GET sources)
assert_equal $LINENO "$res" "[{\"id\":\"mych\",\"source_ip\":\"127.0.0.1\",\"source_port\":\"0\",\"janus_streaming_id\":\"0\"}]"

res=$(test_url POST sources/mych key=value)
assert_equal $LINENO "$res" "Conflict"

## Channels 
res=$(test_url GET channels)
assert_equal $LINENO "$res" "[]"

res=$(test_url UPDATE sources/mych "channel_name=ciao&participant_id=3")
assert_equal $LINENO "$res" "{\"id\":\"mych\",\"source_ip\":\"127.0.0.1\",\"source_port\":\"0\",\"janus_streaming_id\":\"3\"}"

res=$(test_url GET channels)
assert_not_equal $LINENO "$res" "[]"

## Channel playout
ipaddr=$(echo $res | cut -d '"' -f 8)
port=$(echo $res | cut -d '"' -f 12)
res=$(test_url POST channels key=value)
assert_equal $LINENO "$res" "Not Found"

res=$(test_url POST channels/prova "ipaddr=$ipaddr&port=$port")
assert_not_equal $LINENO "$res" "Bad Request"
res=$(test_url POST channels/prova2 "ipaddr=$ipaddr&port=$port")
assert_not_equal $LINENO "$res" "Bad Request"

res=$(test_url POST channels/prova "ipaddr=$ipaddr&port=$port")
assert_equal $LINENO "$res" "Conflict"

res=$(test_url UPDATE channels/random)
assert_equal $LINENO "$res" "Not Found"

res=$(test_url UPDATE channels/prova)
assert_not_equal $LINENO "$res" "Not Found"

success "\nTest run successfully"
