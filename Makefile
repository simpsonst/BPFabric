all::

PREFIX=/usr/local
MKDIR=mkdir -p
PRINTF=printf
FIND=find
CP=cp
CMP=cmp
TOUCH=touch
ZIP=zip
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
headers += bpfabric/dummy/openssl/opensslconf.h
headers += bpfmap.h
headers += ebpf_consts.h
headers += ebpf_crypt.h
headers += ebpf_digest.h
headers += ebpf_fndecl.h
headers += ebpf_fnhelp.h
headers += ebpf_fntypes.h
headers += ebpf_functions.h
headers += ebpf_switch.h

python2.7_zips += controller
python2.7_zips += mininet
python2.7_zips += tools
python2.7_zips += protocol

protocol_pyroot=$(BINODEPS_TMPDIR)/python2.7/protobuf

controller_pyname=bpfabric/controller
mininet_pyname=bpfabric/mininet
tools_pyname=bpfabric/tools
protocol_pyname=bpfabric/protocol

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

$(BINODEPS_OUTDIR)/python2.7/protocol.zip: | \
		$(PROTOCOL:%=$(BINODEPS_TMPDIR)/%.proto-py)

$(BINODEPS_TMPDIR)/%.proto-py: $(BINODEPS_PROTODIR)/%.proto
	@$(PRINTF) '[protobuf Python] %s\n' '$*'
	@$(MKDIR) '$(@D)' '$(dir $(BINODEPS_TMPDIR)/python2.7/protobuf/$*)'
	@$(PROTOC.py) '-I$(BINODEPS_PROTODIR)' \
	  --python_out='$(abspath $(BINODEPS_TMPDIR)/python2.7/protobuf)' \
	 '$<'
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

install:: install-binaries install-libraries install-python-zips install-headers

all:: installed-binaries
all:: installed-libraries
all:: python-zips

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

clean::
	$(MAKE) -C dpdkswitch clean

install-binaries@default::
	$(INSTALL) -m 0755 $(BINODEPS_OUTDIR)/dpdkswitch $(BINDIR@default)
	$(INSTALL) -m 0644 $(BINODEPS_OUTDIR)/dpdkswitch.map $(BINDIR@default)
endif

PYTHON_VERSIONS += 2.7
PYTHON_VERSIONS += 3
PYTHON_VERSIONS += 3.5

define PYTHON_DEFS
PYTHON$1 ?= python$1

endef
$(foreach V,$(PYTHON_VERSIONS),$(eval $(call PYTHON_DEFS,$V)))

define PYTHON_TREE_DEFS
$1_pyroot ?= $1
$1_pyname ?= $1
$1_py$2root ?= $($1_pyroot)
$1_py$2name ?= $($1_pyname)

endef
$(foreach V,$(PYTHON_VERSIONS),$(foreach T,$(python$V_zips),$(eval $(call PYTHON_TREE_DEFS,$T,$V))))


define PYTHON_LIB_DEFS
PYTHON$2_LIBDIR ?= $(LIBDIR)/python$2/site-packages
python$2_files-$1=$$(shell $$(FIND) '$1' -name "*.py" -printf '%P\n')
python$2_dsfiles-$1=$$(python$2_files-$1:%=./%)
python$2_subdirs-$1=$$(sort $$(dir $$(python$2_dsfiles-$1)))

endef
$(foreach V,$(PYTHON_VERSIONS),$(foreach D,$(python$V_libs),$(eval $(call PYTHON_LIB_DEFS,$D,$V))))

define PYTHON_SUBDIR_CMDS
$(call PRINTLIST,'  %s: ','$(1:%/=%)/$(2:./%=%)','$(foreach F,$(filter-out $(foreach SD,$(filter-out $2,$(filter $2%,$(python$3_subdirs-$1))),$(SD)%),$(filter $2%,$(python$3_dsfiles-$1))),$(F:$2%=%))')
$(INSTALL) -d '$(PYTHON$3_LIBDIR)/$(2:./%=%)'
$(INSTALL) -m 0644 $(foreach F,$(filter-out $(foreach SD,$(filter-out $2,$(filter $2%,$(python$3_subdirs-$1))),$(SD)%),$(filter $2%,$(python$3_dsfiles-$1))),'$(1:%/=%)/$(F:./%=%)') '$(PYTHON$3_LIBDIR)/$(2:./%=%)'

