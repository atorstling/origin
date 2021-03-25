#!/bin/bash -eu

cp2() {
  # copy with path, keep special files
  sudo rsync --super -DR $*
}

copy_libraries() {
  # rsync LDR = keep parent directory structure, copy special files, turn symlinks into real files
  sudo rsync -LR $(find $1 -type f -perm +u=x,g=x,o=x | xargs otool -L | awk '{print $1}' | grep -v ':' | xargs otool -L | awk '{print $1}' | grep -v ':' | xargs otool -L | awk '{print $1}' | grep -v ':' | xargs otool -L | awk '{print $1}' | grep -v ':' | sort | uniq) $1
}

root="target/jail"

# SCARY
[ -d $root ] && sudo rm -rf $root

username=$(whoami)

mkdir $root
mkdir $root/tmp
mkdir $root/Users

userhome=$root/Users/$username/
# cp before creating $userhome, will not create subdir 
cp -r test/fixture $userhome

# used in tests
cp2 $(which bash) $(which true) $(which sh) $(which dash) $(which uname) \
  $(which ls) $(which strace) $(which ln) $(which which) /usr/bin/true $root
# utils
cp2 $(which cat) $(which whoami) $root

# binary + symlink
sudo cp target/origin $root/bin
(cd $root/bin; sudo ln -s /usr/bin/true sant)

# special files
cp2 /dev/null /dev/urandom /dev/zero /usr/lib/dyld $root

# copy all dynamic libraries
copy_libraries $root $root

# test dir
test_dir=$root/opt/test
mkdir -p $test_dir/target

