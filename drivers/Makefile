#
# Makefile for building modules outside of the kernel source tree.
#
# Use with command:
#
# make -C <kernel source dir> M=$PWD INSTALL_MOD_DIR=<driver install dir> modules
#
ifdef RHEL_7_3_514
	ccflags-y += -DRHEL_7_3_514=1
endif

ccflags-y += -DKERNEL_BUILD_OS_$(KERNEL_BUILD_OS_ID)=1

ifdef BUILD_N_HDLC
	obj-m += n_hdlc.o
endif

ifdef BUILD_SYNCLINK_GT
	obj-m += synclink_gt.o
endif

ifdef BUILD_SYNCLINK_USB
	obj-m += synclink_usb.o
endif


