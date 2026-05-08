#---------------------------------------------------------------------------------
# Clanbomber - Nintendo Switch Homebrew Port
# Based on the devkitPro switch-examples SDL2 Makefile template.
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile
#---------------------------------------------------------------------------------
TARGET		:=	clanbomber
BUILD		:=	build
SOURCES		:=	src upstream/src
DATA		:=	data
INCLUDES	:=	src upstream/src
ROMFS		:=	romfs

#---------------------------------------------------------------------------------
# .nro Application metadata (visible in hbmenu)
#---------------------------------------------------------------------------------
APP_TITLE	:=	ClanBomber
APP_AUTHOR	:=	ClanBomber Team / Switch port
APP_VERSION	:=	2.3.0-switch

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

DEFINES	:=	-D__SWITCH__ \
			-DCB_DATADIR='"romfs:"' \
			-DCB_LOCALEDIR='"romfs:/locale"' \
			-DPACKAGE='"clanbomber2"' \
			-DENABLE_NLS=0

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -std=gnu++17

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order matters)
#---------------------------------------------------------------------------------
LIBS	:= -lSDL2_ttf -lfreetype -lharfbuzz -lbz2 \
			-lSDL2_image -lpng -ljpeg -lwebp \
			-lSDL2_mixer -lvorbisidec -lmodplug -lmpg123 -lopusfile -lopus -lFLAC -logg \
			-lSDL2 -lEGL -lglapi -ldrm_nouveau \
			-lz -lm -lstdc++ -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linker if any C++ source files are included
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all assets clean-assets

#---------------------------------------------------------------------------------
all: assets $(BUILD)

$(BUILD): assets
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# Asset bundling: copy fonts/maps/pics/wavs from upstream/src into romfs/
#---------------------------------------------------------------------------------
assets: $(ROMFS)/.assets-stamp

$(ROMFS)/.assets-stamp: $(wildcard upstream/src/fonts/*.ttf) \
                       $(wildcard upstream/src/maps/*.map) \
                       $(wildcard upstream/src/pics/*.png) \
                       $(wildcard upstream/src/wavs/*)
	@mkdir -p $(ROMFS)/fonts $(ROMFS)/maps $(ROMFS)/pics $(ROMFS)/wavs
	@cp -u upstream/src/fonts/*.ttf $(ROMFS)/fonts/ 2>/dev/null || true
	@cp -u upstream/src/maps/*.map  $(ROMFS)/maps/  2>/dev/null || true
	@cp -u upstream/src/pics/*.png  $(ROMFS)/pics/  2>/dev/null || true
	@cp -u upstream/src/wavs/*.wav  $(ROMFS)/wavs/  2>/dev/null || true
	@cp -u upstream/src/wavs/*.mod  $(ROMFS)/wavs/  2>/dev/null || true
	@touch $@
	@echo "Assets staged in $(ROMFS)/"

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

clean-assets:
	@rm -rf $(ROMFS)

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
