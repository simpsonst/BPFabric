all::

PREFIX=/usr/local
MKDIR=mkdir -p
PRINTF=printf
FIND=find
CP=cp
CMP=cmp
TOUCH=touch
PROTOC.c=protoc-c
PROTOC.py=protoc

SUBMODELS += bpf
SUBMODEL_OBJSFX@bpf=.bo
SUBMODEL_SFX@bpf=.o
SUBMODEL_LABEL@bpf=BPF
CPPFLAGS@bpf += -target bpf
CC@bpf=clang

BINDIR@bpf=$(LIBDIR)/bpfabric

-include BPFabric-env.mk

EXAMPLES += ewma
EXAMPLES += interarrival
EXAMPLES += learningswitch
EXAMPLES += learningswitch_centralized
EXAMPLES += trafficcount
EXAMPLES += traffichist

PROTOCOL += Header
PROTOCOL += Hello
PROTOCOL += Install
PROTOCOL += Notify
PROTOCOL += Packet
PROTOCOL += Table

UBPF += ubpf_vm
UBPF += ubpf_jit_x86_64
UBPF += ubpf_loader

BPFMAP += bpfmap
BPFMAP += arraymap
BPFMAP += hashtab
BPFMAP += libghthash/hash_functions
BPFMAP += libghthash/hash_table

libraries += ubpf
ubpf_mod += $(UBPF)

libraries += bpfmap
bpfmap_mod += $(BPFMAP)

binaries.c += softswitch
softswitch_obj += $(PROTOCOL:%=%.pb-c)
softswitch_obj += softswitch
softswitch_obj += agent
softswitch_obj += $(UBPF)
softswitch_obj += $(BPFMAP)
softswitch_lib += -lpthread
softswitch_lib += -lprotobuf-c
softswitch_lib += -lssl
softswitch_lib += -lcrypto
softswitch_lib += -luECC

ifneq ($(RTE_SDK),)
binaries.c += dpdkswitch
RTE_TARGET ?= x86_64-native-linuxapp-gcc
#include $(RTE_SDK)/mk/rte.vars.mk
LDFLAGS@default += -L$(RTE_SDK)/$(RTE_TARGET)/lib
CFLAGS@default += -I$(RTE_SDK)/$(RTE_TARGET)/include
endif

dpdkswitch_obj += $(PROTOCOL:%=%.pb-c)
dpdkswitch_obj += dpdkswitch
dpdkswitch_obj += dpdkswitch-pmd
dpdkswitch_obj += agent
dpdkswitch_obj += $(UBPF)
dpdkswitch_obj += $(BPFMAP)
dpdkswitch_lib += -lprotobuf-c
dpdkswitch_lib += -lssl
dpdkswitch_lib += -lcrypto
dpdkswitch_lib += -luECC
dpdkswitch_lib += -lrte_kni
dpdkswitch_lib += -lrte_pipeline
dpdkswitch_lib += -lrte_table
dpdkswitch_lib += -lrte_port
dpdkswitch_lib += -lrte_pdump
dpdkswitch_lib += -lrte_distributor
dpdkswitch_lib += -lrte_reorder
dpdkswitch_lib += -lrte_ip_frag
dpdkswitch_lib += -lrte_meter
dpdkswitch_lib += -lrte_sched
dpdkswitch_lib += -lrte_lpm
dpdkswitch_lib += -Wl,--whole-archive
dpdkswitch_lib += -lrte_acl
dpdkswitch_lib += -Wl,--no-whole-archive
dpdkswitch_lib += -lrte_jobstats
dpdkswitch_lib += -lrte_power
dpdkswitch_lib += -Wl,--whole-archive
dpdkswitch_lib += -lrte_timer
dpdkswitch_lib += -lrte_hash
dpdkswitch_lib += -lrte_vhost
dpdkswitch_lib += -lrte_kvargs
dpdkswitch_lib += -lrte_mbuf
dpdkswitch_lib += -lethdev
dpdkswitch_lib += -lrte_cryptodev
dpdkswitch_lib += -lrte_mempool
dpdkswitch_lib += -lrte_ring
dpdkswitch_lib += -lrte_eal
dpdkswitch_lib += -lrte_cmdline
dpdkswitch_lib += -lrte_cfgfile
dpdkswitch_lib += -lrte_pmd_bond
dpdkswitch_lib += -lrte_pmd_af_packet
dpdkswitch_lib += -lrte_pmd_bnxt
dpdkswitch_lib += -lrte_pmd_cxgbe
dpdkswitch_lib += -lrte_pmd_e1000
dpdkswitch_lib += -lrte_pmd_ena
dpdkswitch_lib += -lrte_pmd_enic
dpdkswitch_lib += -lrte_pmd_fm10k
dpdkswitch_lib += -lrte_pmd_i40e
dpdkswitch_lib += -lrte_pmd_ixgbe
dpdkswitch_lib += -lrte_pmd_null
dpdkswitch_lib += -lrte_pmd_pcap
dpdkswitch_lib += -lpcap
dpdkswitch_lib += -lrte_pmd_ring
dpdkswitch_lib += -lrte_pmd_virtio
dpdkswitch_lib += -lrte_pmd_vhost
dpdkswitch_lib += -lrte_pmd_vmxnet3_uio
dpdkswitch_lib += -lrte_pmd_null_crypto
dpdkswitch_lib += -Wl,--no-whole-archive
dpdkswitch_lib += -lrt
dpdkswitch_lib += -lm
dpdkswitch_lib += -ldl
dpdkswitch_lib += -Wl,-export-dynamic
dpdkswitch_lib += -Wl,-export-dynamic
dpdkswitch_lib += -pthread
dpdkswitch_lib += -Wl,--as-needed
dpdkswitch_lib += -Wl,-Map=$(BINODEPS_OUTDIR)/dpdkswitch.map
dpdkswitch_lib += -Wl,--cref

