// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_FNHELP_H
#define __EBPF_SWITCH_FNHELP_H

#define BPF_FNTYPE(N) bpf_ ## N ## _ft

#define BPF_FNAS(N, A) static BPF_FNTYPE(N) *const A
#define BPF_FN(N) BPF_FNAS(N, N)

#define BPF_FNDEFNAS(N, A) BPF_FNAS(N, A) = (BPF_FNTYPE(N) *) BPF_FUNC_ ## N
#define BPF_FNDEFN(N) BPF_FNDEFNAS(N, N)

#endif
