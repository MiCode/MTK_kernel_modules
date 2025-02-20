/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/******************************************************************************
*                         C O M P I L E R   F L A G S
*******************************************************************************
*/

/******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
*******************************************************************************
*/
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
//#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/serial.h> /* struct serial_struct  */
#include "uart_launcher.h"
#ifdef __ANDROID__
/* use nvram */
#include <dlfcn.h>
#include "CFG_BT_File.h"
#include "CFG_file_lid.h"
#include "libnvram.h"
#endif

//---------------------------------------------------------------------------
static int set_speed(int fd, struct termios *ti, int speed);
int setup_uart_param (int hComPort, int iBaudrate, struct UART_CONFIG *sUartConfig);

#ifdef __ANDROID__
static int notifyFd = -1;
#endif
static int gTtyFd = -1;
static int cont = 1;    /** loop continue running */
struct flock fl;

//---------------------------------------------------------------------------
/* Used as host uart param setup callback */
int setup_uart_param (
    int hComPort,
    int iBaudrate,
    struct UART_CONFIG *sUartConfig)
{
    struct termios ti;
    int  fd;
    BPRINT_I("setup_uart_param begin");
    if(!sUartConfig){
        BPRINT_E("Invalid sUartConfig");
        return -2;
    }

    BPRINT_I("setup_uart_param Baud %d FC %d", iBaudrate, sUartConfig->fc);

    fd = hComPort;
    if (fd < 0) {
        BPRINT_E("Invalid serial port");
        return -2;
    }

    tcflush(fd, TCIOFLUSH);

    if (tcgetattr(fd, &ti) < 0) {
        BPRINT_E("Can't get port settings");
        return -3;
    }

    cfmakeraw(&ti);

    BPRINT_I("ti.c_cflag = 0x%08x", ti.c_cflag);
    ti.c_cflag |= CLOCAL;
    BPRINT_I("CLOCAL = 0x%x", CLOCAL);
    BPRINT_I("(ori)ti.c_iflag = 0x%08x", ti.c_iflag);
    BPRINT_I("(ori)ti.c_cflag = 0x%08x", ti.c_cflag);
    BPRINT_I("sUartConfig->fc= %d (0:none,sw,hw,linux)", sUartConfig->fc);

    switch (sUartConfig->fc) {
     /* HW FC Enable */
    case UART_HW_FC:
        ti.c_cflag |= CRTSCTS;
        ti.c_iflag &= ~(NOFLSH);
        break;
    /* Linux Software FC */
    case UART_LINUX_FC:
        ti.c_iflag |= (IXON | IXOFF | IXANY);
        ti.c_cflag &= ~(CRTSCTS);
        ti.c_iflag &= ~(NOFLSH);
        break;
    /* MTK Software FC */
    case UART_MTK_SW_FC:
        ti.c_iflag |= CRTSCTS;
        ti.c_cflag &= ~(NOFLSH);
        break;
    /* default disable flow control */
    default:
        ti.c_cflag &= ~(CRTSCTS);
        ti.c_iflag &= ~(NOFLSH|CRTSCTS);
    }

    BPRINT_D("c_c CRTSCTS = 0x%16x", CRTSCTS);
    BPRINT_D("c_i IXON = 0x%08x", IXON);
    BPRINT_D("c_i IXOFF = 0x%08x", IXOFF);
    BPRINT_D("c_i IXANY = 0x%08x", IXANY);
    BPRINT_D("(aft)ti.c_iflag = 0x%08x", ti.c_iflag);
    BPRINT_D("(aft)ti.c_cflag = 0x%08x", ti.c_cflag);

    if (tcsetattr(fd, TCSANOW, &ti) < 0) {
        BPRINT_E("Can't set port settings");
        return -4;
    }

    /* Set baudrate */
    if (set_speed(fd, &ti, iBaudrate) < 0) {
        BPRINT_E("Can't set initial baud rate");
        return -5;
    }

    tcflush(fd, TCIOFLUSH);
    BPRINT_I("%s done", __func__);
    return 0;
}

//---------------------------------------------------------------------------
static speed_t get_speed (int baudrate)
{
    unsigned int idx;
    for (idx = 0; idx < sizeof(speeds)/sizeof(speeds[0]); idx++) {
        if (baudrate == (int)speeds[idx].baud) {
            return speeds[idx].speed;
        }
    }
    return CBAUDEX;
}

