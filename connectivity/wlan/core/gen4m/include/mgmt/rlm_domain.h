/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_domain.h"
 *    \brief
 */


#ifndef _RLM_DOMAIN_H
#define _RLM_DOMAIN_H

/*******************************************************************************
 *       C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *  E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
 #include "wsys_cmd_handler_fw.h"

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
#include "rlm_txpwr_limit_emi.h"
#else
#include "rlm_txpwr_limit.h"
#endif
/*******************************************************************************
 *   C O N S T A N T S
 *******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define MAX_SUBBAND_NUM     7
#else
#define MAX_SUBBAND_NUM     6
#endif
#define MAX_SUBBAND_NUM_5G  8

#define COUNTRY_CODE_NULL      ((uint16_t)0x0)

/* ISO/IEC 3166-1 two-character country codes */

/* Andorra */
#define COUNTRY_CODE_AD (((uint16_t) 'A' << 8) | (uint16_t) 'D')
/* UAE */
#define COUNTRY_CODE_AE (((uint16_t) 'A' << 8) | (uint16_t) 'E')
/* Afghanistan */
#define COUNTRY_CODE_AF (((uint16_t) 'A' << 8) | (uint16_t) 'F')
/* Antigua & Barbuda */
#define COUNTRY_CODE_AG (((uint16_t) 'A' << 8) | (uint16_t) 'G')
/* Anguilla */
#define COUNTRY_CODE_AI (((uint16_t) 'A' << 8) | (uint16_t) 'I')
/* Albania */
#define COUNTRY_CODE_AL (((uint16_t) 'A' << 8) | (uint16_t) 'L')
/* Armenia */
#define COUNTRY_CODE_AM (((uint16_t) 'A' << 8) | (uint16_t) 'M')
/* Netherlands Antilles */
#define COUNTRY_CODE_AN (((uint16_t) 'A' << 8) | (uint16_t) 'N')
/* Angola */
#define COUNTRY_CODE_AO (((uint16_t) 'A' << 8) | (uint16_t) 'O')
/*South Pole*/
#define COUNTRY_CODE_AQ (((uint16_t) 'A' << 8) | (uint16_t) 'Q')
/* Argentina */
#define COUNTRY_CODE_AR (((uint16_t) 'A' << 8) | (uint16_t) 'R')
/* American Samoa (USA) */
#define COUNTRY_CODE_AS (((uint16_t) 'A' << 8) | (uint16_t) 'S')
/* Austria */
#define COUNTRY_CODE_AT (((uint16_t) 'A' << 8) | (uint16_t) 'T')
/* Australia */
#define COUNTRY_CODE_AU (((uint16_t) 'A' << 8) | (uint16_t) 'U')
/* Aruba */
#define COUNTRY_CODE_AW (((uint16_t) 'A' << 8) | (uint16_t) 'W')
/* Aland Islands */
#define COUNTRY_CODE_AX (((uint16_t) 'A' << 8) | (uint16_t) 'X')
/* Azerbaijan */
#define COUNTRY_CODE_AZ (((uint16_t) 'A' << 8) | (uint16_t) 'Z')
/* Bosnia and Herzegovina */
#define COUNTRY_CODE_BA (((uint16_t) 'B' << 8) | (uint16_t) 'A')
/* Barbados */
#define COUNTRY_CODE_BB (((uint16_t) 'B' << 8) | (uint16_t) 'B')
/* Bangladesh */
#define COUNTRY_CODE_BD (((uint16_t) 'B' << 8) | (uint16_t) 'D')
/* Belgium  */
#define COUNTRY_CODE_BE (((uint16_t) 'B' << 8) | (uint16_t) 'E')
/* Burkina Faso */
#define COUNTRY_CODE_BF (((uint16_t) 'B' << 8) | (uint16_t) 'F')
/* Bulgaria */
#define COUNTRY_CODE_BG (((uint16_t) 'B' << 8) | (uint16_t) 'G')
/* Bahrain */
#define COUNTRY_CODE_BH (((uint16_t) 'B' << 8) | (uint16_t) 'H')
/* Burundi */
#define COUNTRY_CODE_BI (((uint16_t) 'B' << 8) | (uint16_t) 'I')
/* Benin */
#define COUNTRY_CODE_BJ (((uint16_t) 'B' << 8) | (uint16_t) 'J')
/* Bermuda */
#define COUNTRY_CODE_BM (((uint16_t) 'B' << 8) | (uint16_t) 'M')
/* Brunei */
#define COUNTRY_CODE_BN (((uint16_t) 'B' << 8) | (uint16_t) 'N')
/* Bolivia */
#define COUNTRY_CODE_BO (((uint16_t) 'B' << 8) | (uint16_t) 'O')
/* Brazil */
#define COUNTRY_CODE_BR (((uint16_t) 'B' << 8) | (uint16_t) 'R')
/* Bahamas  */
#define COUNTRY_CODE_BS (((uint16_t) 'B' << 8) | (uint16_t) 'S')
/* Bhutan */
#define COUNTRY_CODE_BT (((uint16_t) 'B' << 8) | (uint16_t) 'T')
/*Bouvet Island*/
#define COUNTRY_CODE_BV (((uint16_t) 'B' << 8) | (uint16_t) 'V')
/* Botswana */
#define COUNTRY_CODE_BW (((uint16_t) 'B' << 8) | (uint16_t) 'W')
/* Belarus */
#define COUNTRY_CODE_BY (((uint16_t) 'B' << 8) | (uint16_t) 'Y')
/* Belize */
#define COUNTRY_CODE_BZ (((uint16_t) 'B' << 8) | (uint16_t) 'Z')
/* Canada */
#define COUNTRY_CODE_CA (((uint16_t) 'C' << 8) | (uint16_t) 'A')
 /* Cocos Islands */