headers += ubpf.h
headers += bpfmap.h
headers += ebpf_consts.h
headers += ebpf_crypt.h
headers += ebpf_digest.h
headers += ebpf_fndecl.h
headers += ebpf_fnhelp.h
headers += ebpf_fntypes.h
headers += ebpf_functions.h
headers += ebpf_switch.h


binaries.c@bpf += $(EXAMPLES)
define BPF_DEFS
$1_obj += $1

endef
$(foreach M,$(EXAMPLES),$(eval $(call BPF_DEFS,$M)))

include binodeps.mk

$(BINOBJS.softswitch@default): $(PROTOCOL:%=$(BINODEPS_SRCDIR_DYN)/%.pb-c.h)

$(BINOBJS.dpdkswitch@default): $(PROTOCOL:%=$(BINODEPS_SRCDIR_DYN)/%.pb-c.h)

LINK.c@bpf=./fake-link-bpf

tidy::
	$(FIND) . -name "*~" -delete

$(BINODEPS_SRCDIR_DYN)/%-pmd.c: $(BINODEPS_SRCDIR_DYN)/%.o
	@$(PRINTF) '[DPDK pmdinfogen] %s\n' '$*'
	@if egrep -q 'PMD_REGISTER_DRIVER(.*)' $(BINODEPS_SRCDIR_DYN)/$*.c ; \
	then \
	  $(RTE_SDK)/$(RTE_TARGET)/app/dpdk-pmdinfogen '$<' '$@' ; \
	else \
	  touch '$@' ; \
	fi

$(BINODEPS_SRCDIR_DYN)/dpdkswitch.o: CFLAGS@default += \
	-include $(RTE_SDK)/$(RTE_TARGET)/include/rte_config.h \
	-m64 -pthread -march=native \
	-DRTE_MACHINE_CPUFLAG_SSE \
	-DRTE_MACHINE_CPUFLAG_SSE2 \
	-DRTE_MACHINE_CPUFLAG_SSE3 \
	-DRTE_MACHINE_CPUFLAG_SSSE3 \
	-DRTE_MACHINE_CPUFLAG_SSE4_1 \
	-DRTE_MACHINE_CPUFLAG_SSE4_2 \
	-DRTE_MACHINE_CPUFLAG_AES \
	-DRTE_MACHINE_CPUFLAG_PCLMULQDQ \
	-DRTE_MACHINE_CPUFLAG_AVX \


$(BINODEPS_TMPDIR)/%.proto-py: $(BINODEPS_SRCDIR)/%.proto
	@$(PRINTF) '[protoc Python] %s\n' '$*'
	@$(MKDIR) '$(@D)' '$(dir $(BINODEPS_OUTDIR)/$*)'
	@($(CD) $(BINODEPS_SRCDIR) ; \
	  $(PROTOC.py) --python_out='$(abspath $(BINODEPS_OUTDIR))' \
	    '$*.proto')
	@$(TOUCH) '$@'

$(BINODEPS_TMPDIR)/%.proto-c: $(BINODEPS_SRCDIR)/%.proto
	@$(PRINTF) '[protoc C] %s\n' '$*'
	@$(MKDIR) '$(@D)' '$(dir $(BINODEPS_TMPDIR)/protobuf/$*)'
	@($(CD) $(BINODEPS_SRCDIR) ; \
	  $(PROTOC.c) --c_out='$(abspath $(BINODEPS_TMPDIR)/protobuf)' '$*.proto')
	@$(TOUCH) '$@'

$(BINODEPS_SRCDIR_DYN)/%.pb-c.c: $(BINODEPS_TMPDIR)/%.proto-c
	@$(CMP) -s '$(BINODEPS_TMPDIR)/protobuf/$*.pb-c.c' '$@' || \
	  ($(CP) '$(BINODEPS_TMPDIR)/protobuf/$*.pb-c.c' '$@' && \
	   $(PRINTF) '[protoc C cp] %s.c\n' '$*')
$(BINODEPS_SRCDIR_DYN)/%.pb-c.h: $(BINODEPS_TMPDIR)/%.proto-c
	@$(CMP) -s '$(BINODEPS_TMPDIR)/protobuf/$*.pb-c.h' '$@' || \
	  ($(CP) '$(BINODEPS_TMPDIR)/protobuf/$*.pb-c.h' '$@' && \
	   $(PRINTF) '[protoc C cp] %s.h\n' '$*')

install-binaries@default::
	$(INSTALL) -m 0644 $(BINODEPS_OUTDIR)/dpdkswitch.map $(BINDIR@default)

all:: installed-binaries
all:: installed-libraries
