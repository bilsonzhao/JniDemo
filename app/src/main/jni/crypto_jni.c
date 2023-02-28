//
// Created by Admin on 2023/2/13.
//

#include "com_cpsdna_jnidemo_CryptoNative.h"
#include <jni.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "android/log.h"
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
#include <stdbool.h>

static const char *TAG = "Bilson_crypto_jni";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

#define DEV_NAME    "/dev/spidev1.0"
#define BUF_SIZE 16
#define PROTOCOL_HEADER 0
#define PROTOCOL_SW1    1
#define PROTOCOL_SW2    2
#define PROTOCOL_LEN1   3
#define PROTOCOL_LEN2   4
#define AUDP_BUF_SIZE   263  //header +  info(max 261) + lrc
#define SPI_DELAY_TX_TO_RX   100000    //us，发送数据后到接收数据的延时时间
#define N_ACCEPT_DELAY       100       //us
#define GPIO_RESET_DELAY     100000    //us

bool DEBUG = true;
bool DEBUG_READ = false;
bool DEBUG_WRITE = false;

static jint fd;
static jint cs_fd;
static jint en_fd;
//spi config
static unsigned char mode = SPI_MODE_0;
static unsigned char bits = 8;
static unsigned char delay = 100;
static unsigned int speed = 5000000;

#define GPIO_VALUE_FILE "/sys/class/gpio/gpio130/value"
#define ENABLE_FILE "/sys/bus/platform/devices/soc:meig_dina_demo/output3Enable"

JNIEXPORT void set_cs_value(jstring value) {
    // to wait N_Accept ok
    usleep(N_ACCEPT_DELAY);
    if (write(cs_fd, value, 1) < 0) {
        LOGE("Failed to set cs value");
    }
}

JNIEXPORT jstring get_cs_value() {
    jstring value;
    if (read(cs_fd, value, 1) < 0) {
        //LOGE("Failed to get cs value");
    }
    return value;
}

JNIEXPORT jint reset_enable_gpio() {
    if (write(en_fd, "0", 1) < 0) {
        LOGE("Failed to pull gpio33 down");
        return -1;
    }
    usleep(GPIO_RESET_DELAY);//delay 100ms
    if (write(en_fd, "1", 1) < 0) {
        LOGE("Failed to pull gpio33 up");
        return -1;
    }
    return 0;
}

JNIEXPORT jint spi_read(jint fd, jboolean* buf, jint len) {
    set_cs_value("0");
    int count = read(fd, buf, len);
    set_cs_value("1");
    return count;
}

JNIEXPORT void spi_write(jint fd, jboolean* buf, jint len) {
    set_cs_value("0");
    write(fd, buf, len);
    set_cs_value("1");
}

JNIEXPORT jboolean xor(jboolean *ptr, jint n) {
    unsigned char result = 0;
    for (int i = 1; i < n - 1; i++) {
        result ^= *(ptr + i);
    }
    return ~result;
}
/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    open
 * Signature: ()I
 */
