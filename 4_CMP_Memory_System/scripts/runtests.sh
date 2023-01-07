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
        A.*)
            test_args=(-mode 1 "../traces/${test_name#A.}.mtr.gz")
            ;;
        B.S1MB.*)
            test_args=(-mode 2 -L2sizeKB 1024 "../traces/${test_name#B.S1MB.}.mtr.gz")
            ;;
        C.S1MB.OP.*)
            test_args=(-mode 3 -L2sizeKB 1024 -dram_policy 0 "../traces/${test_name#C.S1MB.OP.}.mtr.gz")
            ;;
        C.S1MB.CP.*)
            test_args=(-mode 3 -L2sizeKB 1024 -dram_policy 1 "../traces/${test_name#C.S1MB.CP.}.mtr.gz")
            ;;
        D.mix1)
            test_args=(-mode 4 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz)
            ;;
        E.Q1.mix1)
            test_args=(-mode 4 -L2repl 2 -SWP_core0ways 4 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz)
            ;;
        E.Q2.mix1)
            test_args=(-mode 4 -L2repl 2 -SWP_core0ways 8 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz)
            ;;
        E.Q3.mix1)
            test_args=(-mode 4 -L2repl 2 -SWP_core0ways 12 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz)
            ;;
        *) continue ;;
    esac

    total_tests=$((total_tests + 1))
    echo -n 'Running test '"$test_name"'...'

    results="$(mktemp)"
    ../src/sim "${test_args[@]}" | grep '^\(CYCLES\|CORE_\|MEMSYS_\|ICACHE_\|DCACHE_\|L2CACHE_\|DRAM_\)' > "$results"

    if diff -q "$results" "$reference_results" > /dev/null; then
        echo " $green"'passed'"$reset"
        passed_tests=$((passed_tests + 1))
    else
        echo " $red"'failed'"$reset"
        echo "  $blue"'Reference results:'"$reset"
        sed 's/^/    /' <(comm -13 <(sort "$results") <(sort "$reference_results"))
        echo "  $blue"'Your results:'"$reset"
        sed 's/^/    /' <(comm -23 <(sort "$results") <(sort "$reference_results"))
    fi

    rm -f "$results"
done

echo "$blue"'Passed '"$passed_tests"'/'"$total_tests"' tests'"$reset"
