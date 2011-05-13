CC = g++
AR = ar
CFLAGS = -c -Wall -fPIC -DLINUX
LDFLAGS = 
SOURCES = synthesizer.cc Serial.cc
OBJECTS = $(SOURCES:.cc=.o)
PLATFORM = LINUX
STARGET = libValonSynth.a
DTARGET = libValonSynth.so

all: $(SOURCES) $(STARGET) $(DTARGET)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

$(STARGET): $(OBJECTS)
	$(AR) rcs $@ $^

$(DTARGET): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ -o $@

clean:
	rm -rf $(OBJECTS)

clobber: clean
	rm -rf $(STARGET) $(DTARGET)