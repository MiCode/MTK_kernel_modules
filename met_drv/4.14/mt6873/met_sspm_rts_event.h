/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
MET_SSPM_RTS_EVNET(SSPM_PTPOD, "_id,voltage")
MET_SSPM_RTS_EVNET(SSPM_MET_UNIT_TEST, "test")
MET_SSPM_RTS_EVNET(SSPM_QOS_BOUND_STATE, "idx,state,num,event,emibw_mon_total,emibw_mon_cpu,emibw_mon_gpu,emibw_mon_mm,emibw_mon_md,emibw_req_total,emibw_req_cpu,emibw_req_gpu,emibw_req_mm,emibw_req_md,smibw_mon_venc,smibw_mon_cam,smibw_mon_img,smibw_mon_mdp,smibw_mon_gpu,smibw_mon_apu,smibw_mon_vpu0,smibw_mon_vpu1,smibw_mon_mdla0,smibw_mon_edma0,smibw_mon_apumd32,smibw_req_venc,smibw_req_cam,smibw_req_img,smibw_req_mdp,smibw_req_gpu,smibw_req_apu,smibw_req_vpu0,smibw_req_vpu1,smibw_req_mdla0,smibw_req_edma0,smibw_req_apumd32,lat_mon_cpu,lat_mon_vpu0,lat_mon_vpu1,lat_mon_mdla0,lat_mon_edma0,lat_mon_apumd32")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_NON_WFX, "non_wfx_0,non_wfx_1,non_wfx_2,non_wfx_3,non_wfx_4,non_wfx_5,non_wfx_6,non_wfx_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_LOADING, "ratio,cps")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_POWER, "c_up_array_0,c_up_array_1,c_down_array_0,c_down_array_1,c_up_0,c_up_1,c_down_0,c_dwon_1,c_up,c_down,v_up,v_down,v2f_0,v2f_1")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_OPP, "v_dram_opp,v_dram_opp_cur,c_opp_cur_0,c_opp_cur_1,d_times_up,d_times_down")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_RATIO, "ratio_max_0,ratio_max_1,ratio_0,ratio_1,ratio_2,ratio_3,ratio_4,ratio_5,ratio_6,ratio_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_BW, "total_bw")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_CP_RATIO, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_VP_RATIO, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_DE_TIMES, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4,reset")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_ACTIVE_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_IDLE_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_OFF_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_STALL_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_PMU_L3DC, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_PMU_INST_SPEC, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_PMU_CYCLES, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__CORE_NON_WFX_CTR, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__DSU_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__DSU_L3_BW, "L3_BW")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__MCUSYS_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__MCUSYS_EMI_BW, "cpu_emi_bw")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__DVFS, "vproc2,vproc1,cpuL_freq,cpuB_freq,cpu_L_opp,cpu_B_opp,cci_volt,cci_freq,cci_opp")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__LKG_POWER, "cpu_L,cpu_B,dsu")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU__POWER, "cpu_L,cpu_B,dsu,mcusys")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__LOADING, "loading")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__DVFS, "vgpu,gpu_freq")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__URATE, "alu_fma,alu_cvt,alu_sfu,tex,lsc,l2c,vary,tiler,rast")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__THERMAL, "thermal,lkg")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__COUNTER, "GPU_ACTIVE,EXEC_INSTR_FMA,EXEC_INSTR_CVT,EXEC_INSTR_SFU,TEX,VARY_SLOT,L20,L21,L22,L23")
MET_SSPM_RTS_EVNET(SSPM_SWPM_GPU__POWER, "gpu")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CORE__INFRA_STATE_RATIO, "dact,cact,idle,dcm")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CORE__DVFS, "vcore")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CORE__POWER, "dramc,infra_top,aphy_vcore")
MET_SSPM_RTS_EVNET(SSPM_SWPM_DRAM__MEM_IDX, "read_bw,write_bw,srr_pct,pdir_pct,phr_pct,acc_util,mr4,ddr_freq")
MET_SSPM_RTS_EVNET(SSPM_SWPM_DRAM__POWER, "aphy_vddq_0p6v,aphy_vm_0p75v,aphy_vio_1p2v,dram_vddq_0p6v,dram_vdd2_1p1v,dram_vdd1_1p8v")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_MDLA_QOS_CNT__, "bw_MDLA0_0,bw_MDLA0_1,lt_MDLA0_0,lt_MDLA0_1,bw_MDLA0_0_all,bw_MDLA0_1_all,lt_MDLA0_0_all,lt_MDLA0_1_all")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_VPU_QOS_CNT__, "bw_VPU0,bw_VPU1,lt_VPU0,lt_VPU1,bw_VPU0_all,bw_VPU1_all,lt_VPU0_all,lt_VPU1_all")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_OTHERS_QOS_CNT__, "bw_EDMA0,bw_MD32,lt_EDMA0,lt_MD32,bw_EDMA0_all,bw_MD32_all,lt_EDMA0_all,lt_MD32_all")
MET_SSPM_RTS_EVNET(__SSPM_GPU_APU_SSC_CNT__, "APU_0_R,APU_0_W,GPU_0_R,GPU_0_W,APU_1_R,APU_1_W,GPU_1_R,GPU_1_W")
MET_SSPM_RTS_EVNET(__SSPM_BRISKET_CONFIG__, "b05,b06,b07,b08,b15,b16,b17,b18,b25,b26,b27,b28,b35,b36,b37,b38,b45,b46,b47,b48,b55,b56,b57,b58,b65,b66,b67,b68,b75,b76,b77,b78")
MET_SSPM_RTS_EVNET(__SSPM_CREDIT_BASE_CONFIG__, "c04,c05,c06,c07,c14,c15,c16,c17")
MET_SSPM_RTS_EVNET(__SSPM_DRCC_CONFIG__, "d00,d01,d02,d03,d04,d05,d06,d07,d10,d11,d12,d13,d14,d15,d16,d17,d20,d21,d22,d23,d24,d25,d26,d27")
MET_SSPM_RTS_EVNET(__SSPM_SES_CONFIG__, "e01,e02,e03,e04,e11,e12,e13,e14,e21,e22,e23,e24,e31,e32,e33,e34,e41,e42,e43,e44,e51,e52,e53,e54,e61,e62,e63,e64,e71,e72,e73,e74,e81,e82,e83,e84")
MET_SSPM_RTS_EVNET(__SSPM_BCDE_ACTIVE_COUNTER__, "b0,b1,b2,b3,b4,b5,b6,b7,c41,c42,c51,c52,c61,c62,c71,c72,d0,d1,d2,d3,d4,d5,d6,d7,e0,e1,e2,e3,e4,e5,e6,e7,e8")
MET_SSPM_RTS_EVNET(__SSPM_BRISKET_STATUS__, "b01,b02,b04,b11,b12,b14,b21,b22,b24,b31,b32,b34,b41,b42,b44,b51,b52,b54,b61,b62,b64,b71,b72,b74")
