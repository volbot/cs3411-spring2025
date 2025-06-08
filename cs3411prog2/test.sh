#!/bin/sh

RESULTS="";
UTAR="$PWD/utar"
CTAR="$PWD/ctar"
SAMPCTAR="$PWD/prog2-sample-arch.ctar"

entertests() {
    if [ -d testing ]; then
        rm -r testing
    fi
    mkdir testing
    cd testing
}
exittests() {
    cd ../
    rm -r testing
}
clean() {
    rm -f 0?-* gurp* tarred* appended*
}
prep() {
    CMD=( "${UTAR}" "${SAMPCTAR}" )
    "${CMD[@]}"
    echo goooop > gurpo
    echo goooooop > gurp2
}
assert() {
    if [ "$1" = "0" ]; then
        RESULTS="${RESULTS} 1"
    else
        RESULTS="${RESULTS} 0"
    fi
}
runtest() {
    entertests;
    for TESTCASE in "$@"; do
        mkdir "test-${TESTCASE}"
        cd "test-${TESTCASE}"
        prep
        case "$TESTCASE" in
            1) # 3 file ctar and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurpo 05-cat.png 01-textfile.txt )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?
                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt)$?
                ;;
            2) # 4 file ctar and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf)$?
                ;;
            3) # 5 file ctar and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf -a -e gurp2)$?
                ;;
            4) # 50 file ctar and utar
                TEST2=( test )
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" )
                for i in {0..49}
                do
                    echo "this is file number ${i}" > "tarred${i}"
                    CMD+=( "tarred${i}" )
                    TEST2+=( -e "tarred${i}" )
                    if [ "$i" != "49" ]; then
                        TEST2+=( -a )
                    fi
                done
                "${CMD[@]}"

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $( "${TEST2[@]}" )$?
                ;;
            5) # 3 file ctar, 1 append, and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurpo 05-cat.png 01-textfile.txt )
                "${CMD[@]}"
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" 04-audio.m4a )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 04-audio.m4a)$?
                ;;
            6) # 4 file ctar, 1 append, and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf )
                "${CMD[@]}"
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" 04-audio.m4a )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf -a -e 04-audio.m4a)$?
                ;;
            7) # 5 file ctar, 1 append, and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf )
                "${CMD[@]}"
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" 04-audio.m4a )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf -a -e gurp2 -a -e 04-audio.m4a)$?
                ;;

            8) # 50 file ctar, 50 file append, and utar
                TEST2=( test )
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" )
                for i in {0..49}
                do
                    echo "this is file number ${i}" > "tarred${i}"
                    CMD+=( "tarred${i}" )
                    TEST2+=( -e "tarred${i}" -a )
                done
                "${CMD[@]}"

                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" )
                for i in {50..99}
                do
                    echo "this is file number ${i}" > "appended$((${i} - 50))"
                    CMD+=( "appended$((${i} - 50))" )
                    TEST2+=( -e "appended$((${i} - 50))" )
                    if [ "$i" != "99" ]; then
                        TEST2+=( -a )
                    fi
                done
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean input
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $( "${TEST2[@]}" )$?
                ;;
            9) # empty ctar, 6 file append, and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf 04-audio.m4a )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean input
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf -a -e gurp2 -a -e 04-audio.m4a)$?
                ;;

            10) # delete file in first block and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf 04-audio.m4a )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 05-cat.png )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean input
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a ! -e 05-cat.png -a -e 01-textfile.txt -a -e 02-programspec.pdf -a -e gurp2 -a -e 04-audio.m4a)$?
                ;;

            11) # delete file in second block and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf 04-audio.m4a )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 02-programspec.pdf )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean input
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test -e gurpo -a -e 05-cat.png -a -e 01-textfile.txt -a ! -e 02-programspec.pdf -a -e gurp2 -a -e 04-audio.m4a)$?
                ;;

            12) # delete all files and utar
                CMD=( "${CTAR}" -a "test-${TESTCASE}.ctar" gurp2 gurpo 05-cat.png 01-textfile.txt 02-programspec.pdf 04-audio.m4a )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" gurp2 )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" gurpo )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 05-cat.png )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 01-textfile.txt )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 02-programspec.pdf )
                "${CMD[@]}"
                CMD=( "${CTAR}" -d "test-${TESTCASE}.ctar" 04-audio.m4a )
                "${CMD[@]}"
                assert $(test -e test-${TESTCASE}.ctar)$?

                clean input
                CMD=( "${UTAR}" "test-${TESTCASE}.ctar" )
                "${CMD[@]}"
                assert $(test ! -e gurpo -a ! -e 05-cat.png -a ! -e 01-textfile.txt -a ! -e 02-programspec.pdf -a ! -e gurp2 -a ! -e 04-audio.m4a)$?
                ;;

            esac
            cd ../
        done
        # exittests
    }

    runtest 1 2 3 4 5 6 7 8 9 10 11 12

    echo "Each test (out of 12) has two internal assertions: .CTAR created, and proper files exist upon UTAR."
    echo "A 1 is a passed test; a 0 is a failed test."
    echo "NOTE: The internal tests are not sufficient to verify complete functionality. Please review each test case as needed."
    echo "$RESULTS"
