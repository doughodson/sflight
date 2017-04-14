
include makedefs

# Library         : Requires
# ------------------------------------------------------------------------
# xml             :
# sf              : -
#
PROJECTS = src/mdls
PROJECTS += src/xml
PROJECTS += src/xml_bindings

# Examples        : Requires
# ------------------------------------------------------------------------
# mainTest        : libsflight_mdls, libsflight_xml, libsflight_xml_bindings
#
PROJECTS += examples/mainTest

.PHONY: all clean $(PROJECTS)

#
# Rules
#
all: $(PROJECTS)

$(PROJECTS):
	$(MAKE) -C $@

clean:
	-for d in $(PROJECTS); do (cd $$d; $(MAKE) clean ); done

