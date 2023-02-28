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
    Button btnHwReset;
    CryptoNative cryptoNative;

    //data
    byte[] bufReset = {0x33, (byte) 0xCF,0x00,0x00,0x00,0x30};
    byte[] txbuf = {0x55,0x00, (byte) 0xa4,0x04,0x00,0x00,0x0e, (byte) 0xa0,0x00,0x00,0x05, (byte) 0x33, (byte) 0xc0, (byte) 0x00, (byte) 0xff, (byte) 0x86,0x00,0x00,0x00,0x04, (byte) 0xed, (byte) 0x97};
    byte[] bufauthen = {0x55, (byte) 0x80, (byte) 0xca, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x69, 0x51, 0x60, 0x57, 0x52, 0x04, 0x00, 0x01, 0x22, 0x06, 0x21, 0x10, 0x21, 0x31, (byte) 0xf1};


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
        btnHwReset = findViewById(R.id.bt_hwreset);

        btnReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                /*write *************************************/
                cryptoNative.write(bufReset,bufReset.length);
                /*read */
                byte[] bufread1 = cryptoNative.read(256);
                Log.d("Bilson","reset data = " + bytes2hex(bufread1));
            }
        });

        btnSelect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                /*write */
                cryptoNative.write(txbuf,txbuf.length);
                /*read */
                byte[] bufread1 = cryptoNative.read(50);
                Log.d("Bilson","read data = " + bytes2hex(bufread1));

            }
        });


        btnSelect2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                /*write */
                cryptoNative.write(bufauthen,bufauthen.length);
                /*read */
                byte[] bufread1 = cryptoNative.read(50);
                Log.d("Bilson","read data = " + bytes2hex(bufread1));

            }
        });

        btnHwReset.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                int ret = cryptoNative.hwReset();
                Log.d("Bilson","hwReset = " + ret);
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