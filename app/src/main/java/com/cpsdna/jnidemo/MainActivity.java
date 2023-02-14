package com.cpsdna.jnidemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.cpsdna.jnidemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());

        //native
        CryptoNative cryptoNative = new CryptoNative();

        //open device
        int ret = cryptoNative.open();
        Log.d("Bilson","ret = " + ret);

        //data
        byte[] buf = {0x33, (byte) 0xCF,0x00,0x00,0x00,0x30};

        //write and read test
        cryptoNative.write(buf,buf.length);
        byte[] bufread = cryptoNative.read(buf.length);
        Log.d("Bilson","read data = " + bytes2hex(bufread));


        //transfer test
//        byte[] transferRcv = cryptoNative.transfer(buf,buf.length);
//        Log.d("Bilson","transfer rcv = " + bytes2hex(transferRcv));

        //close device
        cryptoNative.close();
    }


    public static String bytes2hex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        String tmp = null;
        for (byte b : bytes) {
            // 将每个字节与0xFF进行与运算，然后转化为10进制，然后借助于Integer再转化为16进制
            tmp = Integer.toHexString(0xFF & b);
            if (tmp.length() == 1) {
                tmp = "0" + tmp;
            }
            sb.append(tmp);
        }
        return sb.toString();

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}