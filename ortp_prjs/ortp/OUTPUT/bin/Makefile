
CFLAGS	:= -I../include
CFLAGS	+= -I../../../deps/bctoolbox-0.6.0/OUTPUT/include
CFLAGS	+= -Wall

##error This file requires compiler and library support for the ISO C++ 2011 standard. This support must be enabled with the -std=c++11 or -std=gnu++11 compiler options.
#CFLAGS  += -std=c++11

##libortp.so: undefined reference to `logf@GLIBC_2.27'
##libortp.so: undefined reference to `expf@GLIBC_2.27'
#rebuild libortp.so

LDFLAGS	:= -L../lib -lortp
LDFLAGS	+= -L../../../deps/bctoolbox-0.6.0/OUTPUT/lib -lbctoolbox
LDFLAGS	+= -L../../../deps/mbedtls/OUTPUT/lib -lmbedtls -lmbedcrypto -lmbedx509
LDFLAGS	+= -L. -lbundle
LDFLAGS += -lavcodec -lavformat -lavutil -lswscale



all : bundle
	gcc rtpsend.c $(CFLAGS) $(LDFLAGS) -o rtpsend
	gcc extr_video.c $(CFLAGS) $(LDFLAGS) -o extr_video


bundle:
	gcc -fPIC -shared rtpbundle.cc -o libbundle.so $(CFLAGS) -std=c++11

.PHONY: clean

clean:
	@rm rtpsend
	@rm extr_video
	@rm libbundle.so
	
