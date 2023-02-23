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
bool DEBUG = false;
static jint fd;
static unsigned char mode = SPI_MODE_0;
static unsigned char bits = 8;
static unsigned char delay = 100;
static unsigned int speed = 1000000;

#define GPIO_VALUE_FILE "/sys/class/gpio/gpio130/value"
#define ENABLE_FILE "/sys/bus/platform/devices/soc:meig_dina_demo/output3Enable"


JNIEXPORT void set_cs_value(jstring value) {

    int gpio_fd;
    gpio_fd = open(GPIO_VALUE_FILE, O_WRONLY);
    if (gpio_fd < 0) {
        LOGE("Failed to open value file");
    }
    if (write(gpio_fd, value, 1) < 0) {
        LOGE("Failed to set value");
    }
    close(gpio_fd);
}

JNIEXPORT void init_enable_gpio() {

    int gpio_fd;
    gpio_fd = open(ENABLE_FILE, O_WRONLY);
    if (gpio_fd < 0) {
        LOGE("Failed to open gpio33 file");
    }
    if (write(gpio_fd, "0", 1) < 0) {
        LOGE("Failed to pull gpio33 down");
    }
    usleep(10000);
    if (write(gpio_fd, "1", 1) < 0) {
        LOGE("Failed to pull gpio33 up");
    }
    close(gpio_fd);
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    open
 * Signature: ()I
 */
jint JNICALL Java_com_cpsdna_jnidemo_CryptoNative_open
        (JNIEnv *env, jobject clazz) {

    //init enbale first
    init_enable_gpio();

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
    close(fd);
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    read
 * Signature: ([BI)I
 */
JNIEXPORT jbyteArray JNICALL Java_com_cpsdna_jnidemo_CryptoNative_read
        (JNIEnv *env, jobject clazz, jint len) {

    if(DEBUG) LOGD("JNI spi read ... ...");

    jboolean *buf;
    jbyteArray jarray = (*env)->NewByteArray(env,len);
    jboolean *array = (*env)->GetByteArrayElements(env,jarray, NULL);

    if (array == NULL)
    {
        LOGE("JNI spi read: GetByteArrayElements fail!");
        return -1;
    }

    buf = (jboolean *)calloc(sizeof(*array), len);
    if (buf == NULL)
    {
        LOGE("JNI spi read: calloc fail!");
        return -1;
    }

    //pull cs down
    set_cs_value("0");
    read(fd, buf, len);
    set_cs_value("1");

    for (int i=0; i<len; i++)
    {
        if(DEBUG) LOGD("JNI spi read: buf: %#x", *(buf + i));
        *(array + i) = (jchar)(*(buf + i));
    }

    (*env)->ReleaseByteArrayElements(env, jarray, array, 0);

    free(buf);

    return jarray;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    write
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_cpsdna_jnidemo_CryptoNative_write
        (JNIEnv *env, jobject clazz, jbyteArray jwrite_arr, jint len) {

    if(DEBUG) LOGD("JNI spi write ... ...");

    jbyte *array = NULL;
    jboolean *buf;
    array = (*env)->GetByteArrayElements(env, jwrite_arr, NULL);
    if (array == NULL)
    {
        LOGE("JNI spi write: GetByteArrayElements fail!");
        return -1;
    }

    buf = (jboolean *)calloc(sizeof(*array), len);
    if(buf == NULL)
    {
        LOGE("JNI spi write: calloc fail!");
        return -1;
    }

    for(int i = 0; i < len; i++)
    {
        *(buf + i) = (jboolean)(*(array + i));
        if(DEBUG) LOGD("JNI spi write: data : %#x\n",*(buf + i));
    }

    (*env)->ReleaseByteArrayElements(env, jwrite_arr, array, 0);

    set_cs_value("0");
    write(fd, buf, len);
    set_cs_value("1");

    free(buf);

    return 0;
}

/*
 * Class:     com_cpsdna_jnidemo_CryptoNative
 * Method:    transfer
 * Signature: ([BI)I
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