endef

define PYTHON_CMDS
$(foreach SD,$(python$2_subdirs-$1),$(call PYTHON_SUBDIR_CMDS,$1,$(SD),$2))

endef

install-python:: install-python-modules install-python-zips

define PYTHON_ZIP_CMDS
$(INSTALL) -d '$(dir $(SHAREDIR)/python$2/$($1_pyname))'
$(INSTALL) -m 0644 '$(BINODEPS_OUTDIR)/python$2/$1.zip' '$(SHAREDIR)/python$2/$($1_pyname).zip'

endef

define PYTHON_TREE_DEPS
$$(BINODEPS_OUTDIR)/python$2/$1.zip: $$($1_pyroot)

endef

define PYTHON_VDEPS
install-python:: install-python$1
install-python$1:: install-python$1-modules
install-python$1:: install-python$1-zips

install-python-modules:: install-python$1-modules

install-python$1-modules::
	@$$(PRINTF) 'Installing Python %s libraries in [%s]:\n' '$1' \
	  '$$(PYTHON$1_LIBDIR)'
	@$$(foreach D,$$(python$1_libs),$$(call PYTHON_CMDS,$$D,$1))

install-python-zips:: install-python$1-zips

install-python$1-zips::
	@$$(PRINTF) 'Installing Python %s apps in %s:\n' '$1' \
	  '$$(SHAREDIR)/python$1'
	@$$(PRINTF) '  %s\n' $$(python$1_zips:%='%')
	@$$(foreach Z,$$(python$1_zips),$$(call PYTHON_ZIP_CMDS,$$Z,$1))

python-zips:: python-zips$1

python-zips$1:: $$(foreach T,$$(python$1_zips),$$(BINODEPS_OUTDIR)/python$1/$$T.zip)

-include $$(python$1_zips:%=$$(BINODEPS_TMPDIR)/python$1/zips/%.mk)

$$(foreach T,$$(python$1_zips),$$(eval $$(call PYTHON_TREE_DEPS,$$T,$1)))

$$(BINODEPS_OUTDIR)/python$1/%.zip:
	@$$(PRINTF) '[Python %s ZIP] %s from %s\n' '$1' '$$*' '$$($$*_pyroot)'
	@$$(MKDIR) '$$(dir $$(BINODEPS_TMPDIR)/python$1/src/$$*)'
	@$$(RM) -r '$$(BINODEPS_TMPDIR)/python$1/src/$$*'
	@$$(CP) --reflink=auto -r '$$($$*_pyroot)' \
	  '$$(BINODEPS_TMPDIR)/python$1/src/$$*'
	@$$(PYTHON$1) -m compileall '$$(BINODEPS_TMPDIR)/python$1/src/$$*'
	@$$(MKDIR) '$$(dir $$(BINODEPS_TMPDIR)/python$1/zips/$$*.zip)'
	@$$(RM) '$$(abspath $$(BINODEPS_TMPDIR)/python$1/zips/$$*.zip)'
	@$$(CD) '$$(BINODEPS_TMPDIR)/python$1/src/$$*' ; \
	  $$(ZIP) -qr '$$(abspath $$(BINODEPS_TMPDIR)/python$1/zips/$$*.zip)' . \
	    -i '*.py' '*.pyc'
	@$$(FIND) '$$(BINODEPS_TMPDIR)/python$1/src/$$*' -mindepth 1 \
	  \( -name "*.py" -o -type d \) \
	  -printf '$$$$(BINODEPS_OUTDIR)/python$1/$$*.zip: $$($$*_pyroot)/%P\n' \
	  > '$$(BINODEPS_TMPDIR)/python$1/zips/$$*.mk-tmp'
	@$$(MV) '$$(BINODEPS_TMPDIR)/python$1/zips/$$*.mk-tmp' \
	  '$$(BINODEPS_TMPDIR)/python$1/zips/$$*.mk'
	@$$(MKDIR) '$$(@D)'
	@$$(MV) '$$(BINODEPS_TMPDIR)/python$1/zips/$$*.zip' '$$@'

endef

$(foreach V,$(PYTHON_VERSIONS),$(eval $(call PYTHON_VDEPS,$V)))

blank::
	$(RM) -r dpdkswitch/build