#define COUNTRY_CODE_CC (((uint16_t) 'C' << 8) | (uint16_t) 'C')
/* Democratic Republic of the Congo */
#define COUNTRY_CODE_CD (((uint16_t) 'C' << 8) | (uint16_t) 'D')
/* Central African Republic */
#define COUNTRY_CODE_CF (((uint16_t) 'C' << 8) | (uint16_t) 'F')
/* Republic of the Congo */
#define COUNTRY_CODE_CG (((uint16_t) 'C' << 8) | (uint16_t) 'G')
/* Switzerland */
#define COUNTRY_CODE_CH (((uint16_t) 'C' << 8) | (uint16_t) 'H')
/* Cote d'lvoire */
#define COUNTRY_CODE_CI (((uint16_t) 'C' << 8) | (uint16_t) 'I')
/* Cook Island */
#define COUNTRY_CODE_CK (((uint16_t) 'C' << 8) | (uint16_t) 'K')
/* Chile */
#define COUNTRY_CODE_CL (((uint16_t) 'C' << 8) | (uint16_t) 'L')
/* Cameroon */
#define COUNTRY_CODE_CM (((uint16_t) 'C' << 8) | (uint16_t) 'M')
/* China */
#define COUNTRY_CODE_CN (((uint16_t) 'C' << 8) | (uint16_t) 'N')
/* Columbia */
#define COUNTRY_CODE_CO (((uint16_t) 'C' << 8) | (uint16_t) 'O')
/* Costa Rica */
#define COUNTRY_CODE_CR (((uint16_t) 'C' << 8) | (uint16_t) 'R')
/* Cuba */
#define COUNTRY_CODE_CU (((uint16_t) 'C' << 8) | (uint16_t) 'U')
/* Cape Verde */
#define COUNTRY_CODE_CV (((uint16_t) 'C' << 8) | (uint16_t) 'V')
/* "Curacao */
#define COUNTRY_CODE_CW (((uint16_t) 'C' << 8) | (uint16_t) 'W')
/* "Christmas Island(Australia) */
#define COUNTRY_CODE_CX (((uint16_t) 'C' << 8) | (uint16_t) 'X')
/* Cyprus */
#define COUNTRY_CODE_CY (((uint16_t) 'C' << 8) | (uint16_t) 'Y')
/* Czech */
#define COUNTRY_CODE_CZ (((uint16_t) 'C' << 8) | (uint16_t) 'Z')
/* Germany */
#define COUNTRY_CODE_DE (((uint16_t) 'D' << 8) | (uint16_t) 'E')
/* Djibouti */
#define COUNTRY_CODE_DJ (((uint16_t) 'D' << 8) | (uint16_t) 'J')
/* Denmark */
#define COUNTRY_CODE_DK (((uint16_t) 'D' << 8) | (uint16_t) 'K')
/* Dominica */
#define COUNTRY_CODE_DM (((uint16_t) 'D' << 8) | (uint16_t) 'M')
/* Dominican Republic */
#define COUNTRY_CODE_DO (((uint16_t) 'D' << 8) | (uint16_t) 'O')
/* Algeria */
#define COUNTRY_CODE_DZ (((uint16_t) 'D' << 8) | (uint16_t) 'Z')
/* Ecuador */
#define COUNTRY_CODE_EC (((uint16_t) 'E' << 8) | (uint16_t) 'C')
/* Estonia */
#define COUNTRY_CODE_EE (((uint16_t) 'E' << 8) | (uint16_t) 'E')
/* Egypt */
#define COUNTRY_CODE_EG (((uint16_t) 'E' << 8) | (uint16_t) 'G')
/* Western Sahara (Morocco) */
#define COUNTRY_CODE_EH (((uint16_t) 'E' << 8) | (uint16_t) 'H')
/* Eritrea */
#define COUNTRY_CODE_ER (((uint16_t) 'E' << 8) | (uint16_t) 'R')
/* Spain */
#define COUNTRY_CODE_ES (((uint16_t) 'E' << 8) | (uint16_t) 'S')
/* Ethiopia */
#define COUNTRY_CODE_ET (((uint16_t) 'E' << 8) | (uint16_t) 'T')
/* Europe */
#define COUNTRY_CODE_EU (((uint16_t) 'E' << 8) | (uint16_t) 'U')
/* Finland */
#define COUNTRY_CODE_FI (((uint16_t) 'F' << 8) | (uint16_t) 'I')
/* Fiji */
#define COUNTRY_CODE_FJ (((uint16_t) 'F' << 8) | (uint16_t) 'J')
/* Falkland Island */
#define COUNTRY_CODE_FK (((uint16_t) 'F' << 8) | (uint16_t) 'K')
/* Micronesia */
#define COUNTRY_CODE_FM (((uint16_t) 'F' << 8) | (uint16_t) 'M')
/* Faroe Island */
#define COUNTRY_CODE_FO (((uint16_t) 'F' << 8) | (uint16_t) 'O')
/* France */
#define COUNTRY_CODE_FR (((uint16_t) 'F' << 8) | (uint16_t) 'R')
/* Wallis and Futuna (France) */
#define COUNTRY_CODE_FR (((uint16_t) 'F' << 8) | (uint16_t) 'R')
/* Gabon */
#define COUNTRY_CODE_GA (((uint16_t) 'G' << 8) | (uint16_t) 'A')
/* United Kingdom */
#define COUNTRY_CODE_GB (((uint16_t) 'G' << 8) | (uint16_t) 'B')
/* Grenada */
#define COUNTRY_CODE_GD (((uint16_t) 'G' << 8) | (uint16_t) 'D')
/* Georgia */
#define COUNTRY_CODE_GE (((uint16_t) 'G' << 8) | (uint16_t) 'E')
/* French Guiana */
#define COUNTRY_CODE_GF (((uint16_t) 'G' << 8) | (uint16_t) 'F')
/* Guernsey */
#define COUNTRY_CODE_GG (((uint16_t) 'G' << 8) | (uint16_t) 'G')
/* Ghana */
#define COUNTRY_CODE_GH (((uint16_t) 'G' << 8) | (uint16_t) 'H')
/* Gibraltar */
#define COUNTRY_CODE_GI (((uint16_t) 'G' << 8) | (uint16_t) 'I')
/* Greenland */
#define COUNTRY_CODE_GL (((uint16_t) 'G' << 8) | (uint16_t) 'L')
/* Gambia */
#define COUNTRY_CODE_GM (((uint16_t) 'G' << 8) | (uint16_t) 'M')
/* Guinea */
#define COUNTRY_CODE_GN (((uint16_t) 'G' << 8) | (uint16_t) 'N')
/* Guadeloupe */
#define COUNTRY_CODE_GP (((uint16_t) 'G' << 8) | (uint16_t) 'P')
/* Equatorial Guinea */
#define COUNTRY_CODE_GQ (((uint16_t) 'G' << 8) | (uint16_t) 'Q')
/* Greece */
#define COUNTRY_CODE_GR (((uint16_t) 'G' << 8) | (uint16_t) 'R')
/* South Georgia and the South Sandwich Islands */
#define COUNTRY_CODE_GS (((uint16_t) 'G' << 8) | (uint16_t) 'S')
/* Guatemala */
#define COUNTRY_CODE_GT (((uint16_t) 'G' << 8) | (uint16_t) 'T')
/* Guam */
#define COUNTRY_CODE_GU (((uint16_t) 'G' << 8) | (uint16_t) 'U')
/* Guinea-Bissau */
#define COUNTRY_CODE_GW (((uint16_t) 'G' << 8) | (uint16_t) 'W')
/* Guyana */
#define COUNTRY_CODE_GY (((uint16_t) 'G' << 8) | (uint16_t) 'Y')
/* Hong Kong */
#define COUNTRY_CODE_HK (((uint16_t) 'H' << 8) | (uint16_t) 'K')
/* Heard and McDonald Islands */
#define COUNTRY_CODE_HM (((uint16_t) 'H' << 8) | (uint16_t) 'M')
/* Honduras */
#define COUNTRY_CODE_HN (((uint16_t) 'H' << 8) | (uint16_t) 'N')
/* Croatia */
#define COUNTRY_CODE_HR (((uint16_t) 'H' << 8) | (uint16_t) 'R')
/* Haiti */
#define COUNTRY_CODE_HT (((uint16_t) 'H' << 8) | (uint16_t) 'T')
/* Hungary */
#define COUNTRY_CODE_HU (((uint16_t) 'H' << 8) | (uint16_t) 'U')
/* Indonesia */
#define COUNTRY_CODE_ID (((uint16_t) 'I' << 8) | (uint16_t) 'D')
/* Ireland */
#define COUNTRY_CODE_IE (((uint16_t) 'I' << 8) | (uint16_t) 'E')
/* Israel */
#define COUNTRY_CODE_IL (((uint16_t) 'I' << 8) | (uint16_t) 'L')
/* Isle of Man */
#define COUNTRY_CODE_IM (((uint16_t) 'I' << 8) | (uint16_t) 'M')
/* India */
#define COUNTRY_CODE_IN (((uint16_t) 'I' << 8) | (uint16_t) 'N')
/* British Indian Ocean Territory */
#define COUNTRY_CODE_IO (((uint16_t) 'I' << 8) | (uint16_t) 'O')
/* Iraq */
#define COUNTRY_CODE_IQ (((uint16_t) 'I' << 8) | (uint16_t) 'Q')
/* Iran */
#define COUNTRY_CODE_IR (((uint16_t) 'I' << 8) | (uint16_t) 'R')
/* Iceland */
#define COUNTRY_CODE_IS (((uint16_t) 'I' << 8) | (uint16_t) 'S')
/* Italy */
#define COUNTRY_CODE_IT (((uint16_t) 'I' << 8) | (uint16_t) 'T')
/* Jersey */
#define COUNTRY_CODE_JE (((uint16_t) 'J' << 8) | (uint16_t) 'E')
/* Jameica */
#define COUNTRY_CODE_JM (((uint16_t) 'J' << 8) | (uint16_t) 'M')
/* Jordan */
#define COUNTRY_CODE_JO (((uint16_t) 'J' << 8) | (uint16_t) 'O')
/* Japan */
#define COUNTRY_CODE_JP (((uint16_t) 'J' << 8) | (uint16_t) 'P')
/* Kenya */
#define COUNTRY_CODE_KE (((uint16_t) 'K' << 8) | (uint16_t) 'E')
/* Kyrgyzstan */
#define COUNTRY_CODE_KG (((uint16_t) 'K' << 8) | (uint16_t) 'G')
/* Cambodia */
#define COUNTRY_CODE_KH (((uint16_t) 'K' << 8) | (uint16_t) 'H')
/* Kiribati */
#define COUNTRY_CODE_KI (((uint16_t) 'K' << 8) | (uint16_t) 'I')
/* Comoros */
#define COUNTRY_CODE_KM (((uint16_t) 'K' << 8) | (uint16_t) 'M')
/* Saint Kitts and Nevis */
#define COUNTRY_CODE_KN (((uint16_t) 'K' << 8) | (uint16_t) 'N')
/* North Korea */
#define COUNTRY_CODE_KP (((uint16_t) 'K' << 8) | (uint16_t) 'P')
/* South Korea */
#define COUNTRY_CODE_KR (((uint16_t) 'K' << 8) | (uint16_t) 'R')
/* Kuwait */
#define COUNTRY_CODE_KW (((uint16_t) 'K' << 8) | (uint16_t) 'W')
/* Cayman Islands */
#define COUNTRY_CODE_KY (((uint16_t) 'K' << 8) | (uint16_t) 'Y')
/* Kazakhstan */
#define COUNTRY_CODE_KZ (((uint16_t) 'K' << 8) | (uint16_t) 'Z')
/* Laos */
#define COUNTRY_CODE_LA (((uint16_t) 'L' << 8) | (uint16_t) 'A')
/* Lebanon */
#define COUNTRY_CODE_LB (((uint16_t) 'L' << 8) | (uint16_t) 'B')
/* Saint Lucia */
#define COUNTRY_CODE_LC (((uint16_t) 'L' << 8) | (uint16_t) 'C')
/* Liechtenstein */
#define COUNTRY_CODE_LI (((uint16_t) 'L' << 8) | (uint16_t) 'I')
/* Sri Lanka */
#define COUNTRY_CODE_LK (((uint16_t) 'L' << 8) | (uint16_t) 'K')
/* Liberia */
#define COUNTRY_CODE_LR (((uint16_t) 'L' << 8) | (uint16_t) 'R')
/* Lesotho */
#define COUNTRY_CODE_LS (((uint16_t) 'L' << 8) | (uint16_t) 'S')
/* Lithuania */
#define COUNTRY_CODE_LT (((uint16_t) 'L' << 8) | (uint16_t) 'T')
/* Luxemburg */
#define COUNTRY_CODE_LU (((uint16_t) 'L' << 8) | (uint16_t) 'U')
/* Latvia */
#define COUNTRY_CODE_LV (((uint16_t) 'L' << 8) | (uint16_t) 'V')
/* Libya */
#define COUNTRY_CODE_LY (((uint16_t) 'L' << 8) | (uint16_t) 'Y')
/* Morocco */
#define COUNTRY_CODE_MA (((uint16_t) 'M' << 8) | (uint16_t) 'A')
/* Monaco */
#define COUNTRY_CODE_MC (((uint16_t) 'M' << 8) | (uint16_t) 'C')
/* Moldova */
#define COUNTRY_CODE_MD (((uint16_t) 'M' << 8) | (uint16_t) 'D')
/* Montenegro */
#define COUNTRY_CODE_ME (((uint16_t) 'M' << 8) | (uint16_t) 'E')
/* Saint Martin / Sint Marteen (Added on window's list) */
#define COUNTRY_CODE_MF (((uint16_t) 'M' << 8) | (uint16_t) 'F')
/* Madagascar */
#define COUNTRY_CODE_MG (((uint16_t) 'M' << 8) | (uint16_t) 'G')
/* Marshall Islands */
#define COUNTRY_CODE_MH (((uint16_t) 'M' << 8) | (uint16_t) 'H')
/* Macedonia */
#define COUNTRY_CODE_MK (((uint16_t) 'M' << 8) | (uint16_t) 'K')
/* Mali */
#define COUNTRY_CODE_ML (((uint16_t) 'M' << 8) | (uint16_t) 'L')
/* Myanmar */
#define COUNTRY_CODE_MM (((uint16_t) 'M' << 8) | (uint16_t) 'M')
/* Mongolia */
#define COUNTRY_CODE_MN (((uint16_t) 'M' << 8) | (uint16_t) 'N')
/* Macao */
#define COUNTRY_CODE_MO (((uint16_t) 'M' << 8) | (uint16_t) 'O')
/* Northern Mariana Islands (Rota Island Saipan and Tinian Island) */
#define COUNTRY_CODE_MP (((uint16_t) 'M' << 8) | (uint16_t) 'P')
/* Martinique (France) */
#define COUNTRY_CODE_MQ (((uint16_t) 'M' << 8) | (uint16_t) 'Q')
/* Mauritania */
#define COUNTRY_CODE_MR (((uint16_t) 'M' << 8) | (uint16_t) 'R')
/* Montserrat (UK) */
#define COUNTRY_CODE_MS (((uint16_t) 'M' << 8) | (uint16_t) 'S')
/* Malta */
#define COUNTRY_CODE_MT (((uint16_t) 'M' << 8) | (uint16_t) 'T')
/* Mauritius */
#define COUNTRY_CODE_MU (((uint16_t) 'M' << 8) | (uint16_t) 'U')
/* Maldives */
#define COUNTRY_CODE_MV (((uint16_t) 'M' << 8) | (uint16_t) 'V')
/* Malawi */
#define COUNTRY_CODE_MW (((uint16_t) 'M' << 8) | (uint16_t) 'W')
/* Mexico */
#define COUNTRY_CODE_MX (((uint16_t) 'M' << 8) | (uint16_t) 'X')
/* Malaysia */
#define COUNTRY_CODE_MY (((uint16_t) 'M' << 8) | (uint16_t) 'Y')
/* Mozambique */
#define COUNTRY_CODE_MZ (((uint16_t) 'M' << 8) | (uint16_t) 'Z')
/* Namibia */
#define COUNTRY_CODE_NA (((uint16_t) 'N' << 8) | (uint16_t) 'A')
/* New Caledonia */
#define COUNTRY_CODE_NC (((uint16_t) 'N' << 8) | (uint16_t) 'C')
/* Niger */
#define COUNTRY_CODE_NE (((uint16_t) 'N' << 8) | (uint16_t) 'E')
/* Norfolk Island */
#define COUNTRY_CODE_NF (((uint16_t) 'N' << 8) | (uint16_t) 'F')
/* Nigeria */
#define COUNTRY_CODE_NG (((uint16_t) 'N' << 8) | (uint16_t) 'G')
/* Nicaragua */
#define COUNTRY_CODE_NI (((uint16_t) 'N' << 8) | (uint16_t) 'I')
/* Netherlands */
#define COUNTRY_CODE_NL (((uint16_t) 'N' << 8) | (uint16_t) 'L')
/* Norway */
#define COUNTRY_CODE_NO (((uint16_t) 'N' << 8) | (uint16_t) 'O')
/* Nepal */
#define COUNTRY_CODE_NP (((uint16_t) 'N' << 8) | (uint16_t) 'P')
/* Nauru */
#define COUNTRY_CODE_NR (((uint16_t) 'N' << 8) | (uint16_t) 'R')
/* Niue */
#define COUNTRY_CODE_NU (((uint16_t) 'N' << 8) | (uint16_t) 'U')
/* New Zealand */
#define COUNTRY_CODE_NZ (((uint16_t) 'N' << 8) | (uint16_t) 'Z')
/* Oman */
#define COUNTRY_CODE_OM (((uint16_t) 'O' << 8) | (uint16_t) 'M')
/* Panama */
#define COUNTRY_CODE_PA (((uint16_t) 'P' << 8) | (uint16_t) 'A')
/* Peru */
#define COUNTRY_CODE_PE (((uint16_t) 'P' << 8) | (uint16_t) 'E')
/* "French Polynesia */
#define COUNTRY_CODE_PF (((uint16_t) 'P' << 8) | (uint16_t) 'F')
/* Papua New Guinea */
#define COUNTRY_CODE_PG (((uint16_t) 'P' << 8) | (uint16_t) 'G')
/* Philippines */
#define COUNTRY_CODE_PH (((uint16_t) 'P' << 8) | (uint16_t) 'H')
/* Pakistan */
#define COUNTRY_CODE_PK (((uint16_t) 'P' << 8) | (uint16_t) 'K')
/* Poland */
#define COUNTRY_CODE_PL (((uint16_t) 'P' << 8) | (uint16_t) 'L')
/* Saint Pierre and Miquelon */
#define COUNTRY_CODE_PM (((uint16_t) 'P' << 8) | (uint16_t) 'M')
/* Pitcairn Islands  */
#define COUNTRY_CODE_PN (((uint16_t) 'P' << 8) | (uint16_t) 'N')
/* Puerto Rico (USA) */
#define COUNTRY_CODE_PR (((uint16_t) 'P' << 8) | (uint16_t) 'R')
/* Palestinian Authority */
#define COUNTRY_CODE_PS (((uint16_t) 'P' << 8) | (uint16_t) 'S')
/* Portugal */
#define COUNTRY_CODE_PT (((uint16_t) 'P' << 8) | (uint16_t) 'T')
/* Palau */
#define COUNTRY_CODE_PW (((uint16_t) 'P' << 8) | (uint16_t) 'W')
/* Paraguay */
#define COUNTRY_CODE_PY (((uint16_t) 'P' << 8) | (uint16_t) 'Y')
/* Qatar */
#define COUNTRY_CODE_QA (((uint16_t) 'Q' << 8) | (uint16_t) 'A')
/* Reunion (France) */
#define COUNTRY_CODE_RE (((uint16_t) 'R' << 8) | (uint16_t) 'E')
/* Kosvo (Added on window's list) */
#define COUNTRY_CODE_RKS (((uint16_t) 'R' << 8) | (uint16_t) 'K')
/* Romania */
#define COUNTRY_CODE_RO (((uint16_t) 'R' << 8) | (uint16_t) 'O')
/* Serbia */
#define COUNTRY_CODE_RS (((uint16_t) 'R' << 8) | (uint16_t) 'S')
/* Russia */
#define COUNTRY_CODE_RU (((uint16_t) 'R' << 8) | (uint16_t) 'U')
/* Rwanda */
#define COUNTRY_CODE_RW (((uint16_t) 'R' << 8) | (uint16_t) 'W')
/* Saudi Arabia */
#define COUNTRY_CODE_SA (((uint16_t) 'S' << 8) | (uint16_t) 'A')
/* Solomon Islands */
#define COUNTRY_CODE_SB (((uint16_t) 'S' << 8) | (uint16_t) 'B')
/* Seychelles */
#define COUNTRY_CODE_SC (((uint16_t) 'S' << 8) | (uint16_t) 'C')
/* Sudan */
#define COUNTRY_CODE_SD (((uint16_t) 'S' << 8) | (uint16_t) 'D')
/* Sweden */
#define COUNTRY_CODE_SE (((uint16_t) 'S' << 8) | (uint16_t) 'E')
/* Singapole */
#define COUNTRY_CODE_SG (((uint16_t) 'S' << 8) | (uint16_t) 'G')
/* Saint Helena */
#define COUNTRY_CODE_SH (((uint16_t) 'S' << 8) | (uint16_t) 'H')
/* Slovenia */
#define COUNTRY_CODE_SI (((uint16_t) 'S' << 8) | (uint16_t) 'I')
/* Svalbard and Jan Mayen */
#define COUNTRY_CODE_SJ (((uint16_t) 'S' << 8) | (uint16_t) 'J')
/* Slovakia */
#define COUNTRY_CODE_SK (((uint16_t) 'S' << 8) | (uint16_t) 'K')
/* Sierra Leone */
#define COUNTRY_CODE_SL (((uint16_t) 'S' << 8) | (uint16_t) 'L')
/* San Marino */
#define COUNTRY_CODE_SM (((uint16_t) 'S' << 8) | (uint16_t) 'M')
/* Senegal */
#define COUNTRY_CODE_SN (((uint16_t) 'S' << 8) | (uint16_t) 'N')
/* Somalia */
#define COUNTRY_CODE_SO (((uint16_t) 'S' << 8) | (uint16_t) 'O')
/* Suriname */
#define COUNTRY_CODE_SR (((uint16_t) 'S' << 8) | (uint16_t) 'R')
/* South_Sudan */
#define COUNTRY_CODE_SS (((uint16_t) 'S' << 8) | (uint16_t) 'S')
/* Sao Tome and Principe */
#define COUNTRY_CODE_ST (((uint16_t) 'S' << 8) | (uint16_t) 'T')
/* El Salvador */
#define COUNTRY_CODE_SV (((uint16_t) 'S' << 8) | (uint16_t) 'V')
/* Sint Maarten */
#define COUNTRY_CODE_SX (((uint16_t) 'S' << 8) | (uint16_t) 'X')
/* Syria */
#define COUNTRY_CODE_SY (((uint16_t) 'S' << 8) | (uint16_t) 'Y')
/* Swaziland */
#define COUNTRY_CODE_SZ (((uint16_t) 'S' << 8) | (uint16_t) 'Z')
/* Turks and Caicos Islands (UK) */
#define COUNTRY_CODE_TC (((uint16_t) 'T' << 8) | (uint16_t) 'C')
/* Chad */
#define COUNTRY_CODE_TD (((uint16_t) 'T' << 8) | (uint16_t) 'D')
/* French Southern and Antarctic Lands */
#define COUNTRY_CODE_TF (((uint16_t) 'T' << 8) | (uint16_t) 'F')
/* Togo */
#define COUNTRY_CODE_TG (((uint16_t) 'T' << 8) | (uint16_t) 'G')
/* Thailand */
#define COUNTRY_CODE_TH (((uint16_t) 'T' << 8) | (uint16_t) 'H')
/* Tajikistan */
#define COUNTRY_CODE_TJ (((uint16_t) 'T' << 8) | (uint16_t) 'J')
/* Tokelau */
#define COUNTRY_CODE_TK (((uint16_t) 'T' << 8) | (uint16_t) 'K')
/* East Timor */
#define COUNTRY_CODE_TL (((uint16_t) 'T' << 8) | (uint16_t) 'L')
/* Turkmenistan */
#define COUNTRY_CODE_TM (((uint16_t) 'T' << 8) | (uint16_t) 'M')
/* Tunisia */
#define COUNTRY_CODE_TN (((uint16_t) 'T' << 8) | (uint16_t) 'N')
/* Tonga */
#define COUNTRY_CODE_TO (((uint16_t) 'T' << 8) | (uint16_t) 'O')
/* Turkey */
#define COUNTRY_CODE_TR (((uint16_t) 'T' << 8) | (uint16_t) 'R')
/* Trinidad and Tobago */
#define COUNTRY_CODE_TT (((uint16_t) 'T' << 8) | (uint16_t) 'T')
/* Tuvalu */
#define COUNTRY_CODE_TV (((uint16_t) 'T' << 8) | (uint16_t) 'V')
/* Taiwan */
#define COUNTRY_CODE_TW (((uint16_t) 'T' << 8) | (uint16_t) 'W')
/* Tanzania */
#define COUNTRY_CODE_TZ (((uint16_t) 'T' << 8) | (uint16_t) 'Z')
/* Ukraine */
#define COUNTRY_CODE_UA (((uint16_t) 'U' << 8) | (uint16_t) 'A')
/* Ugnada */
#define COUNTRY_CODE_UG (((uint16_t) 'U' << 8) | (uint16_t) 'G')
/* United States Minor Outlying Islands */
#define COUNTRY_CODE_UM (((uint16_t) 'U' << 8) | (uint16_t) 'M')
/* US */
#define COUNTRY_CODE_US (((uint16_t) 'U' << 8) | (uint16_t) 'S')
/* Uruguay */
#define COUNTRY_CODE_UY (((uint16_t) 'U' << 8) | (uint16_t) 'Y')
/* Uzbekistan */
#define COUNTRY_CODE_UZ (((uint16_t) 'U' << 8) | (uint16_t) 'Z')
/* Vatican (Holy See) */
#define COUNTRY_CODE_VA (((uint16_t) 'V' << 8) | (uint16_t) 'A')
/* Saint Vincent and the Grenadines */
#define COUNTRY_CODE_VC (((uint16_t) 'V' << 8) | (uint16_t) 'C')
/* Venezuela */
#define COUNTRY_CODE_VE (((uint16_t) 'V' << 8) | (uint16_t) 'E')
/* British Virgin Islands */
#define COUNTRY_CODE_VG (((uint16_t) 'V' << 8) | (uint16_t) 'G')
/* US Virgin Islands */
#define COUNTRY_CODE_VI (((uint16_t) 'V' << 8) | (uint16_t) 'I')
/* Vietnam */
#define COUNTRY_CODE_VN (((uint16_t) 'V' << 8) | (uint16_t) 'N')
/* Vanuatu */
#define COUNTRY_CODE_VU (((uint16_t) 'V' << 8) | (uint16_t) 'U')
/* Wallis and Futuna */
#define COUNTRY_CODE_WF (((uint16_t) 'W' << 8) | (uint16_t) 'F')
/* Samoa */
#define COUNTRY_CODE_WS (((uint16_t) 'W' << 8) | (uint16_t) 'S')
/* Republic of Kosovo */
#define COUNTRY_CODE_XK (((uint16_t) 'X' << 8) | (uint16_t) 'K')
/* Yemen */
#define COUNTRY_CODE_YE (((uint16_t) 'Y' << 8) | (uint16_t) 'E')
/* Mayotte (France) */
#define COUNTRY_CODE_YT (((uint16_t) 'Y' << 8) | (uint16_t) 'T')
/* South Africa */
#define COUNTRY_CODE_ZA (((uint16_t) 'Z' << 8) | (uint16_t) 'A')
/* Zambia */
#define COUNTRY_CODE_ZM (((uint16_t) 'Z' << 8) | (uint16_t) 'M')
/* Zimbabwe */
#define COUNTRY_CODE_ZW (((uint16_t) 'Z' << 8) | (uint16_t) 'W')
/* Default country domain */
#define COUNTRY_CODE_DF (((uint16_t) 'D' << 8) | (uint16_t) 'F')
/* World Wide */
#define COUNTRY_CODE_WW (((uint16_t) '0' << 8) | (uint16_t) '0')