//---------------------------------------------------------------------------
int set_speed(int fd, struct termios *ti, int speed)
{
    struct serial_struct ss;
    int baudenum = get_speed(speed);

    if (speed != CBAUDEX) {
        //printf("%s: standard baudrate: %d -> 0x%08x\n", __FUNCTION__, speed, baudenum);
        if ((ioctl(fd, TIOCGSERIAL, &ss)) < 0) {
            BPRINT_E("%s: BAUD: error to get the serial_struct info:%s\n", __func__, strerror(errno));
            return -1;
        }
#ifdef ANDROID
        ss.flags &= ~ASYNC_SPD_CUST;
#if defined(SERIAL_STRUCT_EXT) /*modified in serial_struct.h*/
        memset(ss.reserved, 0x00, sizeof(ss.reserved));
#endif
        ss.flags |= (1 << 13);    /*set UPFLOWLATENCY flat to tty, or serial_core will reset tty->low_latency to 0*/
        /*set standard buadrate setting*/
        if ((ioctl(fd, TIOCSSERIAL, &ss)) < 0) {
            BPRINT_E("%s: BAUD: error to set serial_struct:%s\n", __func__, strerror(errno));
            return -2;
        }
#endif
        cfsetospeed(ti, baudenum);
        cfsetispeed(ti, baudenum);
        return tcsetattr(fd, TCSANOW, ti);
    }
    else {
        BPRINT_E("%s: unsupported non-standard baudrate: %d -> 0x%08x\n", __func__, speed, baudenum);
        return -3;
    }
}

//---------------------------------------------------------------------------
void init_flock(struct flock *flk)
{
    flk->l_type = F_WRLCK;
    flk->l_whence = SEEK_SET;
    flk->l_pid = getpid();
    flk->l_start = 0;
    flk->l_len = 0;
}

void unlock_flock(int fd, struct flock *fl, int type, int whence)
{
    fl->l_type = type;
    fl->l_whence = whence;
    if (fcntl(fd, F_SETLKW, fl) < 0)
        BPRINT_E("%s: fcntl failed(%d)", __func__, errno);
}

//---------------------------------------------------------------------------
int cmd_hdr_baud (struct UART_CONFIG *sUartConfig, int baudrate) {
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, baudrate, sUartConfig) : -1;
}

//---------------------------------------------------------------------------
int osi_system(const char *cmd)
{
    FILE *fp;
    int ret;

    if (cmd == NULL) {
        BPRINT_E("%s: cmd is NULL", __func__);
        return -1;
    }

    fp = popen(cmd, "w");
    if (fp == NULL) {
        BPRINT_E("%s: (%s) failed", __func__, cmd);
        return -1;
    }

    BPRINT_I("Command: %s", cmd);

    ret = pclose(fp);
    if (ret < 0) {
        BPRINT_E("%s: pclose ret = %d", __func__, ret);
    } else if (ret > 0) {
        BPRINT_I("%s: pclose ret = %d", __func__, ret);
    }
    return ret;
}

//---------------------------------------------------------------------------
static void uart_launcher_sig_handler(int signum)
{
    BPRINT_I("%s: sig[%d] fd[%d]", __func__, signum, gTtyFd);
    cont = 0;
}
#ifdef __ANDROID__
static int xo_read_nvram(CONNXO_CFG_PARAM_STRUCT *pConnxo_cfg_param)
{
    F_ID xo_nvram_fd;
    int rec_size = 0;
    int rec_num = 0;
    unsigned char buf[27] = {"\0"};
    unsigned int num_read = 0;

    void* hcustlib = dlopen("libcustom_nvram.so", RTLD_LAZY);
    if (!hcustlib) {
	BPRINT_E("%s(), dlopen fail! (%s)\n", __FUNCTION__, dlerror());
	return -1;
    }

    unsigned int* iFileXOILID = (unsigned int*)(dlsym(hcustlib, "iFileCONNXOLID")); /* AP_CFG_RDEB_FILE_CONNXO_LID */
    if (!iFileXOILID) {
	BPRINT_E("Error reading iBTDefaultSize\n");
	dlclose(hcustlib);
	return false;
    }

    xo_nvram_fd = NVM_GetFileDesc(*iFileXOILID, &rec_size, &rec_num, ISREAD);

    BPRINT_I("%s: Open CONNXO NVRAM success", __func__);
    BPRINT_I("%s: xo_nvram_fd %d rec_size %d rec_num %d\n",__func__, xo_nvram_fd.iFileDesc, rec_size, rec_num);

    num_read = read(xo_nvram_fd.iFileDesc, buf, sizeof(buf)-1);
    if (num_read <= 0) {
        BPRINT_E("%s: num_read <= 0", __func__);
        dlclose(hcustlib);
        return -1;
    }

    memcpy(pConnxo_cfg_param, &buf[0], sizeof(CONNXO_CFG_PARAM_STRUCT));

    dlclose(hcustlib);

    return 0;
}
#endif
//---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    BPRINT_I("Bluetooth Uart Launcher Ver %s", VERSION);
    int ld = 0;
    struct pollfd fds;
    int fd_num = 0;
    int err = 0;
    int opt;
    int baudrate = 115200;
    int chang_baud_rate = 0;
    int retry = 0;
    int flow_control = UART_DISABLE_FC;
    char *tty_path = "/dev/ttyUSB0";
    struct UART_CONFIG sUartConfig;
    struct sigaction sigact;
