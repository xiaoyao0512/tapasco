CFLAGS+=-Wall -Werror -I$(TAPASCO_HOME)/tlkm/user
LDFLAGS+=-lc

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

tlkm_ioctl_test:	tlkm_ioctl_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	@rm -f *.o tlkm_ioctl_test
