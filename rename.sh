: ${2? "USAGE: $0 <from> <to>"}
git grep -l $1 | xargs -l sed -i "s/$1/$2/"