/* dot11RegDomainsSupportValue */
#define MIB_REG_DOMAIN_FCC     0x10	/* FCC (US) */
#define MIB_REG_DOMAIN_IC      0x20	/* IC or DOC (Canada) */
#define MIB_REG_DOMAIN_ETSI    0x30	/* ETSI (Europe) */
#define MIB_REG_DOMAIN_SPAIN   0x31	/* Spain */
#define MIB_REG_DOMAIN_FRANCE  0x32	/* France */
#define MIB_REG_DOMAIN_JAPAN   0x40	/* MPHPT (Japan) */
#define MIB_REG_DOMAIN_OTHER   0x00	/* other */

/*2.4G*/
#define BAND_2G4_LOWER_BOUND 1
#define BAND_2G4_UPPER_BOUND 14
/*5G SubBand FCC spec*/
#define UNII1_LOWER_BOUND    36
#define UNII1_UPPER_BOUND    50
#define UNII2A_LOWER_BOUND   52
#define UNII2A_UPPER_BOUND   64
#define UNII2C_LOWER_BOUND   100
#define UNII2C_UPPER_BOUND   144
#define UNII3_LOWER_BOUND    149
#define UNII3_UPPER_BOUND    165
/*6G SubBand spec*/
#define UNII5A_LOWER_BOUND   1
#define UNII5A_UPPER_BOUND   47
#define UNII5B_LOWER_BOUND   49
#define UNII5B_UPPER_BOUND   93
#define UNII6_LOWER_BOUND    95
#define UNII6_UPPER_BOUND    115
#define UNII7A_LOWER_BOUND   117
#define UNII7A_UPPER_BOUND   151
#define UNII7B_LOWER_BOUND   153
#define UNII7B_UPPER_BOUND   185
#define UNII8_LOWER_BOUND    187
#define UNII8_UPPER_BOUND    233

