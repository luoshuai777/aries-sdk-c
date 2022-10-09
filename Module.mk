# I2C tools for Linux
#
# Copyright (C) 2007  Jean Delvare <khali@linux-fr.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

ARIES_DIR		:= aries-sdk-c
ARIES_SRC		:= $(ARIES_DIR)/source
ARIES_EXAMPLES		:= $(ARIES_DIR)/examples
ARIES_EXAMPLES_SRC	:= $(ARIES_DIR)/examples/source

# C Flags
# Set warnings and gdb
# add include directories
ARIES_CFLAGS := -Wstrict-prototypes -Wpointer-arith -Wcast-qual \
		-Wcast-align -Wwrite-strings -Wnested-externs -Winline -W -Wundef \
		-Wmissing-prototypes -I./aries-sdk-c/include \
		-I./aries-sdk-c/examples/include -I./include


# By default, create executables for these directories
ARIES_TARGETS	:= aries_test eeprom_update eeprom_test link_example link_test margin_test prbs

# Libraries to include
#    -lpigpio pigpio.h library for RPI
#    -lrt pigpio.h library for RPI
RPI_LDFLAGS := -lpigpio -lrt

# Libraries to include
#    -lm math.h library
ARIES_LDFLAGS := -lm

################################
########### Programs ###########
################################

$(ARIES_EXAMPLES)/aries_test: $(ARIES_EXAMPLES)/aries_test.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/eeprom_update: $(ARIES_EXAMPLES)/eeprom_update.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/eeprom_test: $(ARIES_EXAMPLES)/eeprom_test.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o \
	$(ARIES_EXAMPLES_SRC)/eeprom.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/link_example: $(ARIES_EXAMPLES)/link_example.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/link_test: $(ARIES_EXAMPLES)/link_test.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/margin_test: $(ARIES_EXAMPLES)/margin_test.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/aries_margin.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

$(ARIES_EXAMPLES)/prbs: $(ARIES_EXAMPLES)/prbs.o \
	$(ARIES_EXAMPLES_SRC)/aspeed.o \
	$(ARIES_SRC)/aries_api.o \
	$(ARIES_SRC)/aries_i2c.o \
	$(ARIES_SRC)/aries_link.o \
	$(ARIES_SRC)/aries_misc.o \
	$(ARIES_SRC)/astera_log.o
	$(CC) $(LDFLAGS) $(ARIES_LDFLAGS) -o $@ $^

###############################
########### Objects ###########
###############################

$(ARIES_EXAMPLES)/aries_test.o: $(ARIES_EXAMPLES)/aries_test.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/eeprom_update.o: $(ARIES_EXAMPLES)/eeprom_update.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/eeprom_test.o: $(ARIES_EXAMPLES)/eeprom_test.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/link_example.o: $(ARIES_EXAMPLES)/link_example.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/link_test.o: $(ARIES_EXAMPLES)/link_test.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/margin_test.o: $(ARIES_EXAMPLES)/margin_test.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES)/prbs.o: $(ARIES_EXAMPLES)/prbs.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/aries_api.o: $(ARIES_SRC)/aries_api.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/aries_i2c.o: $(ARIES_SRC)/aries_i2c.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/aries_link.o: $(ARIES_SRC)/aries_link.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/aries_misc.o: $(ARIES_SRC)/aries_misc.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/aries_margin.o: $(ARIES_SRC)/aries_margin.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_SRC)/astera_log.o: $(ARIES_SRC)/astera_log.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES_SRC)/aspeed.o: $(ARIES_EXAMPLES_SRC)/aspeed.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

$(ARIES_EXAMPLES_SRC)/eeprom.o: $(ARIES_EXAMPLES_SRC)/eeprom.c
	$(CC) $(CFLAGS) $(ARIES_CFLAGS) -c $< -o $@

################################
########### Commands ###########
################################

all-tools: $(addprefix $(ARIES_EXAMPLES)/,$(ARIES_TARGETS))

strip-tools: $(addprefix $(ARIES_DIR)/,$(ARIES_TARGETS))
	strip $(addprefix $(ARIES_DIR)/,$(ARIES_TARGETS))

clean-tools:
	$(RM) $(addprefix $(ARIES_DIR)/,*.o) \
		$(addprefix $(ARIES_EXAMPLES)/,$(ARIES_TARGETS)) \
		$(addprefix $(ARIES_EXAMPLES)/,*.o) \
		$(addprefix $(ARIES_EXAMPLES_SRC)/,*.o) \
		$(addprefix $(ARIES_SRC)/,*.o)

install-tools: $(addprefix $(ARIES_DIR)/,$(ARIES_TARGETS))
	$(INSTALL_DIR) $(DESTDIR)$(sbindir) $(DESTDIR)$(man8dir)
	for program in $(ARIES_TARGETS) ; do \
	$(INSTALL_PROGRAM) $(ARIES_DIR)/$$program $(DESTDIR)$(sbindir) ; \
	$(INSTALL_DATA) $(ARIES_DIR)/$$program.8 $(DESTDIR)$(man8dir) ; done

uninstall-tools:
	for program in $(ARIES_TARGETS) ; do \
	$(RM) $(DESTDIR)$(sbindir)/$$program ; \
	$(RM) $(DESTDIR)$(man8dir)/$$program.8 ; done

all: all-tools

strip: strip-tools

clean: clean-tools

install: install-tools

uninstall: uninstall-tools
