#!/bin/sh

bin/difference ideal.log candidate.log

sort -n ideal.log.absolute
bin/add-x < ideal.log.absolute > absolute.plot

sort -n ideal.log.relative
bin/add-x < ideal.log.relative > relative.plot


