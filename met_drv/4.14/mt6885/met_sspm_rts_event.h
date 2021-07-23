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
MET_SSPM_RTS_EVNET(SSPM_QOS_BOUND_STATE, "idx,state,num,event,emibw_mon_total,emibw_mon_cpu,emibw_mon_gpu,emibw_mon_mm,emibw_mon_md,emibw_req_total,emibw_req_cpu,emibw_req_gpu,emibw_req_mm,emibw_req_md,smibw_mon_venc,smibw_mon_cam,smibw_mon_img,smibw_mon_mdp,smibw_mon_gpu,smibw_mon_apu,smibw_mon_vpu0,smibw_mon_vpu1,smibw_mon_mdla,smibw_req_venc,smibw_req_cam,smibw_req_img,smibw_req_mdp,smibw_req_gpu,smibw_req_apu,smibw_req_vpu0,smibw_req_vpu1,smibw_req_vpu2,smibw_req_mdla0,smibw_req_mdla1,smibw_req_edma0,smibw_req_edma1,smibw_req_apumd32,lat_mon_cpu,lat_mon_vpu0,lat_mon_vpu1,lat_mon_vpu2, lat_mon_mdla0, lat_mon_mdla1, lat_mon_edma0, lat_mon_edma1, lat_mon_apumd32")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_NON_WFX, "non_wfx_0,non_wfx_1,non_wfx_2,non_wfx_3,non_wfx_4,non_wfx_5,non_wfx_6,non_wfx_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_LOADING, "ratio,cps")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_POWER, "c_up_array_0,c_up_array_1,c_down_array_0,c_down_array_1,c_up_0,c_up_1,c_down_0,c_dwon_1,c_up,c_down,v_up,v_down,v2f_0,v2f_1")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_OPP, "v_dram_opp,v_dram_opp_cur,c_opp_cur_0,c_opp_cur_1,d_times_up,d_times_down")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_RATIO, "ratio_max_0,ratio_max_1,ratio_0,ratio_1,ratio_2,ratio_3,ratio_4,ratio_5,ratio_6,ratio_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_BW, "total_bw")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_CP_RATIO, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_VP_RATIO, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_DE_TIMES, "up0,up1,up2,up3,up4,down0,down1,down2,down3,down4,reset")
MET_SSPM_RTS_EVNET(SSPM_QOS_PREFETCH_STATUS, "enable,ctrl,force,congestion,perf_hint")
MET_SSPM_RTS_EVNET(SSPM_QOS_PREFETCH_BW, "total_bw,cpu_bw")
MET_SSPM_RTS_EVNET(SSPM_QOS_PREFETCH_OPP, "ddr_opp,cpu1_opp,cpu2_opp")
MET_SSPM_RTS_EVNET(SSPM_QOS_PREFETCH_POWER, "vcore_power,vcore_power_cpu_bw,vcore_power_up,vcore_power_dn,cpu_power,cpu_power_up,cpu_power_dn")
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
MET_SSPM_RTS_EVNET(SSPM_SWPM_DRAM__MEM_IDX, "read_bw_0,read_bw_1,write_bw_0,write_bw_1,srr_pct,pdir_pct_0,pdir_pct_1,phr_pct_0,phr_pct_1,acc_util_0,acc_util_1,mr4,ddr_freq")
MET_SSPM_RTS_EVNET(SSPM_SWPM_DRAM__POWER, "aphy_vddq_0p6v,aphy_vm_0p75v,aphy_vio_1p2v,aphy_vio_1p8v,dram_vddq_0p6v,dram_vdd2_1p1v,dram_vdd1_1p8v")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU__VPU0_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU__VPU1_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU__DVFS, "vvpu,vpu0_freq,vpu1_freq")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU__POWER, "vpu")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_MDLA_QOS_CNT__, "bw_MDLA0_0,bw_MDLA0_1,bw_MDLA1_0,bw_MDLA1_1,lt_MDLA0_0,lt_MDLA0_1,lt_MDLA1_0,lt_MDLA1_1,bw_MDLA0_0_all,bw_MDLA0_1_all,bw_MDLA1_0_all,bw_MDLA1_1_all,lt_MDLA0_0_all,lt_MDLA0_1_all,lt_MDLA1_0_all,lt_MDLA1_1_all")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_VPU_QOS_CNT__, "bw_VPU0,bw_VPU1,bw_VPU2,lt_VPU0,lt_VPU1,lt_VPU2,bw_VPU0_all,bw_VPU1_all,bw_VPU2_all,lt_VPU0_all,lt_VPU1_all,lt_VPU2_all")
MET_SSPM_RTS_EVNET(__SSPM_APUSYS_OTHERS_QOS_CNT__, "bw_EDMA0,bw_EDMA1,bw_MD32,lt_EDMA0,lt_EDMA1,lt_MD32,bw_EDMA0_all,bw_EDMA1_all,bw_MD32_all,lt_EDMA0_all,lt_EDMA1_all,lt_MD32_all")
MET_SSPM_RTS_EVNET(__SSPM_GPU_APU_SSC_CNT__, "N_APU_0_R,N_APU_0_W,N_GPU_0_R,N_GPU_0_W,N_APU_1_R,N_APU_1_W,N_GPU_1_R,N_GPU_1_W,S_APU_0_R,S_APU_0_W,S_GPU_0_R,S_GPU_0_W,S_APU_1_R,S_APU_1_W,S_GPU_1_R,S_GPU_1_W")
