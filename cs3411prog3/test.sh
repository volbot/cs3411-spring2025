#!/bin/sh

RESULTS="";
ENCODE="$PWD/encode"
DECODE="$PWD/decode"
#ENCODE="$PWD/encode-s"
#DECODE="$PWD/decode-s"
SAMPDIR="$PWD/samples"

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
	#inaccurate (never used though?)
	rm -f 0?-* gurp* tarred* appended*
}
prep() {
	CMD=( cp $(echo "${SAMPDIR}/*") ./ )
	"${CMD[@]}"
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
			0) #test test case where nothing happens
				;;
			1) # encode and decode 'hello.txt', compare against sample '.encoded'
				"${ENCODE}" < hello.txt > hello.testencoded
				assert $(diff hello.encoded hello.testencoded)$?
				;;
			2) # encode and decode 'constitution.txt', compare against sample '.encoded'
				"${ENCODE}" < constitution.txt > constitution.testencoded
				assert $(diff constitution.encoded constitution.testencoded)$?
				;;
			3) # encode 'FGSA', compare against sample '.encoded'
				"${ENCODE}" < FGSA > FGSA.testencoded 
				assert $(diff FGSA.encoded FGSA.testencoded)$?
				;;
			4) # decode 'hello.encoded', compare to original
				"${DECODE}" < hello.encoded > new.hello
				assert $(diff hello.txt new.hello)$?
				;;
			5) # decode 'constitution.encoded', compare to original
				"${DECODE}" < constitution.encoded > new.constitution
				assert $(diff constitution.txt new.constitution)$?
				;;
			6) # decode 'FGSA.encoded', compare to original
				"${DECODE}" < FGSA.encoded > new.FGSA
				assert $(diff FGSA new.FGSA)$?
				;;
			7) # encode and decode 'hello.txt', compare to original
				"${ENCODE}" < hello.txt | "${DECODE}" > new.hello
				assert $(diff hello.txt new.hello)$?
				;;
			8) # encode and decode 'constitution.txt', compare to original
				"${ENCODE}" < constitution.txt | "${DECODE}" > new.constitution
				assert $(diff constitution.txt new.constitution)$?
				;;
			9) # encode and decode 'FGSA', compare to original
				"${ENCODE}" < FGSA | "${DECODE}" > new.FGSA
				assert $(diff FGSA new.FGSA)$?
				;;
			esac
			cd ../
		done
		# exittests
	}

	runtest 1 2 3 4 5 6 7 8 9

	echo "A 1 is a passed test; a 0 is a failed test."
	echo "NOTE: The internal tests are not sufficient to verify complete functionality. Please review each test case as needed."
	echo "$RESULTS"
