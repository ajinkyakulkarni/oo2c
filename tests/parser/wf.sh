#!/bin/sh

arg="$1"

count=0
err_count=0
if test "$arg" = "--compare"; then
  for i in wf/*.Mod; do
    rm -f test-output/$i
    if $TEST_PROG $i >test-output/$i; then
      if $DIFF "`dirname $i`/out-xml/`basename $i`" test-output/$i; then
	echo "ok: $i"
      else
	err_count=`expr $err_count + 1`
	echo "failed: $i"
      fi
    else
      err_count=`expr $err_count + 1`
      echo "failed: $i (exit code is non-zero)"
    fi
  done
elif test "$arg" = "--stupid-pp"; then
  for i in wf/*.Mod; do
    rm -f test-output/$i
    if $TEST_PROG --stupid-pp $i >test-output/$i.pp1; then
      if $TEST_PROG --stupid-pp test-output/$i.pp1 >test-output/$i.pp2; then
	if $DIFF test-output/$i.pp1 test-output/$i.pp2; then
	  echo "ok: $i"
	else
	  err_count=`expr $err_count + 1`
	  echo "failed: test-output/$i.pp1"
	fi
      else
	err_count=`expr $err_count + 1`
	echo "failed: test-output/$i.pp1 (exit code is non-zero)"
      fi
    else
      err_count=`expr $err_count + 1`
      echo "failed: $i (exit code is non-zero)"
    fi
  done
fi

test $err_count = "0"
exit $?