#define MAX_COUNTRY_CODE_LEN 4


#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
#define POWER_LIMIT_TABLE_NULL			0xFFFF
#if (CFG_SUPPORT_CONNAC2X == 1) || (CFG_SUPPORT_CONNAC3X == 1)
#define MAX_TX_POWER				127
#define MIN_TX_POWER				-128
#else
#define MAX_TX_POWER				63
#define MIN_TX_POWER				-64
#endif
/*align Frimware Max Power Limit CH Num*/
#define MAX_CMD_SUPPORT_CHANNEL_NUM			61
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define MAX_CMD_EHT_SUPPORT_CHANNEL_NUM	 32
#if (CFG_SUPPORT_WIFI_6G == 1)
#define MAX_CMD_EHT_6G_SUPPORT_CHANNEL_NUM	 29
#endif /* CFG_SUPPORT_WIFI_6G && CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_PWR_LIMIT_COUNTRY */

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/* ARRAY_SIZE(mtk_2ghz_channels) + ARRAY_SIZE(mtk_5ghz_channels) */
#define MAX_SUPPORTED_CH_COUNT (MAX_CHN_NUM)
#define REG_RULE_LIGHT(start, end, bw, reg_flags)	\
		REG_RULE(start, end, bw, 0, 0, reg_flags)
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
#define SUBBAND_6G_NUM                4/* UNII-5/6/7/8 */
#endif
/*******************************************************************************
 *  D A T A   T Y P E S
 *******************************************************************************
 */

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

