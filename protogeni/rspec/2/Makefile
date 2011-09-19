#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2011 University of Utah and the Flux Group.
# All rights reserved.
#

ifdef TRANG
  TRANGCMD = java -jar ${TRANG}
else
  TRANGCMD = java -jar /usr/local/share/java/classes/trang.jar
endif

SCHEMAS = ad.xsd request.xsd manifest.xsd

all: $(SCHEMAS)

clean:
	rm $(SCHEMAS)

%.xsd: %.rnc common.rnc
	mkdir -p tmp
	$(TRANGCMD) $< tmp/$@
	sed "s/any-extension.xsd/any-extension-schema.xsd/" tmp/common.xsd > $(@:.xsd=)-common.xsd
	rm tmp/common.xsd
	sed "s/common.xsd/$(@:.xsd=)-common.xsd/" tmp/$@ > $@
	rm tmp/$@
	sed "s/any-extension.xsd/any-extension-schema.xsd/" $@ > tmp/$@
	mv tmp/$@ $@
	if [ $< = manifest.rnc ]; then \
		sed "s/common.xsd/manifest-common.xsd/" tmp/request.xsd > manifest-request.xsd; \
		rm tmp/request.xsd; \
		sed "s/request.xsd/manifest-request.xsd/" manifest.xsd > manifest.tmp; \
		mv manifest.tmp manifest.xsd; \
	fi
	rm tmp/any-extension.xsd
	rmdir tmp

.PHONY: clean
