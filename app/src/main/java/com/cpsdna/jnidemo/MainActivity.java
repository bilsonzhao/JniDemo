package com.cpsdna.jnidemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.cpsdna.jnidemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private ActivityMainBinding binding;
    Button btnReset;
    Button btnSelect;
    Button btnSelect2;
    CryptoNative cryptoNative;

    //data
    byte[] bufReset = {0x33, (byte) 0xCF,0x00,0x00,0x00,0x30};
//    byte[] bufSelect = {0x55,0x00, (byte) 0xa4,0x04,0x00,0x0e, (byte) 0x31,0x50,0x41,0x59,0x2e, (byte) 0x53,0x59, (byte) 0x53, (byte) 0x2e,0x44,0x44,0x46,0x30, (byte) 0x31, (byte) 0x36};

    byte[] bufSelect = {0x55,0x00, (byte) 0xa4,0x04,0x00,0x00,0x0e, (byte) 0xa0,0x00,0x00,0x05, (byte) 0x33, (byte) 0xc0, (byte) 0x00, (byte) 0xff, (byte) 0x86};
    byte[] bufSelect2 = {0x00,0x00,0x00,0x04, (byte) 0xed, (byte) 0x97};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());

        //native
        cryptoNative = new CryptoNative();

        //open device
        int ret = cryptoNative.open();
        Log.d("Bilson","open dev ret = " + ret);

        //button
        btnReset = findViewById(R.id.bt_reset);
        btnSelect = findViewById(R.id.bt_select);
        btnSelect2 = findViewById(R.id.bt_select2);

        btnReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

//                /*write *************************************/
//                cryptoNative.write(bufReset,bufReset.length);
//
//                try {
//                    Thread.sleep(100);
//                } catch (InterruptedException e) {
//                    e.printStackTrace();
//                }
//
//                /*read */
//                byte[] bufread1 = cryptoNative.read(6);
//                Log.d("Bilson","reset data = " + bytes2hex(bufread1));


                /*transfer******************************/
                cryptoNative.transfer(bufReset,bufReset.length);

                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                byte[] resetbuf = cryptoNative.transfer(new byte[6],6);
                Log.d("Bilson","reset data= " + bytes2hex(resetbuf));

            }
        });

        btnSelect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                /*write */
                cryptoNative.write(bufSelect,bufSelect.length);
                cryptoNative.write(bufSelect2,bufSelect2.length);

                try {
                    Thread.sleep(100); // 延时 0.1 秒
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                /*read */
                byte[] bufread1 = cryptoNative.read(16);
                Log.d("Bilson","read data = " + bytes2hex(bufread1));

                byte[] bufread2 = cryptoNative.read(16);
                Log.d("Bilson","read data = " + bytes2hex(bufread2));

                byte[] bufread3 = cryptoNative.read(16);
                Log.d("Bilson","read data = " + bytes2hex(bufread3));


//                /*transfer**************************************/
//                cryptoNative.transfer(bufSelect,bufSelect.length);
//                cryptoNative.transfer(bufSelect2,bufSelect2.length);
//
//                try {
//                    Thread.sleep(100); // 延时 0.1 秒
//                } catch (InterruptedException e) {
//                    e.printStackTrace();
//                }
//
//                byte[] readdata = cryptoNative.transfer(new byte[16],16);
//                Log.d("Bilson","readdata = " + bytes2hex(readdata));
            }
        });



        btnSelect2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //close device
        cryptoNative.close();
    }

    public static byte[] xorAndNot(byte[] bytes) {
        byte xorResult = 0;
        for (byte b : bytes) {
            xorResult ^= b;
        }
        byte notResult = (byte) ~xorResult;
        return new byte[] {notResult};
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
            tmp = "0x" + tmp + ", ";
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