/* Define Tx Power Control Channel Type */
#define MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE	16
#define PWR_CTRL_CHNL_TYPE_KEY_ALL		"ALL"
#define PWR_CTRL_CHNL_TYPE_KEY_2G4		"2G4"
#define PWR_CTRL_CHNL_TYPE_KEY_5G		"5G"
#define PWR_CTRL_CHNL_TYPE_KEY_BANDEDGE_2G4	"BANDEDGE2G4"
#define PWR_CTRL_CHNL_TYPE_KEY_BANDEDGE_5G	"BANDEDGE5G"
#define PWR_CTRL_CHNL_TYPE_KEY_5G_BAND1		"5GBAND1"
#define PWR_CTRL_CHNL_TYPE_KEY_5G_BAND2		"5GBAND2"
#define PWR_CTRL_CHNL_TYPE_KEY_5G_BAND3		"5GBAND3"
#define PWR_CTRL_CHNL_TYPE_KEY_5G_BAND4		"5GBAND4"
#if (CFG_SUPPORT_WIFI_6G == 1)
#define PWR_CTRL_CHNL_TYPE_KEY_6G		"6G"
#define PWR_CTRL_CHNL_TYPE_KEY_6G_BAND1		"6GBAND1"
#define PWR_CTRL_CHNL_TYPE_KEY_6G_BAND2		"6GBAND2"
#define PWR_CTRL_CHNL_TYPE_KEY_6G_BAND3		"6GBAND3"
#define PWR_CTRL_CHNL_TYPE_KEY_6G_BAND4		"6GBAND4"
#endif /* CFG_SUPPORT_WIFI_6G */

/* Dynamic Tx Power Control Rate Tag */
#define PWR_CTRL_RATE_TAG_KEY_LEGACY		"LEGACY"
#define PWR_CTRL_RATE_TAG_KEY_HE	        "HE"
/* For backward competible, same as HE*/
#define PWR_CTRL_RATE_TAG_KEY_AX160		"AX160"
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_CTRL_RATE_TAG_KEY_EHT		"EHT"
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
#define PWR_CTRL_RATE_TAG_KEY_LEGACY6G		"LEGACY6G"
#define PWR_CTRL_RATE_TAG_KEY_HE6G		"HE6G"
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_CTRL_RATE_TAG_KEY_EHT6G		"EHT6G"
#endif
#endif

#define PWR_CFG_PRAM_NUM_ALL_RATE	1

#define PWR_CFG_PRAM_NUM_AX		   18
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
#define	PWR_CFG_PRAM_NUM_AC			12
#else
#define	PWR_CFG_PRAM_NUM_AC			9
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */

#define PWR_CFG_BACKOFF_MIN		-64
#define PWR_CFG_BACKOFF_MAX		64
#define PWR_CFG_BACKOFF_INIT		0
#define PWR_CFG_ABS_INIT		MAX_TX_POWER

#define ANT_CFG_SUBBAND_NUM_SINGLE 1
#define ANT_CFG_SUBBAND_NUM_2G4 1
#define ANT_CFG_SUBBAND_NUM_5G 4
#define ANT_CFG_SUBBAND_NUM_6G 4

#define ANT_CFG_CHAIN_NUM_SINGLE 1
#define ANT_CFG_CHAIN_NUM_WF0 ANT_CFG_CHAIN_NUM_SINGLE
#define ANT_CFG_CHAIN_NUM_WF1 ANT_CFG_CHAIN_NUM_SINGLE
#define ANT_CFG_CHAIN_NUM_WF2 ANT_CFG_CHAIN_NUM_SINGLE

enum ENUM_TX_POWER_CTRL_LIST_TYPE {
	PWR_CTRL_TYPE_DEFAULT_LIST = 0,
	PWR_CTRL_TYPE_DYNAMIC_LIST,
	PWR_CTRL_TYPE_ALL_LIST,
};

enum ENUM_TX_POWER_CTRL_APPLIED_WAY {
	PWR_CTRL_TYPE_APPLIED_WAY_WIFION = 1,
	PWR_CTRL_TYPE_APPLIED_WAY_IOCTL,
};

enum ENUM_TX_POWER_CTRL_OPERATION {
	PWR_CTRL_TYPE_OPERATION_POWER_LEVEL = 1,
	PWR_CTRL_TYPE_OPERATION_POWER_OFFSET,
};

enum ENUM_TX_POWER_CTRL_TYPE {
	PWR_CTRL_TYPE_WIFION_POWER_LEVEL = 1,
	PWR_CTRL_TYPE_WIFION_POWER_OFFSET,
	PWR_CTRL_TYPE_IOCTL_POWER_LEVEL,
	PWR_CTRL_TYPE_IOCTL_POWER_OFFSET,
};

enum ENUM_TX_POWER_CTRL_VALUE_SIGN {
	PWR_CTRL_TYPE_NO_ACTION = 0,
	PWR_CTRL_TYPE_POSITIVE,
	PWR_CTRL_TYPE_NEGATIVE,
};

enum ENUM_TX_POWER_CTRL_CHANNEL_TYPE {
	PWR_CTRL_CHNL_TYPE_NORMAL = 0,
	PWR_CTRL_CHNL_TYPE_ALL,
	PWR_CTRL_CHNL_TYPE_RANGE,
	PWR_CTRL_CHNL_TYPE_2G4,
	PWR_CTRL_CHNL_TYPE_5G,
	PWR_CTRL_CHNL_TYPE_BANDEDGE_2G4,
	PWR_CTRL_CHNL_TYPE_BANDEDGE_5G,
	PWR_CTRL_CHNL_TYPE_5G_BAND1,
	PWR_CTRL_CHNL_TYPE_5G_BAND2,
	PWR_CTRL_CHNL_TYPE_5G_BAND3,
	PWR_CTRL_CHNL_TYPE_5G_BAND4,
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_CTRL_CHNL_TYPE_6G_NORMAL,
	PWR_CTRL_CHNL_TYPE_6G,
	PWR_CTRL_CHNL_TYPE_6G_BAND1,
	PWR_CTRL_CHNL_TYPE_6G_BAND2,
	PWR_CTRL_CHNL_TYPE_6G_BAND3,
	PWR_CTRL_CHNL_TYPE_6G_BAND4,
	PWR_CTRL_CHNL_TYPE_6G_RANGE,
#endif
};

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG

/* Revise channel power limit by scenario with parameter
 * (WF05G, WF02G4, WF15G, WF12G4)
 */
enum ENUM_POWER_ANT_TAG {
	POWER_ANT_ALL_T = 0,
	POWER_ANT_MIMO_1T,
	POWER_ANT_MIMO_2T,
	POWER_ANT_ALL_T_6G,
	POWER_ANT_CHAIN_COMP,
	POWER_ANT_CHAIN_ABS,
	POWER_ANT_TAG_NUM
};

/* For ALL_T/MIMO_1T/MIMO_2T */
enum ENUM_POWER_ANT_BAND {
	POWER_ANT_2G4_BAND = 0,
	POWER_ANT_5G_BAND1,
	POWER_ANT_5G_BAND2,
	POWER_ANT_5G_BAND3,
	POWER_ANT_5G_BAND4,
	POWER_ANT_BAND_NUM
};

/* For ALL_T_6G */
enum ENUM_POWER_ANT_6G_BAND {
	POWER_ANT_6G_BAND1 = 0,
	POWER_ANT_6G_BAND2,
	POWER_ANT_6G_BAND3,
	POWER_ANT_6G_BAND4,
	POWER_ANT_6G_BAND_NUM
};

/* For ALL_T/MIMO_1T/MIMO_2T/ALL_T_6G */
enum ENUM_POWER_ANT_PARA {
	POWER_ANT_WF0 = 0,
	POWER_ANT_WF1,
	POWER_ANT_NUM
};

/* For CHAIN_COMP/CHAIN_ABS */
enum ENUM_PWR_LMT_CHAIN_BAND {
	PWR_LMT_CHAIN_2G4_BAND = 0,
	PWR_LMT_CHAIN_5G_BAND1,
	PWR_LMT_CHAIN_5G_BAND2,
	PWR_LMT_CHAIN_5G_BAND3,
	PWR_LMT_CHAIN_5G_BAND4,
	PWR_LMT_CHAIN_6G_BAND1,
	PWR_LMT_CHAIN_6G_BAND2,
	PWR_LMT_CHAIN_6G_BAND3,
	PWR_LMT_CHAIN_6G_BAND4,
	PWR_LMT_CHAIN_BAND_NUM,
};

