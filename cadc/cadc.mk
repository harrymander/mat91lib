CADC_DIR = $(MAT91LIB_DIR)/cadc

VPATH += $(CADC_DIR)
INCLUDES += -I$(CADC_DIR)

PERIPHERALS += adc tc pdc
SRC += cadc.c
