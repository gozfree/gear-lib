

# PHONY += menuconfig silentoldconfig clean distclean ALL

Q            	?= @
PROJECT_ROOT	:= $(shell pwd)
# obj          := $(CURDIR)/kconfig
# SUBDIR       := kconfig
# Kconfig      := Kconfig
# SRC_DIR += gear-lib
# rm-distclean += include/generated include/config  .config

-include $(PROJECT_ROOT)/include/config/auto.conf

define fun1
lib_name=$(patsubst [a-z],[A-Z],$(1))
lib_name=$(join "CONFIG_",$(lib_name))
obj-$($(lib_name)) += $(lib_name)

endef

# ifdef $(1)
# export CFLAGS += -D$(1) -I$(shell cd $(2) && pwd)
# export LDFLAGS += -L$(shell cd $(2) && pwd) -Wl,-rpath=$(shell cd $(2) && pwd)
# endif




$(eval $(call fun1,CONFIG_LIBPOSIX,libposix))
# $(eval $(call fun1,CONFIG_LIBTHREAD,libthread))
# $(eval $(call fun1,CONFIG_LIBCOLLECTIONS,libcollections))
# $(eval $(call fun1,CONFIG_LIBDARRAY,libdarray))
# $(eval $(call fun1,CONFIG_LIBGEVENT,libgevent))
# $(eval $(call fun1,CONFIG_LIBLOG,liblog))
# $(eval $(call fun1,CONFIG_LIBPTCP,libptcp))
# $(eval $(call fun1,CONFIG_LIBQUEUE,libqueue))
# $(eval $(call fun1,CONFIG_LIBHASH,libhash))
# $(eval $(call fun1,CONFIG_LIBWORKQ,libworkq))
# $(eval $(call fun1,CONFIG_LIBRINGBUFFER,libringbuffer))
# $(eval $(call fun1,CONFIG_LIBDICT,libdict))
# $(eval $(call fun1,CONFIG_LIBFILE,libfile))
# $(eval $(call fun1,CONFIG_LIBSOCK,libsock))
# $(eval $(call fun1,CONFIG_LIBRPC,librpc))
# $(eval $(call fun1,CONFIG_LIBBITMAP,libbitmap))
# $(eval $(call fun1,CONFIG_LIBBASE64,libbase64))
# $(eval $(call fun1,CONFIG_LIBCONFIG,libconfig))
# $(eval $(call fun1,CONFIG_LIBDEBUG,libdebug))
# $(eval $(call fun1,CONFIG_LIBFSM,libfsm))
# $(eval $(call fun1,CONFIG_LIBHAL,libhal))
# $(eval $(call fun1,CONFIG_LIBHOMEKIT,libhomekit))
# $(eval $(call fun1,CONFIG_LIBIPC,libipc))
# $(eval $(call fun1,CONFIG_LIBJPEG_EX,libjpeg-ex))
# $(eval $(call fun1,CONFIG_LIBMEDIA_IO,libmedia-io))
# $(eval $(call fun1,CONFIG_LIBMP4,libmp4))
# $(eval $(call fun1,CONFIG_LIBMQTTC,libmqttc))
# $(eval $(call fun1,CONFIG_LIBP2P,libp2p))
# $(eval $(call fun1,CONFIG_LIBPLUGIN,libplugin))
# $(eval $(call fun1,CONFIG_LIBRBTREE,librbtree))
# $(eval $(call fun1,CONFIG_LIBRTMPC,librtmpc))
# $(eval $(call fun1,CONFIG_LIBRTSP,librtsp))
# $(eval $(call fun1,CONFIG_LIBSORT,libsort))
# $(eval $(call fun1,CONFIG_LIBSTREX,libstrex))
# $(eval $(call fun1,CONFIG_LIBSUBMASK,libsubmask))
# $(eval $(call fun1,CONFIG_LIBUAC,libuac))
# $(eval $(call fun1,CONFIG_LIBUVC,libuvc))
# $(eval $(call fun1,CONFIG_LIBVECTOR,libvector))
# $(eval $(call fun1,CONFIG_LIBUTF2GBK,libutf2gbk))


$(info $(CONFIG_LIBPOSIX))


ALL:
	$(Q)$(foreach  dir,$(obj-y),make -C $(dir);make -C $(dir) include;)
	$(Q)$(MAKE) -C src

menuconfig : 
	$(Q)python3 -m menuconfig $(Kconfig)
	$(Q)$(MAKE) silentoldconfig

%_defconfig:  
	$(Q)echo "$@ ..."
	$(Q)$(MAKE) -C configs $@
	$(Q)$(MAKE) menuconfig

saveconfig:
	$(Q)python3 -m savedefconfig

silentoldconfig: .config
	$(Q)mkdir -p include/config
	$(Q)cp .config include/config/auto.conf
	$(Q)awk -F "=" '{mk=substr($$1, 1, 1);if (mk != "#" && mk != "" ) printf "#define %s %s\n", $$1, $$2;}' .config > include/config/autoconfig.h
#	$(Q)awk -F "=" '{mk=substr($$1, 1, 1);if (mk != "#" && mk != "" ) if($$2 == "y") printf "set(%s \"%s\")\n", $$1, $$2;else printf "set(%s %s)\n", $$1, $$2;}' .config > include/config/auto.cmake

clean:
	$(Q)echo "nihao"

distclean:clean
	$(Q)-rm -rf  ./.config ./include/config