#ifdef __ANDROID__
    int ret = 0;
    CONNXO_CFG_PARAM_STRUCT sConnxo_cfg;
#endif
    memset(&sUartConfig, 0, sizeof(struct UART_CONFIG));
    memset(&fds, 0, sizeof(struct pollfd));
#ifdef __ANDROID__
    memset(&sConnxo_cfg, 0, sizeof(CONNXO_CFG_PARAM_STRUCT));
#endif
    sUartConfig.fc = flow_control;

    /* Register signal handler */
    sigact.sa_handler = uart_launcher_sig_handler;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    err = sigaction(SIGINT, &sigact, NULL);
    if (!err)
        BPRINT_E("SIGINT failed, err = %d", err);

    err = sigaction(SIGTERM, &sigact, NULL);
    if (!err)
        BPRINT_E("SIGTERM failed, err = %d", err);

    err = sigaction(SIGQUIT, &sigact, NULL);
    if (!err)
        BPRINT_E("SIGQUIT failed, err = %d", err);
    /* SIGKILL, SIGSTOP may not be catched */
    err = sigaction(SIGKILL, &sigact, NULL);
    if (!err)
        BPRINT_E("SIGKILL failed, err = %d", err);

    err = sigaction(SIGSTOP, &sigact, NULL);
    if (!err)
        BPRINT_E("SIGSTOP, err = %d", err);
    init_flock(&fl);
    ld = N_MTK;

    while ((opt = getopt(argc, argv, "c:f:p:k:l::")) != -1) {
        switch (opt) {
            /* change baudrate */
            case 'c':
                baudrate = atoi(optarg);
                BPRINT_I("baudrate = %d", baudrate);
                /* not need to change if baudrate is same */
                if (baudrate == CUST_BAUDRATE_DFT) {
                    chang_baud_rate = 0;
                } else
                    chang_baud_rate = 1;
                break;
            /* flow control */
            case 'f':
                flow_control = atoi(optarg);
                BPRINT_I("flow_control = %d", flow_control);
                break;
            /* tty path */
            case 'p':
                tty_path = optarg;
                BPRINT_I("Log path is %s", tty_path);
                break;
            /* kill process */
            case 'k':
                osi_system("killall uart_launcher");
                BPRINT_I("Kill uart_launcher");
                return 0;
            /* set ldisc */
            case 'l':
                ld = atoi(optarg);
                BPRINT_I("set ldisc[%d]", ld);
                break;
            case '?':
        default:
                BPRINT_I("set baud:\t uart_launcher -c [baudrate]");
                BPRINT_I("flow control:\t uart_launcher -f [flow_control]");
                BPRINT_I("\t\t 0:disable FC  1:MTK_SW_FC  2: SW_FC  3: HW_FC");
                BPRINT_I("tty path:\t uart_launcher -p [path]");
                BPRINT_I("kill process:\t uart_launcher -k");
                goto exit;
                break;
        }
    }

    /* open ttyUSB */
    BPRINT_I("Running...");
    /* node may not ready, retry 20 times */
    while (1) {
        gTtyFd = open(tty_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        BPRINT_I("open done ttyfd %d", gTtyFd);
        if (gTtyFd < 0) {
            if (retry > 20) {
                BPRINT_E("ttyfd %d, error", gTtyFd);
                goto exit;
            } else {
                retry++;
                (void)usleep(1000 * 1000);
            }
        } else
            break;
    }

#ifdef __ANDROID__
    /* open bt_uart_launcher_notify */
    BPRINT_I("open /proc/stpbt/bt_uart_launcher_notify");
    /* node may not ready, retry 20 times */
    while (1) {
        notifyFd= open("/proc/stpbt/bt_uart_launcher_notify", O_RDONLY);
        BPRINT_I("open done bt_uart_launcher_notify fd[%d]", notifyFd);
        if (notifyFd < 0) {
            if (retry > 20) {
                BPRINT_E("bt_uart_launcher_notify %d, error", notifyFd);
	        break;
            } else {
                retry++;
                (void)usleep(1000 * 100);
            }
        } else
            break;
    }
#endif

    /* flock the device node */
    BPRINT_I("flock the device node");
    if (fcntl(gTtyFd, F_SETLK, &fl) < 0) {
        BPRINT_E("lock device node failed, uart_launcher already running.");
        goto exit;
    }

    /* to ensure driver register TIOCSETD, retry 20 times */
    retry = 0;
    while (1) {
        if (ioctl(gTtyFd, TIOCSETD, &ld) < 0) {
            if (retry > 20) {
                BPRINT_E("set TIOCSETD N_MTK error");
                goto exit;
            } else {
                retry++;
                (void)usleep(1000 * 1000);
            }
        } else
            break;
    }

#ifdef __ANDROID__
    /* read xo nvram */
    ret = xo_read_nvram(&sConnxo_cfg);
    err = ioctl(gTtyFd, HCIUARTXOPARAM, &sConnxo_cfg);
    if (err < 0) {
        BPRINT_E("ioctl HCIUARTXOPARAM error:[%d] %s", errno, strerror(errno));
        goto exit;
    }
#endif

restart:
    /* Set default Baud rate */
    sUartConfig.iBaudrate = CUST_BAUDRATE_DFT;
    BPRINT_I("set baudtate = %d", CUST_BAUDRATE_DFT);
    cmd_hdr_baud(&sUartConfig, CUST_BAUDRATE_DFT);
    fds.fd = gTtyFd;
    fds.events = POLLIN;
    ++fd_num;

    err = ioctl(gTtyFd, HCIUARTINIT, NULL);
    if (err < 0) {
        BPRINT_E("set HCIUARTINIT error %d", err);
        goto exit;
    }

    err = ioctl(gTtyFd, HCIUARTGETBAUD, NULL);
    if (err < 0) {
        BPRINT_E("set HCIUARTGETBAUD error %d", err);
        goto exit;
    }

    /* chang baud rate */
    if (chang_baud_rate | flow_control) {
        sUartConfig.iBaudrate = baudrate;
        sUartConfig.fc = flow_control;
        err = ioctl(gTtyFd, HCIUARTSETBAUD, &sUartConfig);
        if (err < 0) {
            BPRINT_E("set HCIUARTSETBAUD error %d", err);
            goto exit;
        }

        BPRINT_I("set baudtate %d", baudrate);
        cmd_hdr_baud(&sUartConfig, baudrate);

        err = ioctl(gTtyFd, HCIUARTSETWAKEUP, NULL);
        if (err < 0) {
            BPRINT_E("set HCIUARTSETWAKEUP error %d", err);
            goto exit;
        }
    }

    if (ioctl(gTtyFd, HCIUARTLOADPATCH, NULL) < 0) {
        BPRINT_E("set HCIUARTLOADPATCH error");
        goto exit;
    }

    while (cont) {
#ifdef __ANDROID__
        err = poll(&fds, fd_num, 20000);
#else
        err = poll(&fds, fd_num, 2000);
#endif
        if (err < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                BPRINT_E("poll error:%d errno:%d, %s", err, errno, strerror(errno));
                break;
            }
        } else if (!err) {
            if (fds.revents & POLLIN) {
                goto restart;
            } else {
                continue;
            }
        }
        goto restart;
    }

    BPRINT_I("%s: deinit flow fd[%d]", __func__, gTtyFd);

    if (gTtyFd < 0)
        goto exit;

