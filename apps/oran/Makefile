CC := gcc

CFLAGS += -Wno-packed-bitfield-compat
CFLAGS += -DSIMDE_X86_AVX_NATIVE -DSIMDE_X86_F16C_NATIVE \
          -DSIMDE_X86_FMA_NATIVE -DSIMDE_X86_GFNI_NATIVE \
	  -DSIMDE_X86_MMX_NATIVE -DSIMDE_X86_PCLMUL_NATIVE \
	  -DSIMDE_X86_SSE2_NATIVE -DSIMDE_X86_SSE3_NATIVE \
	  -DSIMDE_X86_SSE_NATIVE -DSIMDE_X86_XOP_HAVE_COM_ \
	  -DSIMDE_X86_XOP_NATIVE
CLFAGS += -DSIMDE_X86_AVX512BW_NATIVE -DSIMDE_X86_AVX512F_NATIVE \
          -DSIMDE_X86_AVX512VL_NATIVE -mavx512bw -march=skylake-avx512 \
	  -mtune=skylake-avx512
CFLAGS += -DSIMDE_X86_AVX2_NATIVE -DSIMDE_X86_VPCLMULQDQ_NATIVE
CFLAGS += -DSIMDE_X86_SSE4_1_NATIVE
CFLAGS += -DSIMDE_X86_SSE4_2_NATIVE
CFLAGS += -DSIMDE_X86_SSSE3_NATIVE
CFLAGS += -march=native
# CFLAGS += -Werror
CFLAGS += -pipe -fPIC -Wall -fno-strict-aliasing -Wno-packed-bitfield-compat \
          -rdynamic -funroll-loops

LDFLAGS += -lssl -lcrypto -lm -lsctp -lz 

