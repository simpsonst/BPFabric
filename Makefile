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

BINODEPS_PROTODIR=src/protobuf

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

test_libraries += agent
agent_mod += agent

test_libraries += protocol
protocol_mod += $(PROTOCOL:%=%.pb-c)

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

python_dirs += controller
python_dirs += $(BINODEPS_OUTDIR)/python2.7

binaries.c@bpf += $(EXAMPLES)
define BPF_DEFS
$1_obj += $1

endef
$(foreach M,$(EXAMPLES),$(eval $(call BPF_DEFS,$M)))

include binodeps.mk

$(BINOBJS.softswitch@default): $(PROTOCOL:%=$(BINODEPS_SRCDIR_DYN)/%.pb-c.h)

LINK.c@bpf=./fake-link-bpf

tidy::
	$(FIND) . -name "*~" -delete


$(BINODEPS_TMPDIR)/%.proto-py: $(BINODEPS_PROTODIR)/%.proto
	@$(PRINTF) '[protobuf Python] %s\n' '$*'
	@$(MKDIR) '$(@D)' '$(dir $(BINODEPS_OUTDIR)/python2.7/$*)'
	@$(PROTOC.py) '-I$(BINODEPS_PROTODIR)' \
	  --python_out='$(abspath $(BINODEPS_OUTDIR)/python2.7)' '$<'
	@$(TOUCH) '$@'

$(BINODEPS_TMPDIR)/%.proto-c: $(BINODEPS_PROTODIR)/%.proto
	@$(PRINTF) '[protobuf C] %s\n' '$*'
	@$(MKDIR) '$(@D)' '$(dir $(BINODEPS_TMPDIR)/protobuf/$*)'
	@$(PROTOC.c) '-I$(BINODEPS_PROTODIR)' \
	  --c_out='$(abspath $(BINODEPS_TMPDIR)/protobuf)' '$<'
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
	$(INSTALL) -m 0755 $(BINODEPS_OUTDIR)/dpdkswitch $(BINDIR@default)
	$(INSTALL) -m 0644 $(BINODEPS_OUTDIR)/dpdkswitch.map $(BINDIR@default)

install:: install-binaries install-libraries install-python

all:: installed-binaries
all:: installed-libraries

installed-protobuf:: $(PROTOCOL:%=$(BINODEPS_TMPDIR)/%.proto-py)
all:: installed-protobuf

ifneq ($(RTE_SDK),)
installed-binaries:: $(BINODEPS_OUTDIR)/dpdkswitch
$(BINODEPS_OUTDIR)/dpdkswitch: $(PROTOCOL:%=$(BINODEPS_SRCDIR_DYN)/%.pb-c.h) \
		$(BINODEPS_OUTDIR)/libagent.a \
		$(BINODEPS_OUTDIR)/libubpf.a \
		$(BINODEPS_OUTDIR)/libbpfmap.a \
		$(BINODEPS_OUTDIR)/libprotocol.a \
		dpdkswitch/main.c
	$(MAKE) -C dpdkswitch
	$(CP) dpdkswitch/build/app/dpdkswitch \
	  dpdkswitch/build/app/dpdkswitch.map $(BINODEPS_OUTDIR)
endif

PYTHON_LIBDIR ?= $(LIBDIR)/python2.7/site-packages

define PYTHON_DEFS
python_files-$1=$$(shell $$(FIND) '$1' -name "*.py" -printf '%P\n')
python_dsfiles-$1=$$(python_files-$1:%=./%)
python_subdirs-$1=$$(sort $$(dir $$(python_dsfiles-$1)))

endef

$(foreach D,$(python_dirs),$(eval $(call PYTHON_DEFS,$D)))

define PYTHON_SUBDIR_CMDS
$(call PRINTLIST,'  %s: ','$(1:%/=%)/$(2:./%=%)','$(foreach F,$(filter-out $(foreach SD,$(filter-out $2,$(filter $2%,$(python_subdirs-$1))),$(SD)%),$(filter $2%,$(python_dsfiles-$1))),$(F:$2%=%))')
$(INSTALL) -d '$(PYTHON_LIBDIR)/$(2:./%=%)'
$(INSTALL) -m 0644 $(foreach F,$(filter-out $(foreach SD,$(filter-out $2,$(filter $2%,$(python_subdirs-$1))),$(SD)%),$(filter $2%,$(python_dsfiles-$1))),'$(1:%/=%)/$(F:./%=%)') '$(PYTHON_LIBDIR)/$(2:./%=%)'

endef

define PYTHON_CMDS
$(foreach SD,$(python_subdirs-$1),$(call PYTHON_SUBDIR_CMDS,$1,$(SD)))

endef

install-python::
	@printf 'Installing python in [%s]:\n' '$(PYTHON_LIBDIR)'
	@$(foreach D,$(python_dirs),$(call PYTHON_CMDS,$D))

clean::
	$(MAKE) -C dpdkswitch clean

blank::
	$(RM) -r dpdkswitch/build
