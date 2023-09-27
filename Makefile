# invisreg - suite of utilities for hiding registry keys
# Copyright (C) 2023  Sabrina Andersen (NukingDragons)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CC = x86_64-w64-mingw32-gcc

CFLAGS := -O2
_CFLAGS := -Iinclude -Icustom-errno/include

SRCS = custom-errno/error.c \
	   invis/ntdll.c \
	   invis/reg.c \
	   invisreg.c

# Target based rules

.PHONY: all clean invisreg

all: invisreg

clean:
	find . \( -name "*.o" -or -name "*.exe" \) -exec rm {} \; || true

# File based rules

custom-errno/error.c:
	git submodule update --init --recursive --remote

invisreg: $(SRCS:.c=.o)
	$(CC) $(_CLFAGS) $(CFLAGS) $^ -o $@

# Glob based rules

%.o: %.c
	$(CC) $(_CFLAGS) $(CFLAGS) -c $^ -o $@
