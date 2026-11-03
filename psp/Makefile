TARGET = psp_car_dash
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspdebug -lpspdisplay -lpspge -lpspctrl \
       -lpspnet -lpspnet_inet -lpspnet_apctl -lpsputility -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP CAR DASH v0.3

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
