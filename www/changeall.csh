#!/bin/csh -v -x
echo "Replacing $1 with $2 in all files in 3 seconds"
sleep 3
foreach file (*.php3)
  echo "Processing $file"
  set str = "s/$1/$2/g;print;"
  perl -ne $str < $file > .my_tmp
  echo "Okay? (2 sec)"
  sleep 2
  cp .my_tmp $file
  rm .my_tmp
end
echo "Done."