jint JNICALL Java_com_cpsdna_jnidemo_CryptoNative_open
        (JNIEnv *env, jobject clazz) {

    //init enbale first
    en_fd = open(ENABLE_FILE, O_WRONLY);
    if (en_fd < 0) {
        LOGE("Failed to open en_fd file");
    }
    reset_enable_gpio();

    cs_fd = open(GPIO_VALUE_FILE, O_WRONLY);
    if (cs_fd < 0) {
        LOGE("Failed to open cs_fd file");
    }

    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        LOGE( "can not open SPI device\n" );
    }
    int ret = ioctl(fd, SPI_IOC_WR_MODE32,&mode);
    if (ret == -1)
        LOGE("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE32,&mode);
    if (ret == -1)
        LOGE("can't get spi mode");

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        LOGE("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        LOGE("can't get bits per word");

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        LOGE("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        LOGE("can't get max speed hz");

    LOGD("spi mode: %d\n", mode);
    LOGD("bits per word: %d\n", bits);
    LOGD("max speed: %d Hz (%d MHz)\n", speed, speed/1000000);

    return fd;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_cpsdna_jnidemo_CryptoNative_close
        (JNIEnv *env, jobject clazz) {
    if(DEBUG) LOGD("JNI spi close ... ....");
    //close all fd
    close(fd);
    close(cs_fd);
    close(en_fd);
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    read
 * Signature: ([BI)I
 */
JNIEXPORT jbyteArray JNICALL Java_com_cpsdna_jnidemo_CryptoNative_read
        (JNIEnv *env, jobject clazz, jint len) {

    if(DEBUG_READ) LOGD("JNI spi read ... ...");
    usleep(SPI_DELAY_TX_TO_RX);//delay 2000us to wait write ok

    jbyteArray jarray = (*env)->NewByteArray(env,AUDP_BUF_SIZE);
    jboolean *array = (*env)->GetByteArrayElements(env,jarray, NULL);
    if (array == NULL)
    {
        LOGE("JNI spi read: GetByteArrayElements fail!");
        return -1;
    }

    unsigned char header;
    int total_read = 0;
    int to_read = 0;

    //first pull cs down
    set_cs_value("0");
    read(fd, &header, 1);
    if(DEBUG_READ)LOGD("JNI spi read Header: %#x", header);

    array[PROTOCOL_HEADER] = header;
    total_read += 1;
    /****** S帧/R帧 **********************/
    if (array[PROTOCOL_HEADER] == 0x33 || array[PROTOCOL_HEADER] == 0x99) {
        to_read = 5;//4 byte data + 1 byte lrc
        read(fd,array+total_read,to_read);
        total_read += to_read;
        //last pull cs up
        set_cs_value("1");
    } else if (array[PROTOCOL_HEADER] == 0x55) {/***** I帧 ********/
        unsigned char sw_byte[2];
        read(fd, &sw_byte, 2);
        array[PROTOCOL_SW1] = sw_byte[0];
        array[PROTOCOL_SW2] = sw_byte[1];
        total_read +=2;

        unsigned char length_bytes[2];
        read(fd, length_bytes, 2);
        array[PROTOCOL_LEN1] = length_bytes[0];// Len1
        array[PROTOCOL_LEN2] = length_bytes[1];// Len2
        total_read += 2;

        int payload_length = (array[PROTOCOL_LEN1] << 8) |  array[PROTOCOL_LEN2];
        payload_length++;  // Len1 + Len2 + lrc
        int remaining = payload_length;// 剩余数据总长度

        /****这里已经读取了5个字节，在一个cs周期内最多只能读取16个字节，接下来要判断remaining是否大于11个字节
        1.如果remaining小于11，直接一包发完*/
        if(remaining <= 11) {
            to_read = remaining;
            read(fd, array + total_read, to_read);
            total_read += to_read;
            //last pull cs up
            set_cs_value("1");
        } else {
            /*** 2.如果remaing大于11，先发11，之后再判断剩余自己字节是否大于16 ***/
            to_read = 11;
            int read_count = read(fd, array + total_read, to_read);
            remaining -= read_count;
            total_read += to_read;
            //last pull cs up
            set_cs_value("1");

            while (remaining > 0) {
                to_read = remaining;
                if (to_read > BUF_SIZE) {
                    to_read = BUF_SIZE;
                }
                read_count = spi_read(fd, array + total_read, to_read);
                remaining -= read_count;
                total_read += read_count;
            }
        }
    }// I帧 end

    //ensure cs=1 when transfer complete
    set_cs_value("1");

    //only save total_read
    jbyteArray jread_array = (*env)->NewByteArray(env,total_read);
    jboolean *readBuf = (*env)->GetByteArrayElements(env,jread_array, NULL);
    memcpy(readBuf, array, total_read);

    //data check
    /********校验数据是否正确 */
    jboolean lrc = xor(readBuf,total_read);
    if(lrc != readBuf[total_read-1]) {
        LOGE("JNI spi read lrc: %#x",lrc);
        LOGE("JNI spi read data check lrc failed");
    }

    for (int i=0; i<total_read; i++)
    {
        if(DEBUG_READ) LOGD("JNI spi read: buf: %#x", *(readBuf + i));
    }

    (*env)->ReleaseByteArrayElements(env, jread_array, readBuf, 0);
    (*env)->ReleaseByteArrayElements(env, jarray, array, 0);
    return jread_array;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    write
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_cpsdna_jnidemo_CryptoNative_write
        (JNIEnv *env, jobject clazz, jbyteArray jwrite_arr, jint len) {

    if(DEBUG_WRITE) LOGD("JNI spi write ... ...");

    jboolean *array = (*env)->GetByteArrayElements(env, jwrite_arr, NULL);
    if (array == NULL)
    {
        LOGE("JNI spi write: GetByteArrayElements fail!");
        return -1;
    }

    for(int i = 0; i < len; i++)
    {
        if(DEBUG_WRITE) LOGD("JNI spi write: data : %#x\n",*(array + i));
    }

    (*env)->ReleaseByteArrayElements(env, jwrite_arr, array, 0);

    int remaining = len;
    while (remaining > 0) {
        int send_size = remaining > BUF_SIZE ? BUF_SIZE : remaining;
        spi_write(fd,array,send_size);
        array += BUF_SIZE;
        remaining -= BUF_SIZE;
    }
    return 0;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    transfer
 * Signature: ([BI)I
 * 收发一体的函数，暂时未实现，使用read/write替代
 */
JNIEXPORT jbyteArray JNICALL Java_com_cpsdna_jnidemo_CryptoNative_transfer
        (JNIEnv *env, jobject clazz, jbyteArray jtransfer_arr, jint len) {

    jbyte *array = NULL;
    jboolean *buf;

    array = (*env)->GetByteArrayElements(env, jtransfer_arr, NULL);
    if (array == NULL)
    {
        LOGE("JNI spi transfer: GetByteArrayElements fail!");
        return -1;
    }

    buf = (jboolean *)calloc(sizeof(*array), len);
    if(buf == NULL)
    {
        LOGE("JNI spi transfer: calloc fail!");
        return -1;
    }

    for(int i = 0; i < len; i++)
    {
        *(buf + i) = (jboolean)(*(array + i));
        if(DEBUG) LOGD("JNI spi transfer: data : %#x\n",*(buf + i));
    }

    uint8_t rx[len];
    struct spi_ioc_transfer tr = {
            .tx_buf = (unsigned long)buf,
            .rx_buf = (unsigned long)rx,
            .len = len,
            .delay_usecs = delay,
            .speed_hz = speed,
            .bits_per_word = bits,
    };

    if (mode & SPI_TX_QUAD)
        tr.tx_nbits = 4;
    else if (mode & SPI_TX_DUAL)
        tr.tx_nbits = 2;
    if (mode & SPI_RX_QUAD)
        tr.rx_nbits = 4;
    else if (mode & SPI_RX_DUAL)
        tr.rx_nbits = 2;
    if (!(mode & SPI_LOOP)) {
        if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
            tr.rx_buf = 0;
        else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
            tr.tx_buf = 0;
    }

    //ioctl transfer data
    set_cs_value("0");
    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    set_cs_value("1");

    jbyteArray jrcvarray = (*env)->NewByteArray(env,len);
    jboolean *rcvarray = (*env)->GetByteArrayElements(env,jrcvarray, NULL);

    for(int i = 0; i < len; i++)
    {
        if(DEBUG) LOGD("JNI transfer receiver data : %#x\n",rx[i]);
        *(rcvarray + i) = (jchar)(*(rx + i));
    }

    (*env)->ReleaseByteArrayElements(env, jtransfer_arr, array, 0);
    (*env)->ReleaseByteArrayElements(env, jrcvarray, rcvarray, 0);

    free(buf);

    return jrcvarray;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    hwReset
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_cpsdna_jnidemo_CryptoNative_hwReset
        (JNIEnv *env, jobject clazz){
    reset_enable_gpio();
}