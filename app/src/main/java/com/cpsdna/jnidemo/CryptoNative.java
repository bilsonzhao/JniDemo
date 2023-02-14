package com.cpsdna.jnidemo;

public class CryptoNative {

    private static final String TAG = "CryptoNative";

    public native int open();
    public native void close();
    public native byte[] read(int len);
    public native int write(byte[] buf, int len);
    public native byte[] transfer(byte[] buf, int len);

    static {
        System.loadLibrary("crypto");
    }

}