# Directories declarations generated from the `set(<name>_DIR}` directives
OPENAIR_DIR             := openairinterface5g
NFAPI_DIR               := ${OPENAIR_DIR}/nfapi/open-nFAPI
NFAPI_USER_DIR          := ${OPENAIR_DIR}/nfapi/oai_integration
OPENAIR1_DIR            := ${OPENAIR_DIR}/openair1
OPENAIR2_DIR            := ${OPENAIR_DIR}/openair2
OPENAIR3_DIR            := ${OPENAIR_DIR}/openair3
OPENAIR_CMAKE           := ${OPENAIR_DIR}/cmake_targets
S1AP_DIR                := ${OPENAIR3_DIR}/S1AP
NGAP_DIR                := ${OPENAIR3_DIR}/NGAP
M2AP_DIR                := ${OPENAIR2_DIR}/M2AP
M3AP_DIR                := ${OPENAIR3_DIR}/M3AP
X2AP_DIR                := ${OPENAIR2_DIR}/X2AP
F1AP_DIR                := ${OPENAIR2_DIR}/F1AP
E1AP_DIR                := ${OPENAIR2_DIR}/E1AP
CONFIG_ROOTDIR          := ${OPENAIR_DIR}/common/config
MAC_DIR                 := ${OPENAIR2_DIR}/LAYER2/MAC
NR_GNB_MAC_DIR          := ${OPENAIR2_DIR}/LAYER2/NR_MAC_gNB
NR_UE_MAC_DIR           := ${OPENAIR2_DIR}/LAYER2/NR_MAC_UE
PHY_INTERFACE_DIR       := ${OPENAIR2_DIR}/PHY_INTERFACE
NR_PHY_INTERFACE_DIR    := ${OPENAIR2_DIR}/NR_PHY_INTERFACE
NR_UE_PHY_INTERFACE_DIR := ${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE
RRC_DIR                 := ${OPENAIR2_DIR}/RRC/LTE
NR_RRC_DIR              := ${OPENAIR2_DIR}/RRC/NR
NR_UE_RRC_DIR           := ${OPENAIR2_DIR}/RRC/NR_UE
PDCP_DIR                := ${OPENAIR2_DIR}/LAYER2/PDCP_v10.1.0
OCP_ITTI                := ${OPENAIR_DIR}/common/utils/ocp_itti
NR_RLC_DIR              := ${OPENAIR2_DIR}/LAYER2/nr_rlc

RRC_ASN1_DIR            := ${RRC_DIR}/MESSAGES/generated
NR_RRC_ASN1_DIR         := ${NR_RRC_DIR}/MESSAGES/generated
NGAP_ASN1_DIR           := ${NGAP_DIR}/MESSAGES/generated
S1AP_ASN1_DIR           := ${S1AP_DIR}/MESSAGES/generated
E1AP_ASN1_DIR           := ${OPENAIR2_DIR}/E1AP/MESSAGES/generated
F1AP_ASN1_DIR           := ${F1AP_DIR}/MESSAGES/generated
X2AP_ASN1_DIR           := ${X2AP_DIR}/MESSAGES/generated
M2AP_ASN1_DIR           := ${M2AP_DIR}/MESSAGES/generated
M3AP_ASN1_DIR           := ${M3AP_DIR}/MESSAGES/generated

# Includes generated from the `include_directories(}` directives
INCLUDES += -I ${S1AP_DIR}
INCLUDES += -I ${NGAP_DIR}
INCLUDES += -I ${X2AP_DIR}
INCLUDES += -I ${OPENAIR_DIR}/radio/COMMON
INCLUDES += -I ${OPENAIR_DIR}/executables
INCLUDES += -I ${OPENAIR2_DIR}/COMMON
INCLUDES += -I ${OPENAIR2_DIR}/UTIL
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/LOG
INCLUDES += -I ${OPENAIR3_DIR}/COMMON
INCLUDES += -I ${OPENAIR3_DIR}/UTILS
INCLUDES += -I ${NFAPI_DIR}/nfapi/public_inc
INCLUDES += -I ${NFAPI_DIR}/common/public_inc
INCLUDES += -I ${NFAPI_DIR}/pnf/public_inc
INCLUDES += -I ${NFAPI_DIR}/nfapi/inc
INCLUDES += -I ${NFAPI_DIR}/sim_common/inc
INCLUDES += -I ${NFAPI_DIR}/pnf_sim/inc
INCLUDES += -I ${OPENAIR1_DIR}
INCLUDES += -I ${OPENAIR2_DIR}
INCLUDES += -I ${OPENAIR3_DIR}/NAS/TOOLS
INCLUDES += -I ${OPENAIR2_DIR}/ENB_APP
INCLUDES += -I ${OPENAIR2_DIR}/GNB_APP
INCLUDES += -I ${OPENAIR2_DIR}/MCE_APP
INCLUDES += -I ${OPENAIR2_DIR}/LAYER2/RLC
INCLUDES += -I ${OPENAIR2_DIR}/LAYER2/PDCP_v10.1.0
INCLUDES += -I ${OPENAIR2_DIR}/RRC/LTE/MESSAGES
INCLUDES += -I ${OPENAIR2_DIR}/RRC/LTE
INCLUDES += -I ${OPENAIR_DIR}/common/utils
INCLUDES += -I ${OPENAIR_DIR}/common/utils/collection
INCLUDES += -I ${OPENAIR_DIR}/common/utils/ocp_itti
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON/API/NETWORK
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON/EMM/MSG
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON/ESM/MSG
INCLUDES += -I ${OPENAIR3_DIR}/NAS/UE/ESM
INCLUDES += -I ${OPENAIR3_DIR}/NAS/UE/EMM
INCLUDES += -I ${OPENAIR3_DIR}/NAS/UE/API/USER
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON/IES
INCLUDES += -I ${OPENAIR3_DIR}/NAS/COMMON/UTIL
INCLUDES += -I ${OPENAIR3_DIR}/SECU
INCLUDES += -I ${OPENAIR3_DIR}/SCTP
INCLUDES += -I ${OPENAIR3_DIR}/S1AP
INCLUDES += -I ${OPENAIR2_DIR}/X2AP
INCLUDES += -I ${OPENAIR2_DIR}/M2AP
INCLUDES += -I ${OPENAIR2_DIR}/F1AP
INCLUDES += -I ${OPENAIR3_DIR}/ocp-gtpu
INCLUDES += -I ${OPENAIR3_DIR}/M3AP
INCLUDES += -I ${OPENAIR3_DIR}/MME_APP
INCLUDES += -I ${OPENAIR_DIR}/radio/COMMON
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/OSA
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/MEM
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/LISTS
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/FIFO
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/MATH
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/TIMER
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/OTG
INCLUDES += -I ${OPENAIR2_DIR}/UTIL/OPT
INCLUDES += -I ${OPENAIR_DIR}
INCLUDES += -I ${OPENAIR_DIR}/common/utils/T
INCLUDES += -I ${OPENAIR_DIR}/common/utils/hashtable
INCLUDES += -I ${NFAPI_DIR}/common/public_inc
INCLUDES += -I ${NFAPI_DIR}/nfapi/public_inc
INCLUDES += -I ${NFAPI_DIR}/nfapi/inc
INCLUDES += -I ${NFAPI_DIR}/pnf/public_inc
INCLUDES += -I ${NFAPI_DIR}/pnf/inc
INCLUDES += -I ${NFAPI_DIR}/vnf/public_inc
INCLUDES += -I ${NFAPI_DIR}/vnf/inc
INCLUDES += -I ${NFAPI_USER_DIR}
INCLUDES += -I ${OPENAIR2_DIR}/NR_UE_PHY_INTERFACE
INCLUDES += -I ${OPENAIR2_DIR}/LAYER2
INCLUDES += -I ${OPENAIR1_DIR}/SCHED_NR_UE
INCLUDES += -I ${OPENAIR3_DIR}/ocp-gtp
INCLUDES += -I ${NAS_SRC}NR_UE
INCLUDES += -I ${NAS_SRC}UE
INCLUDES += -I ${NAS_SRC}UE/API/USER
INCLUDES += -I ${NAS_SRC}UE/API/USIM
INCLUDES += -I ${NAS_SRC}UE/EMM
INCLUDES += -I ${NAS_SRC}UE/EMM/SAP
INCLUDES += -I ${NAS_SRC}UE/ESM
INCLUDES += -I ${NAS_SRC}UE/ESM/SAP
INCLUDES += -I ${NFAPI_DIR}/nfapi/public_inc
INCLUDES += -I ${NFAPI_DIR}/common/public_inc
INCLUDES += -I ${NFAPI_DIR}/pnf/public_inc
INCLUDES += -I ${NFAPI_DIR}/nfapi/inc
INCLUDES += -I ${NFAPI_DIR}/sim_common/inc
INCLUDES += -I ${NFAPI_DIR}/pnf_sim/inc
INCLUDES += -I ${OPENAIR_DIR}/common/utils/ds
INCLUDES += -I ${OPENAIR1_DIR}/SIMULATION/TOOLS

INCLUDES += -I ${RRC_ASN1_DIR}
INCLUDES += -I ${NR_RRC_ASN1_DIR}
INCLUDES += -I ${NGAP_ASN1_DIR}
INCLUDES += -I ${S1AP_ASN1_DIR}
INCLUDES += -I ${E1AP_ASN1_DIR}
INCLUDES += -I ${F1AP_ASN1_DIR}
INCLUDES += -I ${X2AP_ASN1_DIR}
INCLUDES += -I ${M2AP_ASN1_DIR}
INCLUDES += -I ${M3AP_ASN1_DIR}

DEFINES += -DMAX_NUM_CCs=1
DEFINES += -DNB_ANTENNAS_RX=1
DEFINES += -DNB_ANTENNAS_TX=1
DEFINES += -DNAS_BUILT_IN_UE=1
DEFINES += -DNAS_UE=1
DEFINES += -DNUMBER_OF_UE_MAX_NB_IoT=16
# DEFINES += "-DPACKAGE_VERSION=\"Branch: ${GIT_BRANCH} Abrev. Hash: ${GIT_COMMIT_HASH} Date: ${GIT_COMMIT_DATE}\""
DEFINES += -DPACKAGE_VERSION=\"Ciao\"

# UTIL library
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/LOG/log.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/LOG/vcd_signal_dumper.c
UTIL_SRCS += ${OPENAIR2_DIR}/UTIL/MATH/oml.c
UTIL_SRCS += ${OPENAIR2_DIR}/UTIL/OPT/probe.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/threadPool/thread-pool.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/utils.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/system.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/time_meas.c
UTIL_SRCS += ${OPENAIR_DIR}/common/utils/time_stat.c
UTIL_LNKS += -lpthread

# HASHTABLE library
HASHTABLE_SRCS += ${OPENAIR_DIR}/common/utils/hashtable/hashtable.c
HASHTABLE_SRCS += ${OPENAIR_DIR}/common/utils/hashtable/obj_hashtable.c

# SCTP_CLIENT library
SCTP_CLIENT_SRCS += ${OPENAIR3_DIR}/SCTP/sctp_common.c
SCTP_CLIENT_SRCS += ${OPENAIR3_DIR}/SCTP/sctp_eNB_task.c
SCTP_CLIENT_SRCS += ${OPENAIR3_DIR}/SCTP/sctp_eNB_itti_messaging.c
# target_link_libraries(SCTP_CLIENT PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# SCHED_LIB library
SCHED_LIB_SRCS += ${OPENAIR1_DIR}/SCHED/fapi_l1.c
SCHED_LIB_SRCS += ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_eNb.c
SCHED_LIB_SRCS += ${OPENAIR1_DIR}/SCHED/phy_procedures_lte_common.c
# target_link_libraries(SCHED_LIB PRIVATE asn1_lte_rrc_hdrs)

# SCHED_RU_LIB library
SCHED_RU_LIB_SRCS += ${OPENAIR1_DIR}/SCHED/ru_procedures.c
SCHED_RU_LIB_SRCS += ${OPENAIR1_DIR}/SCHED_NR/nr_ru_procedures.c
SCHED_RU_LIB_SRCS += ${OPENAIR1_DIR}/SCHED/prach_procedures.c
# target_link_libraries(SCHED_RU_LIB PRIVATE asn1_lte_rrc_hdrs}

# SCHED_NR_LIB library
SCHED_NR_LIB_SRCS += ${OPENAIR1_DIR}/SCHED_NR/fapi_nr_l1.c
SCHED_NR_LIB_SRCS += ${OPENAIR1_DIR}/SCHED_NR/phy_procedures_nr_gNB.c
SCHED_NR_LIB_SRCS += ${OPENAIR1_DIR}/SCHED_NR/nr_prach_procedures.c
SCHED_NR_LIB_SRCS += ${OPENAIR1_DIR}/SCHED_NR/phy_frame_config_nr.c
# target_link_libraries(SCHED_NR_LIB PRIVATE asn1_nr_rrc_hdrs)

# Some definitions for PHY libraries
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nr_polar_init.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_bitwise_operations.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_crc_byte.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_crc.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_decoder.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_decoding_tools.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_encoder.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_interleaving_pattern.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_kernal_operation.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_kronecker_power_matrices.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_matrix_and_array.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_procedures.c
PHY_POLARSRC += ${OPENAIR1_DIR}/PHY/CODING/nrPolar_tools/nr_polar_sequence_pattern.c
PHY_SMALLBLOCKSRC += ${OPENAIR1_DIR}/PHY/CODING/nrSmallBlock/encodeSmallBlock.c
PHY_SMALLBLOCKSRC += ${OPENAIR1_DIR}/PHY/CODING/nrSmallBlock/decodeSmallBlock.c
PHY_NR_CODINGIF += ${OPENAIR1_DIR}/PHY/CODING/nrLDPC_load.c
PHY_TURBOIF += ${OPENAIR1_DIR}/PHY/CODING/coding_load.c

# PHY_NR library
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/INIT/nr_init.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/INIT/nr_parms.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/nr_modulation.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prs.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_pss.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_sss.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_pbch.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dci.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dci_tools.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch_tools.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_dlsch_coding.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_decoding.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_tbs_tools.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_sch_dmrs.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prach.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_llr_computation.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_ulsch_demodulation.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/ul_ref_seq_nr.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_dmrs_rx.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gold.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/nr_gen_mod_table.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/dmrs_nr.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/ptrs_nr.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_ul_channel_estimation.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_freq_equalization.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_ESTIMATION/nr_measurements_gNB.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/file_output.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cdot_prod.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/sqrt.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
PHY_NR_SRCS += ${PHY_POLARSRC}
PHY_NR_SRCS += ${PHY_SMALLBLOCKSRC}
PHY_NR_SRCS += ${PHY_NR_SRCS_CODINGIF}
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/pucch_rx.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/srs_rx.c
PHY_NR_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_uci_tools_common.c

# PHY library
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pss.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sss.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/sss_gen.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pilots.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pilots_mbsfn.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_coding.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_modulation.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci_tools.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pbch.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/edci.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/phich.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pcfich.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pucch.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pmch.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/ulsch_decoding.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/rar_tools.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/uci_tools.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_eNB_measurements.c
PHY_SRCS += ${OPENAIR1_DIR}/PHY/INIT/lte_init.c
# target_link_libraries(PHY PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# PHY_COMMON library
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dci_tools_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/lte_mcs.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/get_pmi.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/group_hopping.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/phich_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pcfich_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pmch_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/power_control.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/prach_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/pucch_common.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/dlsch_scrambling.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/srs_modulation.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/ofdm_mod.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_sync_time.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_cell_spec.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_uespec.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_gold.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_gold_mbsfn.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_dl_mbsfn.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/LTE_REFSIG/lte_ul_ref.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/lte_segmentation.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/nr_segmentation.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/nr_rate_matching.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/ccoding_byte.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/ccoding_byte_lte.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/3gpplte_sse.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/crc_byte.c
PHY_COMMON_SRCS += ${PHY_TURBOIF}
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/lte_rate_matching.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/viterbi.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/CODING/viterbi_lte.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/INIT/init_top.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/INIT/lte_parms.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cadd_vv.c
PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/dfts_load.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/log2_approx.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cmult_sv.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cmult_vv.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/cdot_prod.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/signal_energy.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/dB_routines.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/sqrt.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/lut.c
# PHY_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/simde_operations.c
# target_link_libraries(PHY_COMMON PRIVATE shlib_loader)
# add_dependencies(PHY_COMMON dfts)
# target_link_libraries(PHY_COMMON PRIVATE asn1_lte_rrc_hdrs)

# PHY_NR_COMMON library
PHY_NR_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_prach_common.c
PHY_NR_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_csi_rs.c
PHY_NR_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/NR_TRANSPORT/nr_scrambling.c
PHY_NR_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/NR_REFSIG/scrambling_luts.c
PHY_NR_COMMON_SRCS += ${OPENAIR1_DIR}/PHY/NR_UE_TRANSPORT/srs_modulation_nr.c

# PHY_RU library
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/if4_tools.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_UE_TRANSPORT/drs_modulation.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/ulsch_demodulation.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_ul_channel_estimation.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/lte_adjust_sync_eNB.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_ESTIMATION/freq_equalization.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_ul.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/slot_fep_nr.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/ul_7_5_kHz.c
CXX_SRCS    += ${OPENAIR1_DIR}/PHY/MODULATION/gen_75KHz.cpp
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/beamforming.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/MODULATION/compute_bf_weights.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/INIT/lte_init_ru.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/INIT/nr_init_ru.c
PHY_RU_SRCS += ${OPENAIR1_DIR}/PHY/LTE_TRANSPORT/prach.c
# target_link_libraries(PHY_RU PRIVATE asn1_lte_rrc_hdrs)

# GTPV1U library
GTPV1U_SRCS += ${NR_RRC_DIR}/rrc_gNB_GTPV1U.c
GTPV1U_SRCS += ${RRC_DIR}/rrc_eNB_GTPV1U.c
CXX_SRCS += ${OPENAIR3_DIR}/ocp-gtpu/gtp_itf.cpp
# target_link_libraries(GTPV1U PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
# target_link_libraries(GTPV1U PRIVATE SIMU)

# SECURITY library
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/secu_defs.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/kdf.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/aes_128_ctr.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/aes_128_cbc_cmac.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/aes_128_ecb.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/sha_256_hmac.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/rijndael.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/snow3g.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/key_nas_deriver.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/nas_stream_eea0.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/nas_stream_eea1.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/nas_stream_eia1.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/nas_stream_eea2.c
SECURITY_SRCS += ${OPENAIR3_DIR}/SECU/nas_stream_eia2.c
# add_library(SECURITY ${SECURITY_SRC} $<TARGET_OBJECTS:ds>)
# target_include_directories(SECURITY PRIVATE ${OPENAIR_DIR}/common/utils/ds/)
# target_link_libraries(SECURITY PRIVATE ${OpenSSL_LIBRARIES})

# ITTI library
ITTI_SRCS += ${OCP_ITTI}/intertask_interface.cpp
CXX_SRCS += ${OCP_ITTI}/intertask_interface.cpp
# target_link_libraries(ITTI PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# LIB_NAS_UE library
NAS_SRC := ${OPENAIR3_DIR}/NAS/
libnas_api_OBJS += ${NAS_SRC}COMMON/API/NETWORK/as_message.c
libnas_api_OBJS += ${NAS_SRC}COMMON/API/NETWORK/nas_message.c
libnas_api_OBJS += ${NAS_SRC}COMMON/API/NETWORK/network_api.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USER/at_command.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USER/at_error.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USER/at_response.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USER/user_api.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USER/user_indication.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USIM/aka_functions.c
libnas_ue_api_OBJS += ${NAS_SRC}UE/API/USIM/usim_api.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AttachAccept.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AttachComplete.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AttachReject.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AttachRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AuthenticationFailure.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AuthenticationReject.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AuthenticationRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/AuthenticationResponse.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/CsServiceNotification.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/DetachAccept.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/DetachRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/DownlinkNasTransport.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/EmmInformation.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/emm_msg.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/EmmStatus.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/ExtendedServiceRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/GutiReallocationCommand.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/GutiReallocationComplete.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/IdentityRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/IdentityResponse.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/SecurityModeCommand.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/SecurityModeComplete.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/SecurityModeReject.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/ServiceReject.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/ServiceRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateAccept.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateComplete.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateReject.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/TrackingAreaUpdateRequest.c
libnas_emm_msg_OBJS += ${NAS_SRC}COMMON/EMM/MSG/UplinkNasTransport.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextAccept.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextAccept.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ActivateDefaultEpsBearerContextRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/BearerResourceAllocationReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/BearerResourceAllocationRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/BearerResourceModificationReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/BearerResourceModificationRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/DeactivateEpsBearerContextAccept.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/DeactivateEpsBearerContextRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/EsmInformationRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/EsmInformationResponse.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/esm_msg.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/EsmStatus.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextAccept.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/ModifyEpsBearerContextRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/PdnConnectivityReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/PdnConnectivityRequest.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/PdnDisconnectReject.c
libnas_esm_msg_OBJS += ${NAS_SRC}COMMON/ESM/MSG/PdnDisconnectRequest.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AccessPointName.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AdditionalUpdateResult.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AdditionalUpdateType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ApnAggregateMaximumBitRate.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AuthenticationFailureParameter.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AuthenticationParameterAutn.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AuthenticationParameterRand.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/AuthenticationResponseParameter.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/CipheringKeySequenceNumber.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/Cli.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/CsfbResponse.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/DaylightSavingTime.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/DetachType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/DrxParameter.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EmergencyNumberList.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EmmCause.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsAttachResult.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsAttachType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsBearerContextStatus.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsBearerIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsMobileIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsNetworkFeatureSupport.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsQualityOfService.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsUpdateResult.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EpsUpdateType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EsmCause.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EsmInformationTransferFlag.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/EsmMessageContainer.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/GprsTimer.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/GutiType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/IdentityType2.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ImeisvRequest.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/KsiAndSequenceNumber.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/LcsClientIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/LcsIndicator.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/LinkedEpsBearerIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/LlcServiceAccessPointIdentifier.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/LocationAreaIdentification.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MessageType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MobileIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MobileStationClassmark2.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MobileStationClassmark3.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MsNetworkCapability.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/MsNetworkFeatureSupport.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/NasKeySetIdentifier.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/NasMessageContainer.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/NasRequestType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/NasSecurityAlgorithms.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/NetworkName.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/Nonce.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PacketFlowIdentifier.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PagingIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PdnAddress.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PdnType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PlmnList.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ProcedureTransactionIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ProtocolConfigurationOptions.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ProtocolDiscriminator.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/PTmsiSignature.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/QualityOfService.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/RadioPriority.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/SecurityHeaderType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ServiceType.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/ShortMac.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/SsCode.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/SupportedCodecList.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TimeZoneAndTime.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TimeZone.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TmsiStatus.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TrackingAreaIdentity.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TrackingAreaIdentityList.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TrafficFlowAggregateDescription.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TrafficFlowTemplate.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/TransactionIdentifier.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/UeNetworkCapability.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/UeRadioCapabilityInformationUpdateNeeded.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/UeSecurityCapability.c
libnas_ies_OBJS += ${NAS_SRC}COMMON/IES/VoiceDomainPreferenceAndUeUsageSetting.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/device.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/memory.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/nas_timer.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/socket.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/stty.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/TLVEncoder.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/TLVDecoder.c
libnas_utils_OBJS += ${NAS_SRC}COMMON/UTIL/OctetString.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/Attach.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/Authentication.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/Detach.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/emm_main.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/EmmStatusHdl.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/Identification.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/IdleMode.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/LowerLayer.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/SecurityModeControl.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/ServiceRequestHdl.c
libnas_ue_emm_OBJS += ${NAS_SRC}UE/EMM/TrackingAreaUpdate.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_as.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredAttachNeeded.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredAttemptingToAttach.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregistered.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredInitiated.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredLimitedService.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNoCellAvailable.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNoImsi.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredNormalService.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmDeregisteredPlmnSearch.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_esm.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_fsm.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmNull.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_recv.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_reg.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredAttemptingToUpdate.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegistered.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredImsiDetachInitiated.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredInitiated.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredLimitedService.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredNoCellAvailable.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredNormalService.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredPlmnSearch.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmRegisteredUpdateNeeded.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_sap.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/emm_send.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmServiceRequestInitiated.c
libnas_ue_emm_sap_OBJS += ${NAS_SRC}UE/EMM/SAP/EmmTrackingAreaUpdatingInitiated.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/DedicatedEpsBearerContextActivation.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/DefaultEpsBearerContextActivation.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/EpsBearerContextDeactivation.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/esm_ebr.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/esm_ebr_context.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/esm_ip.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/esm_main.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/esm_pt.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/EsmStatusHdl.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/PdnConnectivity.c
libnas_ue_esm_OBJS += ${NAS_SRC}UE/ESM/PdnDisconnect.c
libnas_ue_esm_sap_OBJS += ${NAS_SRC}UE/ESM/SAP/esm_recv.c
libnas_ue_esm_sap_OBJS += ${NAS_SRC}UE/ESM/SAP/esm_send.c
libnas_ue_esm_sap_OBJS += ${NAS_SRC}UE/ESM/SAP/esm_sap.c
LIB_NAS_UE_SRCS += ${NAS_SRC}UE/nas_itti_messaging.c
LIB_NAS_UE_SRCS += ${NAS_SRC}UE/nas_network.c
LIB_NAS_UE_SRCS += ${NAS_SRC}UE/nas_parser.c
LIB_NAS_UE_SRCS += ${NAS_SRC}UE/nas_proc.c
LIB_NAS_UE_SRCS += ${NAS_SRC}UE/nas_user.c
LIB_NAS_UE_SRCS += ${libnas_api_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ue_api_OBJS}
LIB_NAS_UE_SRCS += ${libnas_emm_msg_OBJS}
LIB_NAS_UE_SRCS += ${libnas_esm_msg_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ies_OBJS}
LIB_NAS_UE_SRCS += ${libnas_utils_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ue_emm_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ue_emm_sap_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ue_esm_OBJS}
LIB_NAS_UE_SRCS += ${libnas_ue_esm_sap_OBJS}
# target_link_libraries(LIB_NAS_UE PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# lte_rrc library
lte_rrc_SRCS += ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg.c
lte_rrc_SRCS += ${OPENAIR2_DIR}/RRC/LTE/MESSAGES/asn1_msg_NB_IoT.c
# target_link_libraries(lte_rrc PUBLIC asn1_lte_rrc)
# target_link_libraries(lte_rrc PRIVATE nr_rrc)

# nr_rrc library
nr_rrc_SRCS += ${OPENAIR2_DIR}/RRC/NR/MESSAGES/asn1_msg.c
# target_link_libraries(nr_rrc PUBLIC asn1_nr_rrc asn1_lte_rrc)

# ngap library
ngap_SRCS += ${NGAP_DIR}/ngap_gNB.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_context_management_procedures.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_decoder.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_encoder.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_handlers.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_itti_messaging.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_management_procedures.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_nas_procedures.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_nnsf.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_overload.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_trace.c
ngap_SRCS += ${NGAP_DIR}/ngap_gNB_ue_context.c
# target_link_libraries(ngap PUBLIC asn1_ngap)
# target_link_libraries(ngap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# s1ap library
s1ap_SRCS += ${S1AP_DIR}/s1ap_common.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_context_management_procedures.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_decoder.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_encoder.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_handlers.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_itti_messaging.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_management_procedures.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_nas_procedures.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_nnsf.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_overload.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_trace.c
s1ap_SRCS += ${S1AP_DIR}/s1ap_eNB_ue_context.c
# target_link_libraries(s1ap PUBLIC asn1_s1ap)
# target_link_libraries(s1ap PRIVATE nr_rrc asn1_nr_rrc_hdrs lte_rrc)

# L2_LTE_NR library
L2_RRC_SRC += ${OPENAIR2_DIR}/LAYER2/openair2_proc.c
L2_RRC_SRC += ${RRC_DIR}/rrc_eNB.c
L2_RRC_SRC += ${RRC_DIR}/rrc_eNB_endc.c
L2_RRC_SRC += ${RRC_DIR}/rrc_eNB_S1AP.c
L2_RRC_SRC += ${RRC_DIR}/rrc_eNB_M2AP.c
L2_RRC_SRC += ${RRC_DIR}/rrc_eNB_UE_context.c
L2_RRC_SRC += ${RRC_DIR}/rrc_common.c
L2_RRC_SRC += ${RRC_DIR}/L2_interface.c
L2_RRC_SRC += ${RRC_DIR}/L2_interface_common.c
L2_RRC_SRC += ${RRC_DIR}/L2_interface_ue.c
MAC_SRC += ${PHY_INTERFACE_DIR}/IF_Module.c
MAC_SRC += ${MAC_DIR}/main.c
MAC_SRC += ${MAC_DIR}/l1_helpers.c
MAC_SRC += ${MAC_DIR}/rar_tools.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_dlsch.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_ulsch.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_mch.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_bch.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_primitives.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_RA.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_fairRR.c
MAC_SRC += ${MAC_DIR}/eNB_scheduler_phytest.c
MAC_SRC += ${MAC_DIR}/pre_processor.c
MAC_SRC += ${MAC_DIR}/slicing/slicing.c
MAC_SRC += ${MAC_DIR}/config.c
MAC_SRC += ${MAC_DIR}/config_ue.c
ENB_APP_SRC += ${OPENAIR2_DIR}/ENB_APP/enb_app.c
ENB_APP_SRC += ${OPENAIR2_DIR}/ENB_APP/enb_config.c
ENB_APP_SRC += ${OPENAIR2_DIR}/ENB_APP/enb_config_SL.c
ENB_APP_SRC += ${OPENAIR2_DIR}/ENB_APP/enb_config_eMTC.c
ENB_APP_SRC += ${OPENAIR2_DIR}/ENB_APP/RRC_config_tools.c
MCE_APP_SRC += ${OPENAIR2_DIR}/MCE_APP/mce_app.c
MCE_APP_SRC += ${OPENAIR2_DIR}/MCE_APP/mce_config.c
L2_LTE_NR_SRCS += ${L2_RRC_SRC}
L2_LTE_NR_SRCS += ${MAC_SRC}
L2_LTE_NR_SRCS += ${ENB_APP_SRC}
L2_LTE_NR_SRCS += ${MCE_APP_SRC}
# target_link_libraries(L2_LTE_NR PRIVATE f1ap s1ap nr_rrc)

# L2_NR library
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_oai_api.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_ue_manager.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_entity.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_sdu.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_timer_thread.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_security_nea2.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_integrity_nia2.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/nr_pdcp_integrity_nia1.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/asn1_utils.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/cucp_cuup_handler.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/cuup_cucp_if.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/cuup_cucp_direct.c
NR_PDCP_SRC += ${OPENAIR2_DIR}/LAYER2/nr_pdcp/cuup_cucp_e1ap.c
NR_SDAP_SRC += ${OPENAIR2_DIR}/SDAP/nr_sdap/nr_sdap.c
NR_SDAP_SRC += ${OPENAIR2_DIR}/SDAP/nr_sdap/nr_sdap_entity.c
L2_NR_SRC += ${NR_PDCP_SRC}
L2_NR_SRC += ${NR_SDAP_SRC}
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB.c
L2_NR_SRC += ${NR_RRC_DIR}/nr_rrc_common.c
L2_NR_SRC += ${NR_RRC_DIR}/mac_rrc_dl_direct.c
L2_NR_SRC += ${NR_RRC_DIR}/mac_rrc_dl_f1ap.c
L2_NR_SRC += ${NR_RRC_DIR}/nr_rrc_config.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_nsa.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_internode.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_reconfig.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_UE_context.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_NGAP.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_radio_bearers.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_cuup.c
L2_NR_SRC += ${NR_RRC_DIR}/rrc_gNB_du.c
MAC_NR_SRC += ${NR_PHY_INTERFACE_DIR}/NR_IF_Module.c
MAC_NR_SRC += ${NR_PHY_INTERFACE_DIR}/nr_sched_response.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/main.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/config.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_bch.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_dlsch.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_ulsch.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_primitives.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_phytest.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_uci.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_srs.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/gNB_scheduler_RA.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/mac_rrc_dl_handler.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/mac_rrc_ul_direct.c
MAC_NR_SRC += ${NR_GNB_MAC_DIR}/mac_rrc_ul_f1ap.c
GNB_APP_SRC += ${OPENAIR2_DIR}/GNB_APP/gnb_app.c
GNB_APP_SRC += ${OPENAIR2_DIR}/E1AP/e1ap_setup.c
GNB_APP_SRC += ${OPENAIR2_DIR}/GNB_APP/gnb_config.c
L2_NR_SRCS += ${L2_NR_SRC}
L2_NR_SRCS += ${MAC_NR_SRC}
L2_NR_SRCS += ${GNB_APP_SRC}

# MAC_NR_COMMON library
MAC_NR_COMMON_SRCS += ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_mac_common.c
MAC_NR_COMMON_SRCS += ${OPENAIR2_DIR}/LAYER2/NR_MAC_COMMON/nr_compute_tbs_common.c
# target_link_libraries(MAC_NR_COMMON PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# NFAPI_COMMON_LIB library
NFAPI_COMMON_LIB_SRCS += ${NFAPI_DIR}/common/src/debug.c

# NFAPI_LIB library
NFAPI_LIB_SRCS += ${NFAPI_DIR}/nfapi/src/nfapi.c
NFAPI_LIB_SRCS += ${NFAPI_DIR}/nfapi/src/nfapi_p4.c
NFAPI_LIB_SRCS += ${NFAPI_DIR}/nfapi/src/nfapi_p5.c
NFAPI_LIB_SRCS += ${NFAPI_DIR}/nfapi/src/nfapi_p7.c

# NFAPI_VNF_LIB library
NFAPI_VNF_LIB_SRCS += ${NFAPI_DIR}/vnf/src/vnf.c
NFAPI_VNF_LIB_SRCS += ${NFAPI_DIR}/vnf/src/vnf_interface.c
NFAPI_VNF_LIB_SRCS += ${NFAPI_DIR}/vnf/src/vnf_p7.c
NFAPI_VNF_LIB_SRCS += ${NFAPI_DIR}/vnf/src/vnf_p7_interface.c
# target_link_libraries(NFAPI_VNF_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# NFAPI_PNF_LIB library
NFAPI_PNF_LIB_SRCS += ${NFAPI_DIR}/pnf/src/pnf.c
NFAPI_PNF_LIB_SRCS += ${NFAPI_DIR}/pnf/src/pnf_interface.c
NFAPI_PNF_LIB_SRCS += ${NFAPI_DIR}/pnf/src/pnf_p7.c
NFAPI_PNF_LIB_SRCS += ${NFAPI_DIR}/pnf/src/pnf_p7_interface.c
# target_link_libraries(NFAPI_PNF_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# NFAPI_USER_LIB library
NFAPI_USER_LIB_SRCS += ${NFAPI_USER_DIR}/nfapi.c
NFAPI_USER_LIB_SRCS += ${NFAPI_USER_DIR}/nfapi_pnf.c
NFAPI_USER_LIB_SRCS += ${NFAPI_USER_DIR}/nfapi_vnf.c
NFAPI_USER_LIB_SRCS += ${NFAPI_USER_DIR}/gnb_ind_vars.c
# target_link_libraries(NFAPI_USER_LIB PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# SIMU library
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/taus.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_channel.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/multipath_tv_channel.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/abstraction.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/channel_sim.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/random_channel.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/rangen_double.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/TOOLS/phase_noise.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/RF/rf.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/RF/dac.c
SIMU_SRCS += ${OPENAIR1_DIR}/SIMULATION/RF/adc.c
# target_include_directories(SIMU PUBLIC ${OPENAIR1_DIR}/SIMULATION/TOOLS ${OPENAIR1_DIR}/SIMULATION/RF)

# SIMU_ETH library
SIMU_ETH_SRCS += ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/netlink_init.c
SIMU_ETH_SRCS += ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/multicast_link.c
SIMU_ETH_SRCS += ${OPENAIR1_DIR}/SIMULATION/ETH_TRANSPORT/socket.c
# target_link_libraries(SIMU_ETH PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)

# x2ap library
x2ap_SRCS += ${X2AP_DIR}/x2ap_common.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_decoder.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_encoder.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_handler.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_itti_messaging.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_management_procedures.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_eNB_generate_messages.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_ids.c
x2ap_SRCS += ${X2AP_DIR}/x2ap_timers.c
# target_link_libraries(x2ap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)

# f1ap library
f1ap_SRCS += ${F1AP_DIR}/f1ap_common.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_ids.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_interface_management.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_paging.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_rrc_message_transfer.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_task.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_ue_context_management.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_cu_warning_message_transmission.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_interface_management.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_paging.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_rrc_message_transfer.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_system_information.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_task.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_ue_context_management.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_du_warning_message_transmission.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_encoder.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_handlers.c
f1ap_SRCS += ${F1AP_DIR}/f1ap_itti_messaging.c
# target_link_libraries(f1ap PUBLIC asn1_f1ap)
# target_link_libraries(f1ap PRIVATE ngap nr_rrc HASHTABLE)

# m2ap library
m2ap_SRCS += ${M2AP_DIR}/m2ap_common.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_eNB.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_MCE.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_decoder.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_encoder.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_MCE_handler.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_eNB_handler.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_itti_messaging.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_eNB_management_procedures.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_eNB_generate_messages.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_MCE_management_procedures.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_MCE_generate_messages.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_ids.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_timers.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_MCE_interface_management.c
m2ap_SRCS += ${M2AP_DIR}/m2ap_eNB_interface_management.c
# target_link_libraries(m2ap PUBLIC asn1_m2ap)
# target_link_libraries(m2ap PRIVATE lte_rrc nr_rrc)
# target_include_directories(m2ap PRIVATE "${M2AP_DIR}")

# m3ap library
m3ap_SRCS += ${M3AP_DIR}/m3ap_common.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_decoder.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_encoder.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MCE_handler.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MME_handler.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MME.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MME_management_procedures.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MME_interface_management.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MCE.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MCE_management_procedures.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_MCE_interface_management.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_itti_messaging.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_ids.c
m3ap_SRCS += ${M3AP_DIR}/m3ap_timers.c
# target_link_libraries(m3ap PUBLIC asn1_m3ap)
# target_link_libraries(m3ap PRIVATE m2ap)
# target_link_libraries(m3ap PRIVATE asn1_lte_rrc_hdrs asn1_nr_rrc_hdrs)
# target_include_directories(m3ap PUBLIC "${M3AP_DIR}")

# e1ap library
e1ap_SRCS += ${E1AP_DIR}/e1ap.c
e1ap_SRCS += ${E1AP_DIR}/e1ap_common.c
# target_link_libraries(e1ap
#                       PUBLIC asn1_e1ap f1ap
#                       PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs asn1_f1ap UTIL e1_if)

# shlib_loader library
shlib_loader_SRCS += ${OPENAIR_DIR}/common/utils/load_module_shlib.c
# target_link_libraries(shlib_loader PRIVATE CONFIG_LIB)

# config_internals library
config_internals_SRCS += ${CONFIG_ROOTDIR}/config_common.c

# CONFIG_LIB library
CONFIG_LIB_SRCS += ${CONFIG_ROOTDIR}/config_load_configmodule.c
CONFIG_LIB_SRCS += ${CONFIG_ROOTDIR}/config_userapi.c
CONFIG_LIB_SRCS += ${CONFIG_ROOTDIR}/config_cmdline.c

# nr_rlc_core library
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_entity.c
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_entity_am.c
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_entity_tm.c
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_entity_um.c
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_pdu.c
nr_rlc_core_SRCS += ${NR_RLC_DIR}/nr_rlc_sdu.c
# target_link_libraries(nr_rlc_core PRIVATE ${T_LIB})

# nr_rlc library
nr_rlc_SRCS += ${NR_RLC_DIR}/asn1_utils.c
nr_rlc_SRCS += ${NR_RLC_DIR}/nr_rlc_oai_api.c
nr_rlc_SRCS += ${NR_RLC_DIR}/nr_rlc_ue_manager.c
# target_link_libraries(nr_rlc PUBLIC pthread f1ap nr_rlc_core UTIL ${T_LIB})
# target_link_libraries(nr_rlc PRIVATE asn1_lte_rrc asn1_nr_rrc)

# e1_if library
e1_if_SRCS += ${NR_RRC_DIR}/cucp_cuup_direct.c
e1_if_SRCS += ${NR_RRC_DIR}/cucp_cuup_e1ap.c

# *_ASN1 libraries
include rrc_asn1.mk
include nr_rrc_asn1.mk
include ngap_asn1.mk
include s1ap_asn1.mk
include e1ap_asn1.mk
include f1ap_asn1.mk
include x2ap_asn1.mk
include m2ap_asn1.mk
include m3ap_asn1.mk

# nr-softmodem executable
nr-softmodem_SRCS += ${OPENAIR_DIR}/executables/nr-gnb.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/executables/nr-ru.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/executables/nr-softmodem.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/executables/softmodem-common.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/radio/COMMON/common_lib.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/radio/COMMON/record_player.c
nr-softmodem_SRCS += ${OPENAIR2_DIR}/RRC/NAS/nas_config.c
nr-softmodem_SRCS += ${OPENAIR2_DIR}/RRC/NAS/rb_config.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/common/utils/lte/ue_power.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/common/utils/lte/prach_utils.c
nr-softmodem_SRCS += ${OPENAIR_DIR}/common/utils/nr/nr_common.c
nr-softmodem_SRCS += ${PHY_INTERFACE_DIR}/queue_t.c
nr-softmodem_SRCS += ${OPENAIR1_DIR}/PHY/TOOLS/phy_scope_interface.c

SRCS += ${UTIL_SRCS} ${HASHTABLE_SRCS} ${SCTP_CLIENT_SRCS} ${SCHED_LIB_SRCS}
SRCS += ${SCHED_RU_LIB_SRCS} ${SCHED_NR_LIB_SRCS} ${PHY_NR_SRCS} ${PHY_SRCS}
SRCS += ${PHY_COMMON_SRCS} ${PHY_NR_COMMON_SRCS} ${PHY_RU_SRCS} ${GTPV1U_SRCS}
SRCS += ${SECURITY_SRCS} ${LIB_NAS_UE_SRCS} ${lte_rrc_SRCS}
SRCS += ${nr_rrc_SRCS} ${ngap_SRCS} ${s1ap_SRCS} ${L2_LTE_NR_SRCS} ${L2_NR_SRCS}
SRCS += ${MAC_NR_COMMON_SRCS} ${NFAPI_COMMON_LIB_SRCS} ${NFAPI_LIB_SRCS}
SRCS += ${NFAPI_VNF_LIB_SRCS} ${NFAPI_PNF_LIB_SRCS} ${NFAPI_USER_LIB_SRCS}
SRCS += ${SIMU_SRCS} ${SIMU_ETH_SRCS} ${x2ap_SRCS} ${f1ap_SRCS} ${m2ap_SRCS}
SRCS += ${m3ap_SRCS} ${e1ap_SRCS} ${shlib_loader_SRCS} ${config_internals_SRCS}
SRCS += ${CONFIG_LIB_SRCS} ${nr_rlc_core_SRCS} ${nr_rlc_SRCS} ${e1_if_SRCS}
SRCS += ${RRC_ASN1_SRCS} ${NR_RRC_ASN1_SRCS} ${NGAP_ASN1_SRCS} ${S1AP_ASN1_SRCS}
SRCS += ${E1AP_ASN1_SRCS} ${F1AP_ASN1_SRCS} ${X2AP_ASN1_SRCS} ${M2AP_ASN1_SRCS}
SRCS += ${M3AP_ASN1_SRCS}
SRCS += ${nr-softmodem_SRCS}
OBJS := ${patsubst %.c,build/%.o,${SRCS}}

CXX_OBJS := ${patsubst %.cpp,build/%.o,${CXX_SRCS}}

nr-softmodem: ${OBJS} ${CXX_OBJS}
	${info CXX $@}
	@${CXX} -o $@ ${OBJS} ${CXX_OBJS} ${LDFLAGS}

${OBJS}: build/%.o: %.c
	@mkdir -p ${dir $@}
	${info CC $@}
	@${CC} -o $@ -c $< ${INCLUDES} ${DEFINES} ${CFLAGS} -std=gnu11

${CXX_OBJS}: build/%.o: %.cpp
	@mkdir -p ${dir $@}
	${info CXX $@}
	@${CXX} -o $@ -c $< ${INCLUDES} ${DEFINES} ${CFLAGS} -std=c++11

clean:
	@rm -f ${OBJS}
	@rm -f ${CXX_OBJS}
	@rm -f nr-softmodem

# target_link_libraries(nr-softmodem PRIVATE
#   -Wl,--start-group
#   UTIL HASHTABLE SCTP_CLIENT SCHED_LIB SCHED_RU_LIB SCHED_NR_LIB PHY_NR PHY PHY_COMMON PHY_NR_COMMON PHY_RU GTPV1U SECURITY
#   ITTI ${NAS_UE_LIB} lte_rrc nr_rrc
#   ngap s1ap L2_LTE_NR L2_NR MAC_NR_COMMON NFAPI_COMMON_LIB NFAPI_LIB NFAPI_VNF_LIB NFAPI_PNF_LIB NFAPI_USER_LIB SIMU SIMU_ETH
#   x2ap f1ap m2ap m3ap e1ap shlib_loader
#   -Wl,--end-group z dl)

# target_link_libraries(nr-softmodem PRIVATE pthread m CONFIG_LIB rt sctp)
# target_link_libraries(nr-softmodem PRIVATE ${T_LIB})
# target_link_libraries(nr-softmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
# target_link_libraries(nr-softmodem PRIVATE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs)
# if(E2_AGENT)
#   target_compile_definitions(nr-softmodem PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
# endif()