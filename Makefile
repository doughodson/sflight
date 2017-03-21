
include makedefs

# Library         : Requires
# ------------------------------------------------------------------------
# xml             :
# sf              : -
#
PROJECTS = src/xml
PROJECTS += src

# Examples        : Requires
# ------------------------------------------------------------------------
# mainTest        : libsf
#
PROJECTS += mainTest

.PHONY: all clean $(PROJECTS)

#
# Rules
#
all: $(PROJECTS)

$(PROJECTS):
	$(MAKE) -C $@

clean:
	-for d in $(PROJECTS); do (cd $$d; $(MAKE) clean ); done