#if !defined(__ANDROID__) // In sp project, baudrate is controled by driver
    /* before exit daemon, return baud to default */
    if (chang_baud_rate | flow_control) {
        sUartConfig.iBaudrate = CUST_BAUDRATE_DFT;
        sUartConfig.fc = UART_DISABLE_FC;
        err = ioctl(gTtyFd, HCIUARTSETBAUD, &sUartConfig);
        if (err < 0) {
            BPRINT_E("ioctl HCIUARTSETBAUD error:[%d] %s", errno, strerror(errno));
            goto exit;
        }

        BPRINT_I("set baudtate %d", CUST_BAUDRATE_DFT);
        cmd_hdr_baud(&sUartConfig, CUST_BAUDRATE_DFT);

        err = ioctl(gTtyFd, HCIUARTSETWAKEUP, NULL);
        if (err < 0) {
            BPRINT_E("ioctl HCIUARTSETWAKEUP error:[%d] %s", errno, strerror(errno));
            goto exit;
        }
    }
#endif

exit:
#ifdef __ANDROID__
    BPRINT_I("%s: exit notifyFd[%d]", __func__, notifyFd);
    if (notifyFd > 0) {
        close(notifyFd);
        notifyFd= -1;
    }
#endif

    BPRINT_I("%s: exit ttyFd[%d]", __func__, gTtyFd);

    /* unlock ttyFd */
    if (gTtyFd > 0) {
        err = ioctl(gTtyFd, HCIUARTDEINIT, NULL);
        if (err < 0) {
            BPRINT_E("ioctl HCIUARTDEINIT error:[%d] %s", errno, strerror(errno));
            BPRINT_I("deinit fail, wait...");
            (void)usleep(1000 * 1000);
        }
        BPRINT_I("unlock_flock");
        unlock_flock(gTtyFd, &fl, F_UNLCK, SEEK_SET);
        close(gTtyFd);
        gTtyFd = -1;
    }
    BPRINT_I("uart_launcher stop");
    return 0;
}
//---------------------------------------------------------------------------
