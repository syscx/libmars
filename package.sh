#!/bin/sh -e

script_name=$0
build_no=

usage() {
cat <<EOF
Usage:$script_name [-b <build-number>]
EOF
}

die() {
    echo $* >&2; exit 1
}

while [ $# != 0 ]; do
    case $1 in
	-b)
	    build_no=$2
	    shift
	    ;;
        -c)
            changelist=$2
            shift
            ;;
	*)
	    die "unknown option: $1"
	    ;;
    esac
    shift
done

cd $(dirname $0)
cur_dir=$(pwd)
rks-rpmbuild ${build_no:+-b $build_no} ${changelist:+-D changelist $changelist} $cur_dir/package.spec
