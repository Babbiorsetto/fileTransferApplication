SRCDIR := src
BINDIR := bin
CC := gcc

all: $(BINDIR)/client $(BINDIR)/server

$(BINDIR)/client: $(SRCDIR)/client.c | $(BINDIR)
	$(CC) -o $@ $^

$(BINDIR)/server: $(SRCDIR)/server.c | $(BINDIR)
	$(CC) -o $@ $^

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm $(BINDIR)/*
	rmdir $(BINDIR)
