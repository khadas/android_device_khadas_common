#----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# Copyright (C) 2023. Arm Limited or its affiliates. All rights reserved.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#---------------------------------------------------------------------------

THIS_DIR := $(strip $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))

# TARGET_PRODUCT and PLATFORM_VERSION are available and can be used
# to select an sepolicy directory if required.

BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy

BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-icompression

ifeq ($(PLATFORM_VERSION), 13)
    BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-v13
endif

ifeq ($(PLATFORM_VERSION), 14)
    BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-v14
endif

ifeq ($(PLATFORM_VERSION), UpsideDownCake)
    BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-v14
endif



ifeq (1, 1)
    BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-firmware
endif

ifeq (0, 1)
    BOARD_VENDOR_SEPOLICY_DIRS += $(THIS_DIR)/public/sepolicy-module-params
endif

