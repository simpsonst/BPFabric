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



test_binaries.c@bpf += $(EXAMPLES)
define BPF_DEFS
$1_obj += $1

endef
$(foreach M,$(EXAMPLES),$(eval $(call BPF_DEFS,$M)))

include binodeps.mk

$(BINOBJS.softswitch@default): $(PROTOCOL:%=$(BINODEPS_SRCDIR_DYN)/%.pb-c.h)

LINK.c@bpf=./fake-link-bpf

tidy::
	$(FIND) . -name "*~" -delete

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

all:: $(EXAMPLES:%=$(BINODEPS_OUTDIR)/%$(SUBMODEL_SFX@bpf))
all:: installed-binaries
all:: installed-libraries
