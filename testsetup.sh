#!/bin/bash -eu
root="target/jail"

[ -d $root ] && rm -rf $root

username=$(whoami)

mkdir $root
mkdir $root/tmp
mkdir $root/home

copy_libraries() {
  ldd $1/* | awk '{print $1}' | grep -v ':' | sort | uniq | xargs which | xargs -I{} cp {} $1
}

userhome=$root/home/$username/
# cp before creating $userhome, will not create subdir 
cp -r test/fixture $userhome

mkdir $root/bin
# used in tests
cp $(which bash) $(which true) $(which sh) $(which dash) $(which uname) \
	$(which ls) $(which strace) $(which ln) $root/bin
cp target/origin $root/bin
chmod +x $root/bin/*
# all dynamic libraries
copy_libraries $root/bin

test_dir=$root/opt/test
mkdir -p $test_dir/target

(cd $root/bin; ln -s true sant)
