#!/usr/bin/env bash

source=$(find -regex '.*\.[ch]')

assembly_source=$(find -iname '*.S')

extended_source=$(find -iname '*.py')


# remove trailing whitespaces from non-uncrustifyable source
echo $assembly_source $extended_source Makefile |  xargs sed -i 's/[[:space:]]*$//'

# run uncrustify on source
uncrustify --replace --no-backup -c uncrustify.cfg $source