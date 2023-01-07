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
        A1.*)
            test_args=(-pipewidth 1)
            trace_name="${test_name#A1.}"
            ;;
        A2.*)
            test_args=(-pipewidth 2)
            trace_name="${test_name#A2.}"
            ;;
        A3.*)
            test_args=(-pipewidth 2 -enablememfwd -enableexefwd)
            trace_name="${test_name#A3.}"
            ;;
        B1.*)
            test_args=(-pipewidth 2 -enablememfwd -enableexefwd -bpredpolicy 1)
            trace_name="${test_name#B1.}"
            ;;
        B2.*)
            test_args=(-pipewidth 2 -enablememfwd -enableexefwd -bpredpolicy 2)
            trace_name="${test_name#B2.}"
            ;;
        *) continue ;;
    esac

    total_tests=$((total_tests + 1))
    echo -n 'Running test '"$test_name"'...'

    results="../results/$test_name.res"
    ../src/sim "${test_args[@]}" "../traces/$trace_name.ptr.gz" > "$results"

    filtered_results="$(mktemp)"
    grep '^LAB2_\(NUM_INST\|NUM_CYCLES\|CPI\|BPRED_BRANCHES\|BPRED_MISPRED\|MISPRED_RATE\)' "$results" > "$filtered_results"

    if diff -q "$filtered_results" "$reference_results" > /dev/null; then
        echo " $green"'passed'"$reset"
        passed_tests=$((passed_tests + 1))
    else
        echo " $red"'failed'"$reset"
        echo "  $blue"'Reference results:'"$reset"
        sed 's/^/    /' "$reference_results"
        echo "  $blue"'Your results:'"$reset"
        sed 's/^/    /' "$filtered_results"
    fi

    rm -f "$filtered_results"
done

echo "$blue"'Passed '"$passed_tests"'/'"$total_tests"' tests'"$reset"
