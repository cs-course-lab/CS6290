#!/bin/bash -e

cd "$(dirname "$0")"

red="$(tput setaf 1)"
green="$(tput setaf 2)"
blue="$(tput setaf 4)"
reset="$(tput sgr0)"

if [[ ! -x '../src/sim' ]]; then
    echo "$red"'sim binary not found. Please compile first using `make`'"$reset" >&2
    exit 1
fi

total_tests=0
passed_tests=0

for reference_results in ../ref/results/*.res; do
    test_name="$(basename "$reference_results" .res)"
    case "$test_name" in
        B1.*)
            test_args=(-pipewidth 1 -schedpolicy 0 -loadlatency 1)
            trace_name="${test_name#B1.}"
            ;;
        B2.*)
            test_args=(-pipewidth 1 -schedpolicy 1 -loadlatency 1)
            trace_name="${test_name#B2.}"
            ;;
        B3.*)
            test_args=(-pipewidth 1 -schedpolicy 0 -loadlatency 4)
            trace_name="${test_name#B3.}"
            ;;
        B4.*)
            test_args=(-pipewidth 1 -schedpolicy 1 -loadlatency 4)
            trace_name="${test_name#B4.}"
            ;;
        C1.*)
            test_args=(-pipewidth 2 -schedpolicy 0 -loadlatency 1)
            trace_name="${test_name#C1.}"
            ;;
        C2.*)
            test_args=(-pipewidth 2 -schedpolicy 1 -loadlatency 1)
            trace_name="${test_name#C2.}"
            ;;
        C3.*)
            test_args=(-pipewidth 2 -schedpolicy 0 -loadlatency 4)
            trace_name="${test_name#C3.}"
            ;;
        C4.*)
            test_args=(-pipewidth 2 -schedpolicy 1 -loadlatency 4)
            trace_name="${test_name#C4.}"
            ;;
        *) continue ;;
    esac

    total_tests=$((total_tests + 1))
    echo -n 'Running test '"$test_name"'...'

    results="$(mktemp)"
    ../src/sim "${test_args[@]}" "../traces/$trace_name.ptr.gz" | grep '^LAB3_\(NUM_INST\|NUM_CYCLES\|CPI\)' > "$results"

    if diff -q "$results" "$reference_results" > /dev/null; then
        echo " $green"'passed'"$reset"
        passed_tests=$((passed_tests + 1))
    else
        echo " $red"'failed'"$reset"
        echo "  $blue"'Reference results:'"$reset"
        sed 's/^/    /' "$reference_results"
        echo "  $blue"'Your results:'"$reset"
        sed 's/^/    /' "$results"
    fi

    rm -f "$results"
done

echo "$blue"'Passed '"$passed_tests"'/'"$total_tests"' tests'"$reset"
