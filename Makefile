app := c2tzx-ng
obj := c2tzx-ng.o

CFLAGS := --std=gnu99 -Wall -Werror
LDFLAGS :=

$(app): $(obj)
	$(CC) $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(app)
	install -m 755 $< /usr/local/bin/

clean:
	rm -f $(obj)
