
CFLAGS	:= -I../include
CFLAGS	+= -I../../../deps/bctoolbox-0.6.0/OUTPUT/include
CFLAGS	+= -Wall


LDFLAGS	:= -L../lib -lortp
LDFLAGS	+= -L../../../deps/bctoolbox-0.6.0/OUTPUT/lib -lbctoolbox
LDFLAGS	+= -L../../../deps/mbedtls/OUTPUT/lib -lmbedtls -lmbedcrypto -lmbedx509
LDFLAGS	+= -L. -lbundle
LDFLAGS += -lavcodec -lavformat -lavutil -lswscale



all : bundle
	gcc rtpsend.c $(CFLAGS) $(LDFLAGS) -o rtpsend
	gcc extr_video.c $(CFLAGS) $(LDFLAGS) -o extr_video


bundle:
	gcc -fPIC -shared rtpbundle.cc -o libbundle.so $(CFLAGS)

.PHONY: clean

clean:
	@rm rtpsend
	@rm extr_video
	@rm libbundle.so
	