/* For CHAIN_COMP/CHAIN_ABS */
enum ENUM_PWR_LMT_CHAIN_ANT {
	PWR_LMT_CHAIN_ANT_WF0 = 0,
	PWR_LMT_CHAIN_ANT_WF1,
	/* To Do compile option ? */
	PWR_LMT_CHAIN_ANT_WF2,
	PWR_LMT_CHAIN_ANT_NUM
};
#endif

enum ENUM_PWR_LMT_CHAIN_CFG_TYPE {
	PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_SGL_BAND = 0,
	PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_MULTI_BAND,
	PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_SGL_BAND,
	PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_MULTI_BAND,
	PWR_LMT_CHAIN_CFG_TYPE_NUM
};

struct TX_PWR_CTRL_CHANNEL_SETTING {
	enum ENUM_TX_POWER_CTRL_CHANNEL_TYPE eChnlType;
	uint8_t channelParam[2];
	/******** 2G/5G Legacy *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN op[PWR_LIMIT_DYN_LEGACY_NUM];
	int8_t i8PwrLimit[PWR_LIMIT_DYN_LEGACY_NUM];
	/******** 2G/5G HE *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN opHE[PWR_LIMIT_DYN_HE_NUM];
	int8_t i8PwrLimitHE[PWR_LIMIT_DYN_HE_NUM];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/******** 2G/5G EHT *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN opEHT[PWR_LIMIT_DYN_EHT_NUM];
	int8_t i8PwrLimitEHT[PWR_LIMIT_DYN_EHT_NUM];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	/******** 6G Legacy *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN
		opLegacy_6G[PWR_LIMIT_DYN_LEGACY_6G_NUM];
	int8_t i8PwrLimitLegacy_6G[PWR_LIMIT_DYN_LEGACY_6G_NUM];
	/******** 6G HE *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN op6E[PWR_LIMIT_DYN_HE_6G_NUM];
	int8_t i8PwrLimit6E[PWR_LIMIT_DYN_HE_6G_NUM];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/******** 6G EHT *********/
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN opEHT_6G[PWR_LIMIT_DYN_EHT_6G_NUM];
	int8_t i8PwrLimitEHT_6G[PWR_LIMIT_DYN_EHT_6G_NUM];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
};

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
struct TX_PWR_CTRL_ANT_SETTING {
	int8_t aiPwrAnt2G4[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt5GB1[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt5GB2[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt5GB3[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt5GB4[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt6GB1[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt6GB2[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt6GB3[PWR_LMT_CHAIN_ANT_NUM];
	int8_t aiPwrAnt6GB4[PWR_LMT_CHAIN_ANT_NUM];
};
#endif

struct TX_PWR_CTRL_ELEMENT {
	struct LINK_ENTRY node;
	u_int8_t fgApplied;
	char name[MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE]; /* scenario name */
	uint8_t index; /* scenario index */
	enum ENUM_TX_POWER_CTRL_TYPE eCtrlType;
	uint8_t settingCount;
	/* channel setting count. [.....] means one channel setting */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	struct TX_PWR_CTRL_ANT_SETTING aiPwrAnt[POWER_ANT_TAG_NUM];
#endif
	struct TX_PWR_CTRL_CHANNEL_SETTING rChlSettingList[];
	/* always keep it the last one. */
};

struct PARAM_TX_PWR_CTRL_IOCTL {
	u_int8_t fgApplied;
	uint8_t *name;
	uint8_t index;
	uint8_t *newSetting;
};
#endif

/* Define g_rRlmSubBand index
 * Due to TxPower limit cmd size,
 * EHT cmd can't directly send
 * UNII5 or UNII7 in one cmd
 * So, We Separate UNII5 into UNII5A, UNII5B
 * UNII7 into UNII7A, UNII7B
 */
enum ENUM_PWR_LMT_SUBBAND {
	PWR_LMT_SUBBAND_2G4 = 0,
	PWR_LMT_SUBBAND_UNII1 = 1,
	PWR_LMT_SUBBAND_UNII2A = 2,
	PWR_LMT_SUBBAND_UNII2C = 3,
	PWR_LMT_SUBBAND_UNII3 = 4,
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_LMT_SUBBAND_UNII5A = 5,
	PWR_LMT_SUBBAND_UNII5B = 6,
	PWR_LMT_SUBBAND_UNII6 = 7,
	PWR_LMT_SUBBAND_UNII7A = 8,
	PWR_LMT_SUBBAND_UNII7B = 9,
	PWR_LMT_SUBBAND_UNII8 = 10,
#endif /* CFG_SUPPORT_WIFI_6G */
	PWR_LMT_SUBAND_NUM
};

/* Define TxPwr lmt default table subband pwr index */
enum ENUM_PWR_LMT_DEF_PWR {
	PWR_LMT_SUBBAND_PWR_2G4 = 0,
	PWR_LMT_SUBBAND_PWR_UNII1 = 1,
	PWR_LMT_SUBBAND_PWR_UNII2A = 2,
	PWR_LMT_SUBBAND_PWR_UNII2C = 3,
	PWR_LMT_SUBBAND_PWR_UNII3 = 4,
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_LMT_SUBBAND_PWR_UNII5 = 5,
	PWR_LMT_SUBBAND_PWR_UNII6 = 6,
	PWR_LMT_SUBBAND_PWR_UNII7 = 7,
	PWR_LMT_SUBBAND_PWR_UNII8 = 8,
#endif /* CFG_SUPPORT_WIFI_6G */
	PWR_LMT_DEF_PWR_NUM
};

/* Define channel offset in unit of 5MHz bandwidth */
enum ENUM_CHNL_SPAN {
	CHNL_SPAN_5 = 1,
	CHNL_SPAN_10 = 2,
	CHNL_SPAN_20 = 4,
	CHNL_SPAN_40 = 8,
	CHNL_SPAN_80 = 16
};

/* Define BSS operating bandwidth */
enum ENUM_CHNL_BW {
	CHNL_BW_20,
	CHNL_BW_20_40,
	CHNL_BW_10,
	CHNL_BW_5
};

/*  Define Power limit Rate Tag */
enum ENUM_PWR_CFG_RATE_TAG {
	PWR_CFG_RATE_TAG_MISS = 0,
	PWR_CFG_RATE_TAG_HIT_LEGACY = 1,
	PWR_CFG_RATE_TAG_HIT_HE = 2,
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_CFG_RATE_TAG_HIT_EHT = 3,
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_CFG_RATE_TAG_HIT_LEGACY6G = 4,
	PWR_CFG_RATE_TAG_HIT_HE6G = 5,
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_CFG_RATE_TAG_HIT_EHT6G = 6,
#endif
#endif
	PWR_CFG_RATE_TAG_NUM
};

#if 0
/* If channel width is CHNL_BW_20_40, the first channel will be SCA and
 * the second channel is SCB, then iteratively.
 * Note the final channel will not be SCA.
 */
struct DOMAIN_SUBBAND_INFO {
	uint8_t ucRegClass;
	enum ENUM_BAND eBand;
	enum ENUM_CHNL_SPAN eChannelSpan;
	uint8_t ucFirstChannelNum;
	uint8_t ucNumChannels;
	enum ENUM_CHNL_BW eChannelBw;
	u_int8_t fgDfsNeeded;
	u_int8_t fgIbssProhibited;
};

/* Use it as all available channel list for STA */
struct DOMAIN_INFO_ENTRY {
	uint16_t u2CountryCode;
	uint16_t u2MibRegDomainValue;

	/* If different attributes, put them into different rSubBands.
	 * For example, DFS shall be used or not.
	 */
	struct DOMAIN_SUBBAND_INFO rSubBand[MAX_SUBBAND_NUM];
};

#else /* New definition 20110830 */

/* In all bands, the first channel will be SCA and the second channel is SCB,
 * then iteratively.
 * Note the final channel will not be SCA.
 */
struct DOMAIN_SUBBAND_INFO {
	/* Note1: regulation class depends on operation bandwidth and RF band.
	 *  For example: 2.4GHz, 1~13, 20MHz ==> regulation class = 81
	 *      2.4GHz, 1~13, SCA   ==> regulation class = 83
	 *      2.4GHz, 1~13, SCB   ==> regulation class = 84
	 * Note2: TX power limit is not specified here because path loss
	 *        is unknown.
	 */
	uint8_t ucRegClass;	/* Regulation class for 20MHz */
	uint8_t ucBand;		/* Type: ENUM_BAND_T */
	uint8_t ucChannelSpan;	/* Type: ENUM_CHNL_SPAN_T */
	uint8_t ucFirstChannelNum;
	uint8_t ucNumChannels;
	uint8_t fgDfs;	/* Type: BOOLEAN*/
};

/* Use it as all available channel list for STA */
struct DOMAIN_INFO_ENTRY {
	uint16_t *pu2CountryGroup;
	uint32_t u4CountryNum;

	/* If different attributes, put them into different rSubBands.
	 * For example, DFS shall be used or not.
	 */
	struct DOMAIN_SUBBAND_INFO rSubBand[MAX_SUBBAND_NUM];
};
#endif

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/*
 * MT_TxPwrLimit.dat format
 */
#define SECTION_PREFIX (0x23232323)
#define ELEMENT_PREFIX (0xffff)
#define VERSION (0x00000001)
#define SIZE_OF_VERSION 4
#define WLAN_TX_PWR_LIMIT_FILE_BUF_SIZE 204800

struct tx_pwr_element {
	uint16_t prefix;
	uint8_t channel_num;
	uint8_t reserved;

	/*the followings are in unit: 0.5 dbm*/

	uint8_t tx_pwr_dsss_cck;
	uint8_t tx_pwr_dsss_bpsk;

	uint8_t tx_pwr_ofdm_bpsk; /* 6M, 9M */
	uint8_t tx_pwr_ofdm_qpsk; /* 12M, 18M */
	uint8_t tx_pwr_ofdm_16qam; /* 24M, 36M */
	uint8_t tx_pwr_ofdm_48m;
	uint8_t tx_pwr_ofdm_54m;

	uint8_t tx_pwr_ht20_bpsk; /* MCS0*/
	uint8_t tx_pwr_ht20_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_ht20_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_ht20_mcs5; /* MCS5*/
	uint8_t tx_pwr_ht20_mcs6; /* MCS6*/
	uint8_t tx_pwr_ht20_mcs7; /* MCS7*/

	uint8_t tx_pwr_ht40_bpsk; /* MCS0*/
	uint8_t tx_pwr_ht40_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_ht40_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_ht40_mcs5; /* MCS5*/
	uint8_t tx_pwr_ht40_mcs6; /* MCS6*/
	uint8_t tx_pwr_ht40_mcs7; /* MCS7*/

	uint8_t tx_pwr_vht20_bpsk; /* MCS0*/
	uint8_t tx_pwr_vht20_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_vht20_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_vht20_64qam; /* MCS5, MCS6*/
	uint8_t tx_pwr_vht20_mcs7;
	uint8_t tx_pwr_vht20_mcs8;
	uint8_t tx_pwr_vht20_mcs9;

	uint8_t tx_pwr_vht_40;
	uint8_t tx_pwr_vht_80;
	uint8_t tx_pwr_vht_160nc;
	uint8_t tx_pwr_lg_40;
	uint8_t tx_pwr_lg_80;

	uint8_t tx_pwr_1ss_delta;
	uint8_t reserved_3[3];
};

struct tx_pwr_section {
	uint32_t prefix;
	uint32_t country_code;
};
#endif /*#if (CFG_SUPPORT_SINGLE_SKU == 1)*/

struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT {
	uint8_t aucCountryCode[2];
	/* 0: 2.4G ch   1 ~  14
	 * 1: 5G   ch  36 ~  48
	 * 2: 5G   ch  52 ~  64
	 * 3: 5G   ch 100 ~ 144
	 * 4: 5G   ch 149 ~ 165
	 * 5: 6G   ch  95 ~ 115
	 * 6: 6G   ch 117 ~ 151
	 * 7: 6G   ch 153 ~ 185
	 * 8: 6G   ch 187 ~ 233
	 */
	int8_t aucPwrLimitSubBand[PWR_LMT_DEF_PWR_NUM];
	/* bit0: cPwrLimit2G4, bit1: cPwrLimitUnii1; bit2: cPwrLimitUnii2A;*/
	/* bit3: cPwrLimitUnii2C; bit4: cPwrLimitUnii3; mW: 0, mW\MHz : 1 */
	uint8_t ucPwrUnit;
};

struct SUBBAND_CHANNEL {
	enum ENUM_BAND eBand;
	uint8_t ucStartCh;
	uint8_t ucEndCh;
	uint8_t ucInterval;
	uint8_t ucReserved;
};

struct COUNTRY_POWER_LIMIT_COUNTRY_CODE {
	uint8_t    aucCountryCode[2];
};

struct COUNTRY_POWER_LIMIT_GROUP_TABLE {
	uint8_t    aucGroupCode[2];
	uint32_t   u4CountryNum;
	struct COUNTRY_POWER_LIMIT_COUNTRY_CODE *prGroup;
};

#endif /* CFG_SUPPORT_PWR_LIMIT_COUNTRY */

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/*
 * Event from chip for single sku
 */
struct SINGLE_SKU_INFO {
	uint32_t u4EfuseCountryCode;
	uint8_t  isEfuseValid;
	uint8_t  ucReserved[7];
};

/*
 * single sku control structure
 */
enum regd_state {
	REGD_STATE_UNDEFINED,
	REGD_STATE_INIT,
	REGD_STATE_SET_WW_CORE,
	REGD_STATE_SET_COUNTRY_USER,
	REGD_STATE_SET_COUNTRY_DRIVER,
	REGD_STATE_SET_COUNTRY_IE,
	REGD_STATE_INVALID
};

struct mtk_regd_control {
	u_int8_t en;
	u_int8_t isEfuseCountryCodeUsed;
	enum regd_state state;
	uint32_t alpha2;
	uint32_t tmp_alpha2; /*store country code set by iwpriv "country XX"*/
	struct GLUE_INFO *pGlueInfo; /*wlan GlueInfo*/
	u8 n_channel_active_2g;
	u8 n_channel_active_5g;
	u8 n_channel_active_6g;
	struct CMD_DOMAIN_CHANNEL channels[MAX_SUPPORTED_CH_COUNT];
	u8 dfs_region;
	enum ENUM_MBMC_BN eDBDCBand;
};

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
struct mtk_regdomain {
	char country_code[4];
	const void *prRegdRules;
};
#endif

#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
/* Order by priority */
enum ENUM_PWR_MODE_6G_TYPE {
	PWR_MODE_6G_SP = 0,  /* Standard Power, Priority: Low */
	PWR_MODE_6G_LPI = 1, /* Low Power Indoor*/
	PWR_MODE_6G_VLP = 2, /* Very Low Power, Priority: High */
	PWR_MODE_6G_NUM
};

struct  PWR_MODE_6G_SUBAND_SUPPROT {
	uint8_t   fgPwrMode6GSupport[PWR_MODE_6G_NUM];
};

struct COUNTRY_PWR_MODE_6G_SUPPORT_TABLE {
	uint8_t aucCountryCode[2];
	struct PWR_MODE_6G_SUBAND_SUPPROT rSubBand[SUBBAND_6G_NUM];
};
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
struct TX_PWR_ANT_CFG_PARA_TABLE {
	const char arKeywords[32];
	uint8_t ucSettingNum;
	uint8_t ucStart;
	uint8_t ucEnd;
};
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */

struct PWR_LIMIT_INFO {
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
	uint8_t ucVersion;
	enum ENUM_PWR_LMT_SUBBAND eStartSubBand;
	enum ENUM_PWR_LMT_SUBBAND eEndSubBand;
};

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT_INFO {
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLmtDefaultTable;
	uint32_t TableNum;
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY_INFO {
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY *table;
	uint32_t table_num;
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE_INFO {
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE *table;
	uint32_t table_num;
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT_INFO {
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT *table;
	uint32_t table_num;
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_INFO {
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY_INFO Legacy;
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE_INFO HE;
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT_INFO EHT;
};

typedef void (*PFN_PWR_LMT_DEFAULT_PAYLOAD_FUNC) (
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitSubBand,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo);

typedef void (*PFN_PWR_LMT_CONFIG_PAYLOAD_FUNC) (
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

typedef void (*PFN_PWR_LMT_DUMP_PAYLOAD_FUNC) (
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

typedef void (*PFN_PWR_LMT_APPLY_DYN_SETTING_FUNC) (
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *element,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

typedef void (*PFN_PWR_LMT_WRITE_EMI_FUNC) (
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *size);

struct PWR_LIMIT_HANDLER_INFO {
	PFN_PWR_LMT_DEFAULT_PAYLOAD_FUNC pfLoadDefTbl;
	PFN_PWR_LMT_CONFIG_PAYLOAD_FUNC pfLoadCfgTbl;
	PFN_PWR_LMT_DUMP_PAYLOAD_FUNC pfDumpData;
	PFN_PWR_LMT_APPLY_DYN_SETTING_FUNC pfApplyDynSet;
	PFN_PWR_LMT_WRITE_EMI_FUNC pfWriteEmi;
};
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */
/*******************************************************************************
 * P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *         P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *      M A C R O S
 *******************************************************************************
 */
#define CAL_CH_OFFSET_80M(_PRIMARY_CH, _CENTRAL_CH) \
			(((_PRIMARY_CH - _CENTRAL_CH) + 6) >> 2)

#define CAL_CH_OFFSET_160M(_PRIMARY_CH, _CENTRAL_CH) \
			(((_PRIMARY_CH - _CENTRAL_CH) + 14) >> 2)

/*******************************************************************************
 * F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
struct DOMAIN_INFO_ENTRY *rlmDomainGetDomainInfo(
	struct ADAPTER *prAdapter);

void
rlmDomainGetChnlList(struct ADAPTER *prAdapter,
		     enum ENUM_BAND eSpecificBand, u_int8_t fgNoDfs,
		     uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
		     struct RF_CHANNEL_INFO *paucChannelList);

void rlmDomainGetChnlListFromOpClass(struct ADAPTER *prAdapter,
	uint8_t ucOpClass, struct RF_CHANNEL_INFO *paucChannelList,
	uint8_t *pucChannelListNum);

void rlmDomainGetDfsChnls(struct ADAPTER *prAdapter,
			  uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
			  struct RF_CHANNEL_INFO *paucChannelList);

u_int8_t rlmDomainIsDfsChnls(struct ADAPTER *prAdapter,
				uint8_t ucChannel);

void rlmDomainSendCmd(struct ADAPTER *prAdapter, bool fgPwrLmtSend);

void rlmDomainResetActiveChannel(void);

void rlmDomainSendDomainInfoCmd(struct ADAPTER *prAdapter);

void rlmDomainSendPassiveScanInfoCmd(struct ADAPTER
				     *prAdapter);

uint32_t rlmDomainSupOperatingClassIeFill(uint8_t *pBuf);

u_int8_t rlmDomainCheckChannelEntryValid(struct ADAPTER
		*prAdapter, enum ENUM_BAND eBand, uint8_t ucCentralCh);

uint8_t rlmDomainGetCenterChannel(enum ENUM_BAND eBand,
				  uint8_t ucPriChannel,
				  enum ENUM_CHNL_EXT eExtend);

u_int8_t rlmDomainIsValidRfSetting(struct ADAPTER *prAdapter,
				   enum ENUM_BAND eBand, uint8_t ucPriChannel,
				   enum ENUM_CHNL_EXT eExtend,
				   enum ENUM_CHANNEL_WIDTH eChannelWidth,
				   uint8_t ucChannelS1, uint8_t ucChannelS2);

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

u_int8_t
rlmDomainCheckPowerLimitValid(struct ADAPTER *prAdapter,
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY
#else
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */
		rPowerLimitTableConfiguration,
	uint8_t ucPwrLimitNum);

void rlmDomainCheckCountryPowerLimitTable(
	struct ADAPTER *prAdapter);

uint16_t rlmDomainPwrLimitDefaultTableDecision(
	struct ADAPTER *prAdapter, uint16_t u2CountryCode);

void rlmDomainSendPwrLimitCmd(struct ADAPTER *prAdapter);
#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)
u_int8_t rlmDomainIsUsingLocalRegDomainDataBase(void);
void rlmDomainSetCountryCode(char *alpha2,
			     u8 size_of_alpha2);
void rlmDomainSetDfsRegion(u8 dfs_region);
u8 rlmDomainGetDfsRegion(void);
void rlmDomainSetDfsDbdcBand(enum ENUM_MBMC_BN eDBDCBand);
enum ENUM_MBMC_BN rlmDomainGetDfsDbdcBand(void);
void rlmDomainResetCtrlInfo(u_int8_t force);
void rlmDomainAddActiveChannel(u8 band);
u8 rlmDomainGetActiveChannelCount(u8 band);
void rlmDomainParsingChannel(struct ADAPTER *prAdapter);
struct CMD_DOMAIN_CHANNEL *rlmDomainGetActiveChannels(void);
void rlmExtractChannelInfo(u32 max_ch_count,
			   struct CMD_DOMAIN_ACTIVE_CHANNEL_LIST *prBuff);
void regd_set_using_local_regdomain_db(void);
void rlmDomainSetDefaultCountryCode(void);
enum regd_state rlmDomainGetCtrlState(void);
bool rlmDomainIsSameCountryCode(char *alpha2,
				u8 size_of_alpha2);
const void *rlmDomainSearchRegdomainFromLocalDataBase(char *alpha2);
struct GLUE_INFO *rlmDomainGetGlueInfo(void);
bool rlmDomainIsEfuseUsed(void);
uint8_t rlmDomainGetChannelBw(enum ENUM_BAND eBand, uint8_t channelNum);

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
extern const struct mtk_regdomain *g_prRegRuleTable[];
#endif

#endif

const void *rlmDomainGetLocalDefaultRegd(void);
void rlmDomainSendInfoToFirmware(struct ADAPTER
				 *prAdapter);
uint32_t rlmDomainExtractSingleSkuInfoFromFirmware(
	struct ADAPTER *prAdapter, uint8_t *pucEventBuf);
u_int8_t regd_is_single_sku_en(void);
u_int8_t rlmDomainIsLegalChannel(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand, uint8_t ucChannel);
#if CFG_CH_SELECT_ENHANCEMENT
u_int8_t rlmDomainIsIndoorChannel(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand, uint8_t ucChannel);
#endif
u_int8_t rlmDomainIsStaSapIndoorConn(struct ADAPTER *prAdapter);
u_int8_t rlmDomainIsLegalDfsChannel(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucChannel);
enum ENUM_CHNL_EXT rlmSelectSecondaryChannelType(
	struct ADAPTER *prAdapter, enum ENUM_BAND band,
	u8 primary_ch);
void rlmDomainOidSetCountry(struct ADAPTER *prAdapter,
	char *country, uint8_t size_of_country, uint8_t fgNeedHoldRtnlLock);
u32 rlmDomainGetCountryCode(void);
void rlmDomainAssert(u_int8_t cond);
void rlmDomainU32ToAlpha(uint32_t u4CountryCode, char *pcAlpha);
uint32_t rlmDomainAlpha2ToU32(char *pcAlpha2, uint8_t ucAlpha2Size);
uint8_t rlmDomainCountryCodeUpdateSanity(
	struct GLUE_INFO *prGlueInfo,
	struct ADAPTER **prAdapter);

void rlmDomainCountryCodeUpdate(struct ADAPTER *prAdapter,
		uint32_t u4CountryCode, uint8_t fgNeedHoldRtnlLock);
void rlmDomainSetCountry(struct ADAPTER *prAdapter,
	uint8_t fgNeedHoldRtnlLock);

uint32_t
rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
	uint32_t u4CountryCode, uint8_t fgNeedHoldRtnlLock);

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
void txPwrCtrlInit(struct ADAPTER *prAdapter);
void txPwrCtrlLoadConfig(struct ADAPTER *prAdapter);
void txPwrCtrlUninit(struct ADAPTER *prAdapter);
void txPwrCtrlShowList(struct ADAPTER *prAdapter,
				uint8_t filterType,
				char *message);
void txPwrCtrlDeleteElement(struct ADAPTER *prAdapter,
				uint8_t *name, uint32_t index,
				enum ENUM_TX_POWER_CTRL_LIST_TYPE eListType);
struct TX_PWR_CTRL_ELEMENT *txPwrCtrlStringToStruct(char *pcContent,
				u_int8_t fgSkipHeader);
struct TX_PWR_CTRL_ELEMENT *txPwrCtrlFindElement(
				struct ADAPTER *prAdapter,
				uint8_t *name,
				uint32_t index,
				u_int8_t fgCheckIsApplied,
				enum ENUM_TX_POWER_CTRL_LIST_TYPE eListType);
void txPwrCtrlAddElement(struct ADAPTER *prAdapter,
				struct TX_PWR_CTRL_ELEMENT *prElement);

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
int32_t txPwrParseTagMimo1T(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);
int32_t txPwrParseTagMimo2T(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);
int32_t txPwrParseTagAllT(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);

int32_t txPwrParseTagAllT6G(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);

int32_t txPwrParseTagChainComp(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);

int32_t txPwrParseTagChainAbs(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord);
#endif

#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update 6G power mode, when the power mode have
 *        been change, it will re-send country power limit cmd to FW
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] ucBssIndex : Bss index
 * \param[in] e6GPwrMode : Enum of 6G power mode
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomain6GPwrModeUpdate(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeCurr);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to decide the current 6G power mode for STA
 *
 * \param[in] fgIsHE6GPresent : Flag for HE 6G info have present
 * \param[in] uc6GHeRegInfo : HE regulaty info
 *
 * \return value : Enum of 6G Power mode
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomain6GPwrModeDecision(
	struct ADAPTER *prAdapter,
	uint8_t fgIsHE6GPresent,
	uint8_t uc6GHeRegInfo);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use check whether the country record from STA
 *       support the current 6G power mode or not.
 *
 * \param[in] eBand : RF Band index
 * \param[in] ucCenterCh : Center Channel
 * \param[in] u2CountryCode : Country code
 * \param[in] e6GPwrMode : Enum of 6G Power mode
 * \param[in] pfgSupport : Pointer of flag to indicate the support or not for
 *                         STA country
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomain6GPwrModeCountrySupportChk(
	enum ENUM_BAND eBand,
	uint8_t ucCenterCh,
	uint16_t u2CountryCode,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode,
	uint8_t *pfgSupport);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use check whether the subband of the country
 *       support the current 6G power mode or not.
 *
 * \param[in] eBand : RF Band index
 * \param[in] u1SubBand : Subband index
 * \param[in] u2CountryCode : Country code
 * \param[in] e6GPwrMode : Enum of 6G Power mode
 * \param[in] pfgSupport : Pointer of flag to indicate the support or not for
 *                         STA country
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomain6GPwrModeSubbandChk(
	enum ENUM_BAND eBand,
	uint8_t u1SubBand,
	uint16_t u2CountryCode,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode,
	uint8_t *pfgSupport);
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */
/*******************************************************************************
 *   F U N C T I O N S
 *******************************************************************************
 */

uint8_t regCountryDfsMapping(struct ADAPTER *prAdapter);
#endif /* _RLM_DOMAIN_H */
