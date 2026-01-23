#*******************************************************************************
#************************keySTREAM Trusted Agent ("KTA")************************
#
# (c) 2023-2025 Nagravision SÃ rl
#
# Subject to your compliance with these terms, you may use the Nagravision SÃ rl
# Software and any derivatives exclusively with Nagravision's products. It is your
# responsibility to comply with third party license terms applicable to your
# use of third party software (including open source software) that may accompany
# Nagravision Software.
#
# Redistribution of this Nagravision Software in source or binary form is allowed
# and must include the above terms of use and the following disclaimer with the
# distribution and accompanying materials.
#
# THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
# IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
# NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
# EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
# OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
# THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
# POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
# NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
# SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
# TO NAGRAVISION FOR THIS SOFTWARE.
#*******************************************************************************/
#** \brief  Makefile for KTA
#*
#*  \author Kudelski IoT
#*
#*  \date 2025/01/22
#*
#*  \file Makefile
#******************************************************************************/

# Specify here the compiler of the device toolchain
CC = gcc
AR = ar
EXE_NAME   = KTAlib.a


INCLUDE_DIR := ./
INCLUDE_DIR += ./SOURCE/include
INCLUDE_DIR+=./SOURCE/kta/common/crypto/include
INCLUDE_DIR+=./SOURCE/kta/common/general/include
INCLUDE_DIR+=./SOURCE/kta/common/icpp_parser/include
INCLUDE_DIR+=./SOURCE/kta/common/version/include
INCLUDE_DIR+=./SOURCE/kta/modules/acthandler/include
INCLUDE_DIR+=./SOURCE/kta/modules/cmdhandler/include
INCLUDE_DIR+=./SOURCE/kta/modules/config/include
INCLUDE_DIR+=./SOURCE/kta/modules/reghandler/include
INCLUDE_DIR += ./COMMON/logmodule/include
INCLUDE_DIR += ./SOURCE/salapi/include
INCLUDE_DIR += ./COMMSTACK/http/include
INCLUDE_DIR+=./SOURCE/kta/modules/fotaservice/include
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/config/default/library/kta_lib/SOURCE/include
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/config/default/library/kta_lib/COMMON/logmodule/include
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/config/default/library/cryptoauthlib
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/config/default/library/cryptoauthlib/atcacert
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/config/default
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/packs/ATSAMD21E18A_DFP
INCLUDE_DIR += ./../../apps/keystream_late_provisioning_app/src/packs/CMSIS/CMSIS/Core/Include
_INCLUDES = $(foreach item, $(INCLUDE_DIR), -I$(item))

SOURCES:=./ktaFieldMgntHook.c
SOURCES+=./SOURCE/kta/common/crypto/crypto.c
SOURCES+=./SOURCE/kta/common/general/general.c
SOURCES+=./SOURCE/kta/common/icpp_parser/icpp_parser.c
SOURCES+=./SOURCE/kta/common/version/version.c
SOURCES+=./SOURCE/kta/ktamgr/ktamgr.c
SOURCES+=./SOURCE/kta/modules/acthandler/acthandler.c
SOURCES+=./SOURCE/kta/modules/cmdhandler/cmdhandler.c
SOURCES+=./SOURCE/kta/modules/config/config.c
SOURCES+=./SOURCE/kta/modules/reghandler/reghandler.c
SOURCES+=./SOURCE/salapi/k_sal_crypto.c
SOURCES+=./SOURCE/salapi/k_sal_fota.c
SOURCES+=./SOURCE/salapi/k_sal_log.c
SOURCES+=./SOURCE/salapi/k_sal_object.c
SOURCES+=./SOURCE/salapi/k_sal_storage.c
SOURCES+=./SOURCE/salapi/k_sal_fotastorage.c
SOURCES+=./COMMON/logmodule/KTALog.c
SOURCES+=./COMMSTACK/http/comm_if.c
SOURCES+=./COMMSTACK/http/http.c
SOURCES+=./COMMSTACK/http/k_sal_com.c
SOURCES+=./COMMSTACK/http/k_sal_os.c
SOURCES+=./cust_def_device.c
SOURCES+=./cust_def_signer.c
SOURCES+=./SOURCE/kta/modules/fotaservice/fotaplatform.c
SOURCES+=./SOURCE/kta/modules/fotaservice/fotaagent.c
SOURCES+=./SOURCE/kta/modules/fotaservice/fotaprocess.c


OBJECTS=$(SOURCES:.c=.o)

$(EXE_NAME): $(OBJECTS)
	$(AR) -r $@ $^

.PHONY: all
all: $(OBJECTS) $(EXE_NAME)

.c.o:
	$(CC) -c $(_INCLUDES) $< -o $@


.PHONY: clean
clean:
	rm $(OBJECTS) $(EXE_NAME)

# Makefile
