CDRECORD    = cdrecord dev=IODVDServices
SCRAMBLE    = scramble
UNMOUNT     = diskutil unmountDisk disk1
DD          = dd
CP          = cp
MKISOFS     = mkisofs
CAT         = cat
TOUCH       = touch
1ST_READ    = /usr/local/dcdev/dcload/dcload-ip/target-src/1st_read/1st_read.bin
IP_BIN      = /usr/local/dcdev/dcload/dcload-ip/make-cd/IP.BIN
DISC_SIZE   = 700
DATA_SIZE   = du -h -I ".svn" -I ".DS_Store" cd | sed -e 's/[a-z,A-Z]//g'

all: clean burn-data

1st_read.bin: $(1ST_READ)
	$(SCRAMBLE) $(1ST_READ) cd/1st_read.bin

IP.BIN: $(IP_BIN)
	$(CP) $(IP_BIN) IP.BIN

audio.raw:
	$(DD) if=/dev/zero of=audio.raw bs=2352 count=300

burn-audio: audio.raw
	$(UNMOUNT)
	$(CDRECORD) -v -speed=8 -tao -multi -audio audio.raw
	touch burn-audio

00DUMMY.DAT: 1st_read.bin IP.BIN burn-audio
	@. dummy.sh $(DISC_SIZE)
	
data.raw: 00DUMMY.DAT
	$(UNMOUNT)
	$(MKISOFS) -l -r -C `$(CDRECORD) -msinfo` -exclude-list exclude.list -G IP.BIN -o data.raw cd/
	touch burn-data

burn-data: data.raw
	$(UNMOUNT)
	$(CDRECORD) -v -speed=8 -tao -multi -xa data.raw

.PHONY : clean
clean:
	rm -f cd/1st_read.bin cd/00DUMMY.DAT IP.BIN audio.raw data.raw burn-audio burn-data
