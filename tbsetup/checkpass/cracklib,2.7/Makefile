###
# This program is copyright Alec Muffett 1993. The author disclaims all
# responsibility or liability with respect to it's usage or its effect
# upon hardware or computer systems, and maintains copyright as set out
# in the "LICENCE" document which accompanies distributions of Crack v4.0
# and upwards.
###

###
# set this to the absolute path (less extn) of compressed dict.

DICTPATH="/usr/local/lib/pw_dict"

###
# Set this to the path of one or more files continaing wordlists.

SRCDICTS=/usr/share/dict/words /usr/share/dict/propernames

###
# If you have installed the cracklib-dicts directory, use this
#SRCDICTS=/usr/dict/words cracklib-dicts/*

default:
	@echo "you evidently don't know what you're doing. go read the README"

all:
	gmake -C cracklib
	gmake -C util DICTPATH=$(DICTPATH) 

clean:
	gmake -C cracklib clean 
	gmake -C util clean 
	-rm -f all installed Part* *.BAK *.bak *~

install: all
	@echo 'if "sort" dies from lack of space, see "util/mkdict"'
	util/mkdict $(SRCDICTS) | util/packer $(DICTPATH)
	touch installed
###	@echo 'now go install passwd/passwd where you want it'
