
PLUGIN_DIR = bgp_security \
    data_center \
    decision_process \
    extended_communities \
    geo_tags \
    hello_world \
    monitoring \
    rib_walk \
    route_reflector


.PHONY: all build clean

all: build

build:
	@for a in $(PLUGIN_DIR); do \
		if [ -d $$a ]; then \
			echo "processing folder $$a"; \
			$(MAKE) -C $$a; \
		fi; \
	done;

clean:
	@for a in $(PLUGIN_DIR); do \
		if [ -d $$a ]; then \
			echo "processing folder $$a"; \
			$(MAKE) clean -C $$a; \
		fi; \
	done;
